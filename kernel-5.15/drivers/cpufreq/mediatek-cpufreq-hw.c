// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/bitfield.h>
#include <linux/cpufreq.h>
#include <linux/energy_model.h>
#include <linux/init.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>
#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
#include <linux/device.h>
#include <trace/hooks/cpufreq.h>
#endif

#define LUT_MAX_ENTRIES			32U
#define LUT_FREQ			GENMASK(11, 0)
#define LUT_ROW_SIZE			0x4
#define CPUFREQ_HW_STATUS		BIT(0)
#define SVS_HW_STATUS			BIT(1)
#define POLL_USEC			1000
#define TIMEOUT_USEC			300000
#define REG_FREQ_SCALING		0x4cc

enum {
	REG_FREQ_LUT_TABLE,
	REG_FREQ_ENABLE,
	REG_FREQ_PERF_STATE,
	REG_FREQ_HW_STATE,
	REG_EM_POWER_TBL,
	REG_FREQ_LATENCY,

	REG_ARRAY_SIZE,
};

struct cpufreq_mtk {
	struct cpufreq_frequency_table *table;
	void __iomem *reg_bases[REG_ARRAY_SIZE];
	int nr_opp;
	unsigned int last_index;
	cpumask_t related_cpus;
};

static const u16 cpufreq_mtk_offsets[REG_ARRAY_SIZE] = {
	[REG_FREQ_LUT_TABLE]	= 0x0,
	[REG_FREQ_ENABLE]	= 0x84,
	[REG_FREQ_PERF_STATE]	= 0x88,
	[REG_FREQ_HW_STATE]	= 0x8c,
	[REG_EM_POWER_TBL]	= 0x90,
	[REG_FREQ_LATENCY]	= 0x114,
};

static struct cpufreq_mtk *mtk_freq_domain_map[NR_CPUS];
static bool freq_scaling_disabled = true;

static int look_up_cpu(struct device *cpu_dev)
{
	unsigned int cpu = 0;

	for_each_possible_cpu(cpu) {
		if (cpu_dev == get_cpu_device(cpu))
			return cpu;
	}

	return 0; /* fallback to cpu0 */
}


static int __maybe_unused
mtk_cpufreq_get_cpu_power(unsigned long *power, unsigned long *KHz,
		struct device *cpu_dev)
{
	int cpu = look_up_cpu(cpu_dev);
	struct cpufreq_mtk *c = mtk_freq_domain_map[cpu];
	int i;

	for (i = 0; i < c->nr_opp; i++) {
		if (c->table[i].frequency < *KHz)
			break;
	}
	i--;

	*KHz = c->table[i].frequency;
	*power = readl_relaxed(c->reg_bases[REG_EM_POWER_TBL] +
			    i * LUT_ROW_SIZE) / 1000;

	return 0;
}

static int mtk_cpufreq_hw_target_index(struct cpufreq_policy *policy,
				       unsigned int index)
{
	struct cpufreq_mtk *c = policy->driver_data;

	writel_relaxed(index, c->reg_bases[REG_FREQ_PERF_STATE]);

	return 0;
}

static unsigned int mtk_cpufreq_hw_get(unsigned int cpu)
{
	struct cpufreq_mtk *c;
	unsigned int index, nr_opp;

	c = mtk_freq_domain_map[cpu];
	nr_opp = c->nr_opp;

	index = readl_relaxed(c->reg_bases[REG_FREQ_PERF_STATE]);

	if (index <= LUT_MAX_ENTRIES - 1) {
		index = min(index, LUT_MAX_ENTRIES - 1);
		return c->table[index].frequency;
	} else if (c->table[0].frequency >= index && index >= c->table[nr_opp - 1].frequency) {
		return index;
	}

	return -1;
}

static unsigned int mtk_cpufreq_hw_fast_switch(struct cpufreq_policy *policy,
					       unsigned int target_freq)
{
	struct cpufreq_mtk *c = policy->driver_data;
	unsigned int index;
#if IS_ENABLED(CONFIG_MTK_IRQ_MONITOR_DEBUG)
	u64 ts[2];

	ts[0] = sched_clock();
#endif

	if (policy->cached_target_freq == target_freq)
		index = policy->cached_resolved_idx;
	else
		index = cpufreq_table_find_index_dl(policy, target_freq);

	if (!freq_scaling_disabled) {
		writel_relaxed(target_freq, c->reg_bases[REG_FREQ_PERF_STATE]);

		if (c->last_index == index)
			return 0;
		c->last_index = index;
	} else
		writel_relaxed(index, c->reg_bases[REG_FREQ_PERF_STATE]);

#if IS_ENABLED(CONFIG_MTK_IRQ_MONITOR_DEBUG)
	ts[1] = sched_clock();

	if ((ts[1] - ts[0] > 100000ULL) && in_hardirq()) {
		printk_deferred("%s duration %llu, ts[0]=%llu, ts[1]=%llu\n",
			__func__, ts[1] - ts[0], ts[0], ts[1]);
	}
#endif

	return policy->freq_table[index].frequency;
}

static int mtk_cpufreq_hw_cpu_init(struct cpufreq_policy *policy)
{
	struct cpufreq_mtk *c;
	struct device *cpu_dev;
	struct em_data_callback em_cb = EM_DATA_CB(mtk_cpufreq_get_cpu_power);
	struct pm_qos_request *qos_request;
	int sig, pwr_hw = CPUFREQ_HW_STATUS | SVS_HW_STATUS;
	unsigned int latency;

	qos_request = kzalloc(sizeof(*qos_request), GFP_KERNEL);
	if (!qos_request)
		return -ENOMEM;

	cpu_dev = get_cpu_device(policy->cpu);
	if (!cpu_dev) {
		pr_info("failed to get cpu%d device\n", policy->cpu);
		kfree(qos_request);
		return -ENODEV;
	}

	c = mtk_freq_domain_map[policy->cpu];
	if (!c) {
		pr_info("No scaling support for CPU%d\n", policy->cpu);
		kfree(qos_request);
		return -ENODEV;
	}

	cpumask_copy(policy->cpus, &c->related_cpus);

	policy->freq_table = c->table;
	policy->driver_data = c;

	latency = readl_relaxed(c->reg_bases[REG_FREQ_LATENCY]);
	if (!latency)
		latency = CPUFREQ_ETERNAL;

	/* us convert to ns */
	policy->cpuinfo.transition_latency = latency * 1000;

	policy->fast_switch_possible = true;

	/* Let CPUs leave idle-off state for SVS CPU initializing */
	cpu_latency_qos_add_request(qos_request, PM_QOS_DEFAULT_VALUE);

	/* HW should be in enabled state to proceed now */
	writel_relaxed(0x1, c->reg_bases[REG_FREQ_ENABLE]);

	if (readl_poll_timeout(c->reg_bases[REG_FREQ_HW_STATE], sig,
			       (sig & pwr_hw) == pwr_hw, POLL_USEC,
			       TIMEOUT_USEC)) {
		if (!(sig & CPUFREQ_HW_STATUS)) {
			pr_info("cpufreq hardware of CPU%d is not enabled\n",
				policy->cpu);
			cpu_latency_qos_remove_request(qos_request);
			kfree(qos_request);
			return -ENODEV;
		}

		pr_info("SVS of CPU%d is not enabled\n", policy->cpu);
	}

	em_dev_register_perf_domain(cpu_dev, c->nr_opp, &em_cb, policy->cpus,
			true);
	cpu_latency_qos_remove_request(qos_request);
	kfree(qos_request);

	return 0;
}

static int mtk_cpufreq_hw_cpu_exit(struct cpufreq_policy *policy)
{
	struct cpufreq_mtk *c;

	c = mtk_freq_domain_map[policy->cpu];
	if (!c) {
		pr_info("No scaling support for CPU%d\n", policy->cpu);
		return -ENODEV;
	}

	/* HW should be in paused state now */
	writel_relaxed(0x0, c->reg_bases[REG_FREQ_ENABLE]);

	return 0;
}

#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
static void mtk_cpufreq_suppress(void *data, struct device *dev, int val)
{
	dev_set_uevent_suppress(dev, val);
}
#endif

static struct cpufreq_driver cpufreq_mtk_hw_driver = {
	.flags		= CPUFREQ_NEED_INITIAL_FREQ_CHECK |
			  CPUFREQ_HAVE_GOVERNOR_PER_POLICY |
			  CPUFREQ_IS_COOLING_DEV,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= mtk_cpufreq_hw_target_index,
	.get		= mtk_cpufreq_hw_get,
	.init		= mtk_cpufreq_hw_cpu_init,
	.exit		= mtk_cpufreq_hw_cpu_exit,
	.fast_switch	= mtk_cpufreq_hw_fast_switch,
	.name		= "mtk-cpufreq-hw",
	.attr		= cpufreq_generic_attr,
};

static int mtk_cpu_create_freq_table(struct platform_device *pdev,
				     struct cpufreq_mtk *c)
{
	struct device *dev = &pdev->dev;
	void __iomem *base_table;
	u32 data, i, freq, prev_freq = 0;

	c->table = devm_kcalloc(dev, LUT_MAX_ENTRIES + 1,
				sizeof(*c->table), GFP_KERNEL);
	if (!c->table)
		return -ENOMEM;

	base_table = c->reg_bases[REG_FREQ_LUT_TABLE];

	for (i = 0; i < LUT_MAX_ENTRIES; i++) {
		data = readl_relaxed(base_table + (i * LUT_ROW_SIZE));
		freq = FIELD_GET(LUT_FREQ, data) * 1000;

		if (freq == prev_freq)
			break;

		c->table[i].frequency = freq;

		dev_dbg(dev, "index=%d freq=%d\n",
			i, c->table[i].frequency);

		prev_freq = freq;
	}

	c->table[i].frequency = CPUFREQ_TABLE_END;
	c->nr_opp = i;

	return 0;
}

static int mtk_get_related_cpus(int index, struct cpufreq_mtk *c)
{
	struct device_node *cpu_np;
	struct of_phandle_args args;
	int cpu, ret;

	for_each_possible_cpu(cpu) {
		cpu_np = of_cpu_device_node_get(cpu);
		if (!cpu_np)
			continue;

		ret = of_parse_phandle_with_args(cpu_np, "performance-domains",
						 "#performance-domain-cells", 0,
						 &args);
		of_node_put(cpu_np);
		if (ret < 0)
			continue;

		if (index == args.args[0]) {
			cpumask_set_cpu(cpu, &c->related_cpus);
			mtk_freq_domain_map[cpu] = c;
		}
	}

	return 0;
}

static int mtk_cpu_resources_init(struct platform_device *pdev,
				  unsigned int cpu, int index,
				  const u16 *offsets)
{
	struct cpufreq_mtk *c;
	struct device *dev = &pdev->dev;
	int ret, i;
	void __iomem *base;

	if (mtk_freq_domain_map[cpu])
		return 0;

	c = devm_kzalloc(dev, sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	base = devm_platform_ioremap_resource(pdev, index);
	if (IS_ERR(base))
		return PTR_ERR(base);

	for (i = REG_FREQ_LUT_TABLE; i < REG_ARRAY_SIZE; i++)
		c->reg_bases[i] = base + offsets[i];

	ret = mtk_get_related_cpus(index, c);
	if (ret) {
		dev_info(dev, "Domain-%d failed to get related CPUs\n", index);
		return ret;
	}

	ret = mtk_cpu_create_freq_table(pdev, c);
	if (ret) {
		dev_info(dev, "Domain-%d failed to create freq table\n", index);
		return ret;
	}

	return 0;
}

static int mtk_cpufreq_hw_driver_probe(struct platform_device *pdev)
{
	struct device_node *cpu_np;
	struct device_node *hvfs_node;
	struct of_phandle_args args;
	struct resource *csram_res;
	struct platform_device *pdev_c;
	static void __iomem *csram_base;
	const u16 *offsets;
	unsigned int cpu;
	int ret;
	offsets = of_device_get_match_data(&pdev->dev);
	if (!offsets)
		return -EINVAL;

	hvfs_node = of_find_node_by_name(NULL, "cpuhvfs");
	if (hvfs_node == NULL) {
		pr_notice("failed to find node @ %s\n", __func__);
		return -ENODEV;
	}

	pdev_c = of_find_device_by_node(hvfs_node);
	if (pdev_c == NULL) {
		pr_notice("failed to find pdev @ %s\n", __func__);
		return -EINVAL;
	}

	csram_res = platform_get_resource(pdev_c, IORESOURCE_MEM, 1);
	if (!csram_res) {
		pr_notice("failed to get mem resource @ %s\n", __func__);
		return -ENODEV;
	}

	if (!request_mem_region(csram_res->start + REG_FREQ_SCALING, LUT_ROW_SIZE,
			csram_res->name)) {
		pr_notice("failed to request resource %pR\n", csram_res);
		return -EBUSY;
	}

	csram_base = ioremap(csram_res->start + REG_FREQ_SCALING, LUT_ROW_SIZE);
	if (!csram_base) {
		pr_notice("failed to map csram_base @ %s\n", __func__);
		ret = -ENOMEM;
		goto release_region;
	}

	if (readl_relaxed(csram_base))
		freq_scaling_disabled = false;

	for_each_possible_cpu(cpu) {
		cpu_np = of_cpu_device_node_get(cpu);
		if (!cpu_np) {
			dev_info(&pdev->dev, "Failed to get cpu %d device\n",
				cpu);
			return -ENODEV;
		}

		ret = of_parse_phandle_with_args(cpu_np, "performance-domains",
						 "#performance-domain-cells", 0,
						 &args);
		if (ret < 0)
			return ret;

		/* Get the bases of cpufreq for domains */
		ret = mtk_cpu_resources_init(pdev, cpu, args.args[0], offsets);
		if (ret) {
			dev_info(&pdev->dev, "CPUFreq resource init failed\n");
			return ret;
		}
	}

	ret = cpufreq_register_driver(&cpufreq_mtk_hw_driver);
	if (ret) {
		dev_info(&pdev->dev, "CPUFreq HW driver failed to register\n");
		return ret;
	}

	return 0;

release_region:
	release_mem_region(csram_res->start, resource_size(csram_base));
#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
	ret = register_trace_android_vh_cpufreq_offline(mtk_cpufreq_suppress, NULL);
#endif

	return ret;
}

static int mtk_cpufreq_hw_driver_remove(struct platform_device *pdev)
{
	return cpufreq_unregister_driver(&cpufreq_mtk_hw_driver);
}

static const struct of_device_id mtk_cpufreq_hw_match[] = {
	{ .compatible = "mediatek,cpufreq-hw", .data = &cpufreq_mtk_offsets },
	{}
};

static struct platform_driver mtk_cpufreq_hw_driver = {
	.probe = mtk_cpufreq_hw_driver_probe,
	.remove = mtk_cpufreq_hw_driver_remove,
	.driver = {
		.name = "mtk-cpufreq-hw",
		.of_match_table = mtk_cpufreq_hw_match,
	},
};
module_platform_driver(mtk_cpufreq_hw_driver);

MODULE_DESCRIPTION("Mediatek cpufreq-hw driver");
MODULE_LICENSE("GPL v2");
