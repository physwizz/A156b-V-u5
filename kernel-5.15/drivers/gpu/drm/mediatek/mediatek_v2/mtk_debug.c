// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/wait.h>
#if IS_ENABLED(CONFIG_DEBUG_FS)
#include <linux/debugfs.h>
#include <mt-plat/mrdump.h>
#endif
#include <linux/panic_notifier.h>
#include <linux/kdebug.h>

#if IS_ENABLED(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#endif
#include <linux/sched/clock.h>
#include <linux/of_address.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_framebuffer.h>

#if IS_ENABLED(CONFIG_DRM_PANEL_MCD_COMMON)
#include <drm/drm_vblank.h>
#endif

#include "mtk_dump.h"
#include "mtk_debug.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_mmp.h"
#include "mtk_drm_drv.h"
#include "mtk_drm_assert.h"
#include "mtk_drm_helper.h"
#include "mtk_layering_rule.h"
#include "mtk_drm_lowpower.h"
#ifdef IF_ZERO
#include "mt_iommu.h"
#include "mtk_iommu_ext.h"
#endif
#include "mtk_drm_gem.h"
#include "mtk_drm_fb.h"
#include "mtk_disp_aal.h"
#include "mtk_disp_c3d.h"
#include "mtk_disp_tdshp.h"
#include "mtk_disp_oddmr/mtk_disp_oddmr.h"
#include "mtk_dp_debug.h"
#include "mtk_drm_arr.h"
#include "mtk_drm_graphics_base.h"
#include "mtk_dsi.h"
#include "mtk_disp_bdg.h"

#include "mtk_notify.h"

#define DISP_REG_CONFIG_MMSYS_CG_SET(idx) (0x104 + 0x10 * (idx))
#define DISP_REG_CONFIG_MMSYS_CG_CLR(idx) (0x108 + 0x10 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_EN(idx) (0x200 + 0x20 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RST(idx) (0x204 + 0x20 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON0(idx) (0x208 + 0x20 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON1(idx) (0x20c + 0x20 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RD_ADDR(idx) (0x210 + 0x20 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_WR_ADDR(idx) (0x214 + 0x20 * (idx))
#define DISP_REG_CONFIG_DISP_FAKE_ENG_STATE(idx) (0x218 + 0x20 * (idx))
#define DISP_REG_CONFIG_RDMA_SHARE_SRAM_CON (0x654)
#define	DISP_RDMA_FAKE_SMI_SEL(idx) (BIT(4 + idx))
#define SMI_LARB_VC_PRI_MODE (0x020)
#define SMI_LARB_NON_SEC_CON(port) (0x380 + 4 * (port))
#define GET_M4U_PORT 0x1F

int dsi0_panel_id = -1;
EXPORT_SYMBOL(dsi0_panel_id);

#define INIT_TID 1

#if IS_ENABLED(CONFIG_DEBUG_FS)
static struct dentry *mtkfb_dbgfs;
#endif

#if IS_ENABLED(CONFIG_PROC_FS)
static struct proc_dir_entry *mtkfb_procfs;
static struct proc_dir_entry *disp_lowpower_proc;
static struct proc_dir_entry *mtkfb_debug_procfs;
#endif
static struct drm_device *drm_dev;

bool g_mobile_log;
EXPORT_SYMBOL(g_mobile_log);
bool g_fence_log;
bool g_detail_log;
EXPORT_SYMBOL(g_detail_log);
bool g_msync_debug;
EXPORT_SYMBOL(g_msync_debug);
bool g_gpuc_direct_push;
EXPORT_SYMBOL(g_gpuc_direct_push);
bool g_ovl_bwm_debug;
EXPORT_SYMBOL(g_ovl_bwm_debug);
bool g_vidle_apsrc_debug;
EXPORT_SYMBOL(g_vidle_apsrc_debug);
bool g_profile_log;

bool g_irq_log;
bool g_trace_log;
unsigned int mipi_volt;
unsigned int disp_met_en;
unsigned int disp_met_condition;
unsigned int lfr_dbg;
unsigned int lfr_params;
unsigned int disp_spr_bypass;
unsigned int disp_cm_bypass;
static unsigned int m_old_pq_persist_property[32];
unsigned int m_new_pq_persist_property[32];
unsigned int g_mml_mode;
#if IS_ENABLED(CONFIG_MTK_DISP_DEBUG)
struct wr_online_dbg g_wr_reg;
#endif

int gCaptureOVLEn;
int gCaptureWDMAEn;
int gCapturePriLayerDownX = 20;
int gCapturePriLayerDownY = 20;
int gCaptureOutLayerDownX = 20;
int gCaptureOutLayerDownY = 20;
int gCaptureAssignLayer;
u64 vfp_backup;
static int hrt_lp_switch;

static struct completion cwb_cmp;

struct logger_buffer {
	char **buffer_ptr;
	unsigned int len;
	unsigned int id;
	const unsigned int cnt;
	const unsigned int size;
};

static DEFINE_SPINLOCK(dprec_err_logger_spinlock);
static DEFINE_SPINLOCK(dprec_fence_logger_spinlock);
static DEFINE_SPINLOCK(dprec_dbg_logger_spinlock);
static DEFINE_SPINLOCK(dprec_dump_logger_spinlock);
/* redundant spin lock prevent exception condition */
static DEFINE_SPINLOCK(dprec_status_logger_spinlock);

static struct list_head cb_data_list[MAX_CRTC];
static DEFINE_SPINLOCK(cb_data_clock_lock);

static char **err_buffer;
static char **fence_buffer;
static char **dbg_buffer;
static char **dump_buffer;
static char **status_buffer;
static struct logger_buffer dprec_logger_buffer[DPREC_LOGGER_PR_NUM] = {
	{0, 0, 0, ERROR_BUFFER_COUNT, LOGGER_BUFFER_SIZE},
	{0, 0, 0, FENCE_BUFFER_COUNT, LOGGER_BUFFER_SIZE},
	{0, 0, 0, DEBUG_BUFFER_COUNT, LOGGER_BUFFER_SIZE},
	{0, 0, 0, DUMP_BUFFER_COUNT, LOGGER_BUFFER_SIZE},
	{0, 0, 0, STATUS_BUFFER_COUNT, LOGGER_BUFFER_SIZE},
};
static bool is_buffer_init;
static char *debug_buffer;
static bool logger_enable = 1;

static int draw_RGBA8888_buffer(char *va, int w, int h,
		       char r, char g, char b, char a)
{
	int i, j;
	int Bpp =  mtk_get_format_bpp(DRM_FORMAT_RGBA8888);

	for (i = 0; i < h; i++)
		for (j = 0; j < w; j++) {
			int x = j * Bpp + i * w * Bpp;

			va[x++] = a;
			va[x++] = b;
			va[x++] = g;
			va[x++] = r;
		}

	return 0;
}

static int prepare_fake_layer_buffer(struct drm_crtc *crtc)
{
	unsigned int i;
	size_t size;
	struct mtk_drm_gem_obj *mtk_gem;
	struct drm_mode_fb_cmd2 mode = { 0 };
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_fake_layer *fake_layer = &mtk_crtc->fake_layer;

	if (fake_layer->init)
		return 0;

	mode.width = crtc->state->adjusted_mode.hdisplay;
	mode.height = crtc->state->adjusted_mode.vdisplay;
	mode.pixel_format = DRM_FORMAT_RGBA8888;
	mode.pitches[0] = mode.width
			* mtk_get_format_bpp(mode.pixel_format);
	size = mode.width * mode.height
		* mtk_get_format_bpp(mode.pixel_format);

	for (i = 0; i < PRIMARY_OVL_PHY_LAYER_NR; i++) {
		mtk_gem = mtk_drm_gem_create(crtc->dev, size, true);
		draw_RGBA8888_buffer(mtk_gem->kvaddr, mode.width, mode.height,
			(!((i + 0) % 3)) * 255 / (i / 3 + 1),
			(!((i + 1) % 3)) * 255 / (i / 3 + 1),
			(!((i + 2) % 3)) * 255 / (i / 3 + 1), 100);
		fake_layer->fake_layer_buf[i] =
			mtk_drm_framebuffer_create(crtc->dev, &mode,
						&mtk_gem->base);
	}
	fake_layer->init = true;
	DDPMSG("%s init done\n", __func__);

	return 0;
}

static unsigned long long get_current_time_us(void)
{
	unsigned long long time = sched_clock();
	struct timespec64 t;

	/* return do_div(time,1000); */
	return time;

	ktime_get_ts64(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_nsec / NSEC_PER_USEC;
}

static char *_logger_pr_type_spy(enum DPREC_LOGGER_PR_TYPE type)
{
	switch (type) {
	case DPREC_LOGGER_ERROR:
		return "error";
	case DPREC_LOGGER_FENCE:
		return "fence";
	case DPREC_LOGGER_DEBUG:
		return "dbg";
	case DPREC_LOGGER_DUMP:
		return "dump";
	case DPREC_LOGGER_STATUS:
		return "status";
	default:
		return "unknown";
	}
}

static void init_log_buffer(void)
{
	unsigned long va;
	unsigned long pa;
	unsigned long size;
	int i, buf_size, buf_idx;
	char *temp_buf;

	if (is_buffer_init)
		return;

	/*1. Allocate Error, Fence, Debug and Dump log buffer slot*/
	err_buffer = kzalloc(sizeof(char *) * ERROR_BUFFER_COUNT, GFP_KERNEL);
	if (!err_buffer)
		goto err;
	fence_buffer = kzalloc(sizeof(char *) * FENCE_BUFFER_COUNT, GFP_KERNEL);
	if (!fence_buffer)
		goto err;
	dbg_buffer = kzalloc(sizeof(char *) * DEBUG_BUFFER_COUNT, GFP_KERNEL);
	if (!dbg_buffer)
		goto err;
	dump_buffer = kzalloc(sizeof(char *) * DUMP_BUFFER_COUNT, GFP_KERNEL);
	if (!dump_buffer)
		goto err;
	status_buffer = kzalloc(sizeof(char *) * DUMP_BUFFER_COUNT, GFP_KERNEL);
	if (!status_buffer)
		goto err;

	/*2. Allocate log ring buffer.*/
	buf_size = sizeof(char) * (DEBUG_BUFFER_SIZE - 4096);
	temp_buf = kzalloc(buf_size, GFP_KERNEL);
	if (!temp_buf)
		goto err;

	/*3. Dispatch log ring buffer to each buffer slot*/
	buf_idx = 0;
	for (i = 0; i < ERROR_BUFFER_COUNT; i++) {
		err_buffer[i] = (temp_buf + buf_idx * LOGGER_BUFFER_SIZE);
		buf_idx++;
	}
	dprec_logger_buffer[0].buffer_ptr = err_buffer;

	for (i = 0; i < FENCE_BUFFER_COUNT; i++) {
		fence_buffer[i] = (temp_buf + buf_idx * LOGGER_BUFFER_SIZE);
		buf_idx++;
	}
	dprec_logger_buffer[1].buffer_ptr = fence_buffer;

	for (i = 0; i < DEBUG_BUFFER_COUNT; i++) {
		dbg_buffer[i] = (temp_buf + buf_idx * LOGGER_BUFFER_SIZE);
		buf_idx++;
	}
	dprec_logger_buffer[2].buffer_ptr = dbg_buffer;

	for (i = 0; i < DUMP_BUFFER_COUNT; i++) {
		dump_buffer[i] = (temp_buf + buf_idx * LOGGER_BUFFER_SIZE);
		buf_idx++;
	}
	dprec_logger_buffer[3].buffer_ptr = dump_buffer;

	for (i = 0; i < STATUS_BUFFER_COUNT; i++) {
		status_buffer[i] = (temp_buf + buf_idx * LOGGER_BUFFER_SIZE);
		buf_idx++;
	}
	dprec_logger_buffer[4].buffer_ptr = status_buffer;

	is_buffer_init = true;

	va = (unsigned long)err_buffer[0];
	pa = __pa_nodebug(va);
	size = (DEBUG_BUFFER_SIZE - 4096);

	mrdump_mini_add_extra_file(va, pa, size, "DISPLAY");

	DDPINFO("[DISP]%s success\n", __func__);
	return;
err:
	DDPPR_ERR("[DISP]%s: log buffer allocation fail\n", __func__);
}

static inline spinlock_t *dprec_logger_lock(enum DPREC_LOGGER_PR_TYPE type)
{
	switch (type) {
	case DPREC_LOGGER_ERROR:
		return &dprec_err_logger_spinlock;
	case DPREC_LOGGER_FENCE:
		return &dprec_fence_logger_spinlock;
	case DPREC_LOGGER_DEBUG:
		return &dprec_dbg_logger_spinlock;
	case DPREC_LOGGER_DUMP:
		return &dprec_dump_logger_spinlock;
	case DPREC_LOGGER_STATUS:
		return &dprec_status_logger_spinlock;
	default:
		DDPPR_ERR("invalid logger type\n");
	}
	return NULL;
}

int mtk_dprec_logger_pr(unsigned int type, char *fmt, ...)
{
	int n = 0;
	unsigned long flags = 0;
	uint64_t time = get_current_time_us();
	unsigned long rem_nsec;
	char **buf_arr;
	char *buf = NULL;
	int len = 0;

	if (!logger_enable)
		return -1;

	if (type >= DPREC_LOGGER_PR_NUM)
		return -1;

	if (!is_buffer_init)
		return -1;

	spin_lock_irqsave(dprec_logger_lock(type), flags);
	if (dprec_logger_buffer[type].len < 128) {
		dprec_logger_buffer[type].id++;
		dprec_logger_buffer[type].id = dprec_logger_buffer[type].id %
					       dprec_logger_buffer[type].cnt;
		dprec_logger_buffer[type].len = dprec_logger_buffer[type].size;
	}
	buf_arr = dprec_logger_buffer[type].buffer_ptr;
	buf = buf_arr[dprec_logger_buffer[type].id] +
	      dprec_logger_buffer[type].size - dprec_logger_buffer[type].len;
	len = dprec_logger_buffer[type].len;

	if (buf) {
		va_list args;

		rem_nsec = do_div(time, 1000000000);
		n += snprintf(buf + n, len - n, "[%5lu.%06lu]",
			      (unsigned long)time, rem_nsec / 1000);

		va_start(args, fmt);
		n += vscnprintf(buf + n, len - n, fmt, args);
		va_end(args);
	}

	dprec_logger_buffer[type].len -= n;
	spin_unlock_irqrestore(dprec_logger_lock(type), flags);

	return n;
}
EXPORT_SYMBOL(mtk_dprec_logger_pr);

int mtk_dprec_logger_get_buf(enum DPREC_LOGGER_PR_TYPE type, char *stringbuf,
			     int len)
{
	int n = 0;
	int i;
	char **buf_arr;
	int c;

	if (type < 0) {
		DDPPR_ERR("%s invalid DPREC_LOGGER_PR_TYPE\n", __func__);
		return -1;
	}
	c = dprec_logger_buffer[type].id;

	if (type >= DPREC_LOGGER_PR_NUM || type < 0 || len < 0)
		return 0;

	if (!is_buffer_init)
		return 0;

	buf_arr = dprec_logger_buffer[type].buffer_ptr;

	for (i = 0; i < dprec_logger_buffer[type].cnt; i++) {
		c++;
		c %= dprec_logger_buffer[type].cnt;
		n += scnprintf(stringbuf + n, len - n,
			       "dprec log buffer[%s][%d]\n",
			       _logger_pr_type_spy(type), c);
		n += scnprintf(stringbuf + n, len - n, "%s\n", buf_arr[c]);
	}

	return n;
}

int __mtkfb_set_backlight_level(unsigned int level, unsigned int panel_ext_param,
			       unsigned int cfg_flag, bool group)
{
	struct drm_crtc *crtc;
	int ret = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	/* this debug cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("%s failed to find crtc\n", __func__);
		return -EINVAL;
	}
	if (group == true)
		ret = mtk_drm_setbacklight_grp(crtc, level, panel_ext_param, cfg_flag);
	else
		ret = mtk_drm_setbacklight(crtc, level, panel_ext_param, cfg_flag);

	return ret;
}

int mtkfb_set_backlight_level(unsigned int level, unsigned int panel_ext_param,
				 unsigned int cfg_flag)
{
	return __mtkfb_set_backlight_level(level, panel_ext_param, cfg_flag, false);
}
EXPORT_SYMBOL(mtkfb_set_backlight_level);

int mtk_drm_set_conn_backlight_level(unsigned int conn_id, unsigned int level,
				unsigned int panel_ext_param, unsigned int cfg_flag)
{
	struct drm_crtc *crtc;
	struct drm_connector *conn;
	struct mtk_drm_private *priv;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_dsi *mtk_dsi;
	int ret = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	priv = drm_dev->dev_private;
	if (IS_ERR_OR_NULL(priv)) {
		DDPPR_ERR("%s, invalid priv\n", __func__);
		return -EINVAL;
	}

	/* connector obj ref count add 1 after lookup */
	conn = drm_connector_lookup(drm_dev, NULL, conn_id);
	if (IS_ERR_OR_NULL(conn)) {
		DDPPR_ERR("%s, invalid conn_id %u\n", __func__, conn_id);
		return -EINVAL;
	}

	mtk_dsi = container_of(conn, struct mtk_dsi, conn);

	mutex_lock(&priv->commit.lock);
	mtk_crtc = mtk_dsi->ddp_comp.mtk_crtc;
	crtc = (mtk_crtc) ? &mtk_crtc->base : NULL;

	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("%s, invalid crtc\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ret = mtk_drm_setbacklight(crtc, level, panel_ext_param, cfg_flag);
out:
	drm_connector_put(conn);
	mutex_unlock(&priv->commit.lock);

	return ret;
}
EXPORT_SYMBOL(mtk_drm_set_conn_backlight_level);

int mtk_drm_get_conn_obj_id_from_idx(unsigned int disp_idx, int flag)
{
	struct drm_encoder *encoder;
	unsigned int i = 0;
	int conn_obj_id = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	drm_for_each_encoder(encoder, drm_dev) {
		struct mtk_dsi *mtk_dsi;

		if (encoder->encoder_type != DRM_MODE_ENCODER_DSI)
			continue;

		mtk_dsi = container_of(encoder, struct mtk_dsi, encoder);

		/* there's not strong binding to disp_idx and DSI connector_obj_id */
		if (mtk_dsi && disp_idx == i)
			conn_obj_id = mtk_dsi->conn.base.id;

		++i;
	}

	return conn_obj_id;
}
EXPORT_SYMBOL(mtk_drm_get_conn_obj_id_from_idx);

int mtkfb_set_aod_backlight_level(unsigned int level)
{
	struct drm_crtc *crtc;
	int ret = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	/* this debug cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("%s failed to find crtc\n", __func__);
		return -EINVAL;
	}
	ret = mtk_drm_aod_setbacklight(crtc, level);

	return ret;
}
EXPORT_SYMBOL(mtkfb_set_aod_backlight_level);

void mtk_disp_mipi_ccci_callback(unsigned int en, unsigned int usrdata)
{
	struct drm_crtc *crtc;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);

	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	mtk_crtc_mipi_freq_switch(crtc, en, usrdata);

	return;
}
EXPORT_SYMBOL(mtk_disp_mipi_ccci_callback);

void mtk_disp_osc_ccci_callback(unsigned int en, unsigned int usrdata)
{
	struct drm_crtc *crtc;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);

	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	mtk_crtc_osc_freq_switch(crtc, en, usrdata);

	return;
}
EXPORT_SYMBOL(mtk_disp_osc_ccci_callback);

void display_enter_tui(void)
{
	struct drm_crtc *crtc;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);

	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	mtk_crtc_enter_tui(crtc);
}
EXPORT_SYMBOL(display_enter_tui);


void display_exit_tui(void)
{
	struct drm_crtc *crtc;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);

	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	mtk_crtc_exit_tui(crtc);
}
EXPORT_SYMBOL(display_exit_tui);

static int debug_get_info(unsigned char *stringbuf, int buf_len)
{
	int n = 0;
	struct mtk_drm_private *private;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s:%d, drm_dev is NULL\n",
			__func__, __LINE__);
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(drm_dev->dev_private)) {
		DDPPR_ERR("%s:%d, drm_dev->dev_private is NULL\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	private = drm_dev->dev_private;
#ifdef IF_ZERO
	DISPFUNC();

	n += mtkfb_get_debug_state(stringbuf + n, buf_len - n);

	n += primary_display_get_debug_state(stringbuf + n, buf_len - n);

	n += disp_sync_get_debug_info(stringbuf + n, buf_len - n);

	n += dprec_logger_get_result_string_all(stringbuf + n, buf_len - n);

	n += disp_helper_get_option_list(stringbuf + n, buf_len - n);
#endif
	n += mtk_drm_primary_display_get_debug_state(private, stringbuf + n,
		buf_len - n);

	n += mtk_drm_dump_wk_lock(private, stringbuf + n,
		buf_len - n);

	n += mtk_drm_helper_get_opt_list(private->helper_opt, stringbuf + n,
					 buf_len - n);

	n += mtk_dprec_logger_get_buf(DPREC_LOGGER_ERROR, stringbuf + n,
				      buf_len - n);

	n += mtk_dprec_logger_get_buf(DPREC_LOGGER_FENCE, stringbuf + n,
				      buf_len - n);

	n += mtk_dprec_logger_get_buf(DPREC_LOGGER_DUMP, stringbuf + n,
				      buf_len - n);

	n += mtk_dprec_logger_get_buf(DPREC_LOGGER_DEBUG, stringbuf + n,
				      buf_len - n);

	n += mtk_dprec_logger_get_buf(DPREC_LOGGER_STATUS, stringbuf + n,
				      buf_len - n);

	stringbuf[n++] = 0;
	return n;
}

static void mtk_fake_engine_iommu_enable(struct drm_device *dev,
		unsigned int idx)
{
	int port, ret;
	unsigned int value;
	struct device_node *larb_node;
	void __iomem *baddr;
	struct mtk_drm_private *priv = dev->dev_private;

	/* get larb reg */
	larb_node = of_parse_phandle(priv->mmsys_dev->of_node,
				"fake-engine", idx * 2);
	if (!larb_node) {
		DDPPR_ERR("Cannot find larb node\n");
		return;
	}
	baddr = of_iomap(larb_node, 0);
	of_node_put(larb_node);

	/* get port num */
	ret = of_property_read_u32_index(priv->mmsys_dev->of_node,
				"fake-engine", idx * 2 + 1, &port);
	if (ret < 0) {
		DDPPR_ERR("Node %s cannot find fake-engine data!\n",
			priv->mmsys_dev->of_node->full_name);
		return;
	}
	port &= GET_M4U_PORT;

	value = readl(baddr + SMI_LARB_NON_SEC_CON(port));
	value = (value & ~0x1) | (0x1 & 0x1);
	writel_relaxed(value, baddr + SMI_LARB_NON_SEC_CON(port));
}

static void mtk_fake_engine_share_port_config(struct drm_crtc *crtc,
						unsigned int idx, bool en)
{
	unsigned int value;
	struct device_node *larb_node;
	static void __iomem **baddr;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	const struct mtk_fake_eng_data *fake_eng_data =
						priv->data->fake_eng_data;
	int i;

	if (!baddr) {
		baddr = devm_kmalloc_array(crtc->dev->dev,
				fake_eng_data->fake_eng_num,
				sizeof(void __iomem *),
				GFP_KERNEL);
		if (!baddr)
			return;

		for (i = 0; i < fake_eng_data->fake_eng_num; i++) {
			larb_node = of_parse_phandle(priv->mmsys_dev->of_node,
				"fake-engine", i * 2);
			if (!larb_node) {
				DDPPR_ERR("Cannot find larb node\n");
				return;
			}
			baddr[i] = of_iomap(larb_node, 0);
			of_node_put(larb_node);
		}
	}

	if (en) {
		value = readl(baddr[idx] + SMI_LARB_VC_PRI_MODE);
		value = (value & ~0x3) | (0x0 & 0x3);
		writel_relaxed(value, baddr[idx] + SMI_LARB_VC_PRI_MODE);

		value = readl(mtk_crtc->config_regs +
				DISP_REG_CONFIG_RDMA_SHARE_SRAM_CON);
		value |= DISP_RDMA_FAKE_SMI_SEL(idx);
		writel_relaxed(value, mtk_crtc->config_regs +
				DISP_REG_CONFIG_RDMA_SHARE_SRAM_CON);
	} else {
		value = readl(baddr[idx] + SMI_LARB_VC_PRI_MODE);
		value = (value & ~0x3) | (0x1 & 0x3);
		writel_relaxed(value, baddr[idx] + SMI_LARB_VC_PRI_MODE);

		value = readl(mtk_crtc->config_regs +
				DISP_REG_CONFIG_RDMA_SHARE_SRAM_CON);
		value &= ~(DISP_RDMA_FAKE_SMI_SEL(idx));
		writel_relaxed(value, mtk_crtc->config_regs +
				DISP_REG_CONFIG_RDMA_SHARE_SRAM_CON);
	}
}

void fake_engine(struct drm_crtc *crtc, unsigned int idx, unsigned int en,
		unsigned int wr_en, unsigned int rd_en, unsigned int wr_pat1,
		unsigned int wr_pat2, unsigned int latency,
		unsigned int preultra_cnt,
		unsigned int ultra_cnt)
{
	int burst = 7;
	int test_len = 255;
	int loop = 1;
	int preultra_en = 0;
	int ultra_en = 0;
	int dis_wr = !wr_en;
	int dis_rd = !rd_en;
	int delay_cnt = 0;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_private *priv = crtc->dev->dev_private;
	const struct mtk_fake_eng_data *fake_eng_data;
	const struct mtk_fake_eng_reg *fake_eng;
	static struct mtk_drm_gem_obj **gem;
	int i;

	fake_eng_data = priv->data->fake_eng_data;
	if (!fake_eng_data) {
		DDPPR_ERR("this platform not support any fake engine\n");
		return;
	}

	if (idx > fake_eng_data->fake_eng_num - 1) {
		DDPPR_ERR("this platform not support fake engine %d\n", idx);
		return;
	}

	fake_eng = &fake_eng_data->fake_eng_reg[idx];

	if (preultra_cnt > 0) {
		preultra_en = 1;
		preultra_cnt--;
	}

	if (ultra_cnt > 0) {
		ultra_en = 1;
		ultra_cnt--;
	}

	if (en) {
		if (!gem) {
			gem = devm_kmalloc_array(crtc->dev->dev,
					fake_eng_data->fake_eng_num,
					sizeof(struct mtk_drm_gem_obj *),
					GFP_KERNEL);
			if (!gem)
				return;

			for (i = 0; i < fake_eng_data->fake_eng_num; i++) {
				gem[i] = mtk_drm_gem_create(crtc->dev,
							1024*1024, true);
				mtk_fake_engine_iommu_enable(crtc->dev, i);
				DDPMSG("fake_engine_%d va=0x%08lx, pa=0x%08x\n",
					i, (unsigned long)gem[i]->kvaddr,
					(unsigned int)gem[i]->dma_addr);
			}
		}

		if (fake_eng->share_port)
			mtk_fake_engine_share_port_config(crtc, idx, en);

		writel_relaxed(BIT(fake_eng->CG_bit), mtk_crtc->config_regs +
			DISP_REG_CONFIG_MMSYS_CG_CLR(fake_eng->CG_idx));

		writel_relaxed((unsigned int)gem[idx]->dma_addr,
			mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_RD_ADDR(idx));
		writel_relaxed((unsigned int)gem[idx]->dma_addr + 4096,
			mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_WR_ADDR(idx));
		writel_relaxed((wr_pat1 << 24) | (loop << 22) | test_len,
			mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_CON0(idx));
		writel_relaxed((ultra_en << 23) | (ultra_cnt << 20) |
			(preultra_en << 19) | (preultra_cnt << 16) |
			(burst << 12) | (dis_wr << 11) | (dis_rd << 10) |
			latency, mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_CON1(idx));

		writel_relaxed(1, mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_RST(idx));
		writel_relaxed(0, mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_RST(idx));
		writel_relaxed(0x3, mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_EN(idx));

		if (wr_pat2 != wr_pat1)
			writel_relaxed((wr_pat2 << 24) | (loop << 22) |
				test_len,
				mtk_crtc->config_regs +
				DISP_REG_CONFIG_DISP_FAKE_ENG_CON0(idx));

		DDPMSG("fake_engine_%d enable\n", idx);
	} else {
		writel_relaxed(0x1, mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_EN(idx));

		while ((readl(mtk_crtc->config_regs +
				DISP_REG_CONFIG_DISP_FAKE_ENG_STATE(idx))
				& 0x1) == 0x1) {
			delay_cnt++;
			udelay(1);
			if (delay_cnt > 1000) {
				DDPPR_ERR("Wait fake_engine_%d idle timeout\n",
					idx);
				break;
			}
		}

		writel_relaxed(0x0, mtk_crtc->config_regs +
			DISP_REG_CONFIG_DISP_FAKE_ENG_EN(idx));

		writel_relaxed(BIT(fake_eng->CG_bit), mtk_crtc->config_regs +
			DISP_REG_CONFIG_MMSYS_CG_SET(fake_eng->CG_idx));

		if (fake_eng->share_port)
			mtk_fake_engine_share_port_config(crtc, idx, en);

		DDPMSG("fake_engine_%d disable\n", idx);
	}
}

void dump_fake_engine(void __iomem *config_regs)
{
	DDPDUMP("=================Dump Fake_engine================\n");
		mtk_serial_dump_reg(config_regs, 0x100, 1);
		mtk_serial_dump_reg(config_regs, 0x110, 1);
		mtk_serial_dump_reg(config_regs, 0x200, 4);
		mtk_serial_dump_reg(config_regs, 0x210, 3);
		mtk_serial_dump_reg(config_regs, 0x220, 4);
		mtk_serial_dump_reg(config_regs, 0x230, 3);
}

static void mtk_ddic_send_cb(struct cmdq_cb_data data)
{
	struct mtk_cmdq_cb_data *cb_data = data.data;

	cmdq_pkt_destroy(cb_data->cmdq_handle);
	kfree(cb_data);
	CRTC_MMP_MARK(0, ddic_send_cmd, 1, 1);
}

int mtk_drm_set_frame_skip(bool skip)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (!crtc) {
		pr_info("find crtc fail\n");
		return 0;
	}
	mtk_crtc = to_mtk_crtc(crtc);
	if (skip) {
		mtk_crtc->cust_skip_frame = true;
		pr_info("skip frame set 1\n");
	} else {
		mtk_crtc->cust_skip_frame = false;
		pr_info("skip frame set 0\n");
	}

	return 0;
}
EXPORT_SYMBOL(mtk_drm_set_frame_skip);

void reset_frame_wq(struct frame_condition_wq *wq)
{
	atomic_set(&wq->condition, 0);
}

void wakeup_frame_wq(struct frame_condition_wq *wq)
{
	atomic_set(&wq->condition, 1);
	wake_up(&wq->wq);
}

static int wait_frame_wq(struct frame_condition_wq *wq, int timeout)
{
	int ret;

	ret = wait_event_timeout(wq->wq, atomic_read(&wq->condition), timeout);

	atomic_set(&wq->condition, 0);

	return ret;
}

int wait_frame_condition(enum DISP_FRAME_STATE state, unsigned int timeout)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	unsigned int ret = 0;

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
		typeof(*crtc), head);
	if (!crtc) {
		DDPPR_ERR("crtc error\n");
		return -EINVAL;
	}
	mtk_crtc = to_mtk_crtc(crtc);

	if (state == DISP_FRAME_START) {
		reset_frame_wq(&mtk_crtc->frame_start);
		ret = wait_frame_wq(&mtk_crtc->frame_start, timeout);
	} else if (state == DISP_FRAME_DONE) {
		reset_frame_wq(&mtk_crtc->frame_done);
		ret = wait_frame_wq(&mtk_crtc->frame_done, timeout);
	}

	if (ret)
		DDPMSG("%s %d : success\n", __func__, state);
	else
		DDPMSG("%s %d : timeout\n", __func__, state);
	return ret;
}

int mtk_crtc_lock_control(bool en)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_drm_private *private;

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
		typeof(*crtc), head);

	if (!crtc) {
		DDPPR_ERR("crtc error\n");
		return -EINVAL;
	}

	private = crtc->dev->dev_private;
	mtk_crtc = to_mtk_crtc(crtc);

	if (!private) {
		DDPPR_ERR("private error\n");
		return -EINVAL;
	}

	if (en) {
		mutex_lock(&private->commit.lock);
		DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
#ifndef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
		mtk_crtc->customer_lock_tid = (int) sys_gettid();
#else
		mtk_crtc->customer_lock_tid = current->pid;
#endif
	} else {
		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		mtk_crtc->customer_lock_tid = 0;
		mutex_unlock(&private->commit.lock);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_crtc_lock_control);

int set_lcm_wrapper(struct mtk_ddic_dsi_msg *cmd_msg, int blocking)
{
	int ret = 0;
	int current_tid = 0;
	bool recursive_check = false;
	bool need_lock = false;
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *output_comp;

#ifndef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
	current_tid = sys_gettid();
#else
	current_tid = current->pid;
#endif

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (!crtc) {
		pr_info("find crtc fail\n");
		return -EINVAL;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	if (!mtk_crtc) {
		pr_info("find mtk_crtc fail\n");
		return -EINVAL;
	}

	output_comp = mtk_ddp_comp_request_output(mtk_crtc);
	if (unlikely(!output_comp)) {
		pr_info("%s:invalid output comp\n", __func__);
		return -EINVAL;
	}

	recursive_check = (((int)mtk_crtc->need_lock_tid !=
							(int)current_tid) &&
							(int)current_tid != INIT_TID);

	need_lock = (current_tid != mtk_crtc->customer_lock_tid);

	DDPMSG("%s + %d_%d_%d_%d_%d_%d\n", __func__, current_tid, mtk_crtc->need_lock_tid,
			mtk_crtc->customer_lock_tid, recursive_check,
			need_lock, mtk_crtc->set_lcm_scn);

	if (recursive_check)
		ret = mtk_ddic_dsi_send_cmd(cmd_msg, blocking, need_lock);
	else if (mtk_crtc->set_lcm_scn > SET_LCM_CMDQ_AVAILABLE &&
				mtk_crtc->set_lcm_scn < SET_LCM_CMDQ_FRAME_DONE)
		ret = mtk_ddp_comp_io_cmd(output_comp, NULL, SET_LCM_CMDQ, cmd_msg);
	else if (mtk_crtc->set_lcm_scn > SET_LCM_CMDQ_FRAME_DONE)
		ret = mtk_ddic_dsi_send_cmd(cmd_msg, blocking, FALSE);
	else
		ret = mtk_ddp_comp_io_cmd(output_comp, NULL,
		SET_LCM_DCS_CMD, cmd_msg);
	return ret;
}
EXPORT_SYMBOL(set_lcm_wrapper);

int read_lcm_wrapper(struct mtk_ddic_dsi_msg *cmd_msg)
{
	int ret = 0;
	int current_tid = 0;
	bool recursive_check = false;
	bool need_lock = false;
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *output_comp;

#ifndef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
	current_tid = sys_gettid();
#else
	current_tid = current->pid;
#endif

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (!crtc) {
		pr_info("find crtc fail\n");
		return -EINVAL;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	if (!mtk_crtc) {
		pr_info("find mtk_crtc fail\n");
		return -EINVAL;
	}

	output_comp = mtk_ddp_comp_request_output(mtk_crtc);
	if (unlikely(!output_comp)) {
		pr_info("%s:invalid output comp\n", __func__);
		return -EINVAL;
	}

	recursive_check = (((int)mtk_crtc->need_lock_tid !=
							(int)current_tid) &&
							(int)current_tid != INIT_TID);

	need_lock = (current_tid != mtk_crtc->customer_lock_tid);

	DDPMSG("%s + %d_%d_%d_%d_%d_%d\n", __func__, current_tid, mtk_crtc->need_lock_tid,
			mtk_crtc->customer_lock_tid, recursive_check, need_lock,
			mtk_crtc->set_lcm_scn);

	if (recursive_check)
		ret = mtk_ddic_dsi_read_cmd(cmd_msg, need_lock);
	else if (mtk_crtc->set_lcm_scn > SET_LCM_CMDQ_FRAME_DONE)
		ret = mtk_ddic_dsi_read_cmd(cmd_msg, FALSE);
	else
		ret = mtk_ddp_comp_io_cmd(output_comp, NULL,
		READ_LCM_DCS_CMD, cmd_msg);
	return ret;
}
EXPORT_SYMBOL(read_lcm_wrapper);

int mtk_ddic_dsi_send_cmd(struct mtk_ddic_dsi_msg *cmd_msg,
			int blocking, bool need_lock)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_drm_private *private;
	struct mtk_ddp_comp *output_comp;
	struct cmdq_pkt *cmdq_handle;
	struct cmdq_client *gce_client;
	bool is_frame_mode;
	bool use_lpm = false;
	struct mtk_cmdq_cb_data *cb_data;
	int index = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	DDPMSG("%s +\n", __func__);

	/* This cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return -EINVAL;
	}

	index = drm_crtc_index(crtc);

	CRTC_MMP_EVENT_START(index, ddic_send_cmd, (unsigned long)crtc,
				blocking);

	private = crtc->dev->dev_private;
	mtk_crtc = to_mtk_crtc(crtc);
	if (need_lock) {
		mutex_lock(&private->commit.lock);
		DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	}

	if (!mtk_crtc->enabled) {
		DDPMSG("crtc%d disable skip %s\n",
			drm_crtc_index(&mtk_crtc->base), __func__);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		CRTC_MMP_EVENT_END(index, ddic_send_cmd, 0, 1);
		return -EINVAL;
	} else if (mtk_crtc->ddp_mode == DDP_NO_USE) {
		DDPMSG("skip %s, ddp_mode: NO_USE\n",
			__func__);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		CRTC_MMP_EVENT_END(index, ddic_send_cmd, 0, 2);
		return -EINVAL;
	}

	output_comp = mtk_ddp_comp_request_output(mtk_crtc);
	if (unlikely(!output_comp)) {
		DDPPR_ERR("%s:invalid output comp\n", __func__);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		CRTC_MMP_EVENT_END(index, ddic_send_cmd, 0, 3);
		return -EINVAL;
	}

	is_frame_mode = mtk_crtc_is_frame_trigger_mode(&mtk_crtc->base);
	if (cmd_msg)
		use_lpm = cmd_msg->flags & MIPI_DSI_MSG_USE_LPM;

	CRTC_MMP_MARK(index, ddic_send_cmd, 1, 0);

	/* Kick idle */
	mtk_drm_idlemgr_kick(__func__, crtc, 0);

	CRTC_MMP_MARK(index, ddic_send_cmd, 2, 0);

	/* only use CLIENT_DSI_CFG for VM CMD scenario */
	/* use CLIENT_CFG otherwise */

	gce_client = (!is_frame_mode && !use_lpm &&
				mtk_crtc->gce_obj.client[CLIENT_DSI_CFG]) ?
			mtk_crtc->gce_obj.client[CLIENT_DSI_CFG] :
			mtk_crtc->gce_obj.client[CLIENT_CFG];

	mtk_crtc_pkt_create(&cmdq_handle, crtc, gce_client);

	if (mtk_crtc_with_sub_path(crtc, mtk_crtc->ddp_mode))
		mtk_crtc_wait_frame_done(mtk_crtc, cmdq_handle,
			DDP_SECOND_PATH, 0);
	else
		mtk_crtc_wait_frame_done(mtk_crtc, cmdq_handle,
			DDP_FIRST_PATH, 0);

	if (is_frame_mode) {
		cmdq_pkt_clear_event(cmdq_handle,
			mtk_crtc->gce_obj.event[EVENT_STREAM_BLOCK]);
		cmdq_pkt_wfe(cmdq_handle,
			mtk_crtc->gce_obj.event[EVENT_CABC_EOF]);
	}

	/* DSI_SEND_DDIC_CMD */
	if (output_comp)
		ret = mtk_ddp_comp_io_cmd(output_comp, cmdq_handle,
		DSI_SEND_DDIC_CMD, cmd_msg);

	if (is_frame_mode) {
		cmdq_pkt_set_event(cmdq_handle,
			mtk_crtc->gce_obj.event[EVENT_CABC_EOF]);
		cmdq_pkt_set_event(cmdq_handle,
			mtk_crtc->gce_obj.event[EVENT_STREAM_BLOCK]);
	}

#if defined (CUSTOMER_USE_SIMPLE_API)
	if (blocking == SET_LCM_BLOCKING_NOWAIT) {
		cmdq_pkt_flush_async(cmdq_handle, NULL, NULL);
		mtk_drm_idlemgr_kick(__func__, crtc, 0);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		DDPMSG("%s flush done\n", __func__);
		cmdq_pkt_wait_complete(cmdq_handle);
		cmdq_pkt_destroy(cmdq_handle);

		DDPMSG("%s - %d\n", __func__, blocking);

		CRTC_MMP_EVENT_END(index, ddic_send_cmd, (unsigned long)crtc,
			blocking);

		return ret;
	}
#endif

	if (blocking) {
		cmdq_pkt_flush(cmdq_handle);
		cmdq_pkt_destroy(cmdq_handle);
	} else {
		cb_data = kmalloc(sizeof(*cb_data), GFP_KERNEL);
		if (!cb_data) {
			DDPPR_ERR("%s:cb data creation failed\n", __func__);
			if (need_lock) {
				DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
				mutex_unlock(&private->commit.lock);
			}
			CRTC_MMP_EVENT_END(index, ddic_send_cmd, 0, 4);
			return -EINVAL;
		}

		cb_data->cmdq_handle = cmdq_handle;
		cmdq_pkt_flush_threaded(cmdq_handle, mtk_ddic_send_cb, cb_data);
	}
	DDPMSG("%s -\n", __func__);
	if (need_lock) {
		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		mutex_unlock(&private->commit.lock);
	}
	CRTC_MMP_EVENT_END(index, ddic_send_cmd, (unsigned long)crtc,
			blocking);

	return ret;
}

static void set_cwb_info_buffer(struct drm_crtc *crtc, int format)
{

	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_cwb_info *cwb_info = mtk_crtc->cwb_info;
	struct drm_mode_fb_cmd2 mode = {0};
	struct mtk_drm_gem_obj *mtk_gem;
	u32 color_format = DRM_FORMAT_RGB888;
	int Bpp;

	/*alloc && config two fb if WDMA after PQ, use width height affcted by resolution switch*/
	mtk_crtc_set_width_height(&mode.width, &mode.height,
		crtc, (cwb_info->scn == WDMA_WRITE_BACK));

	if (format == 0)
		color_format = DRM_FORMAT_RGB888;
	else if (format == 1)
		color_format = DRM_FORMAT_ARGB2101010;

	mode.pixel_format = color_format;
	Bpp = mtk_get_format_bpp(mode.pixel_format);
	mode.pitches[0] = mode.width * Bpp;

	mtk_gem = mtk_drm_gem_create(
		crtc->dev, mode.pitches[0] * mode.height, true);
	cwb_info->buffer[0].addr_mva = mtk_gem->dma_addr;
	cwb_info->buffer[0].addr_va = (u64)mtk_gem->kvaddr;

	cwb_info->buffer[0].fb  =
		mtk_drm_framebuffer_create(
		crtc->dev, &mode, &mtk_gem->base);
	DDPMSG("[capture] b[0].addr_mva:0x%x, addr_va:0x%llx\n",
			cwb_info->buffer[0].addr_mva,
			cwb_info->buffer[0].addr_va);

	mtk_gem = mtk_drm_gem_create(
		crtc->dev, mode.pitches[0] * mode.height, true);
	cwb_info->buffer[1].addr_mva = mtk_gem->dma_addr;
	cwb_info->buffer[1].addr_va = (u64)mtk_gem->kvaddr;

	cwb_info->buffer[1].fb  =
		mtk_drm_framebuffer_create(
		crtc->dev, &mode, &mtk_gem->base);
	DDPMSG("[capture] b[1].addr_mva:0x%x, addr_va:0x%llx\n",
			cwb_info->buffer[1].addr_mva,
			cwb_info->buffer[1].addr_va);
}

int mtk_ddic_dsi_read_cmd(struct mtk_ddic_dsi_msg *cmd_msg, bool need_lock)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_drm_private *private;
	struct mtk_ddp_comp *output_comp;
	int index = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	DDPMSG("%s +\n", __func__);

	/* This cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return -EINVAL;
	}

	index = drm_crtc_index(crtc);

	CRTC_MMP_EVENT_START(index, ddic_read_cmd, (unsigned long)crtc, 0);

	private = crtc->dev->dev_private;
	mtk_crtc = to_mtk_crtc(crtc);

	if (need_lock) {
		mutex_lock(&private->commit.lock);
		DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	}
	if (!mtk_crtc->enabled) {
		DDPMSG("crtc%d disable skip %s\n",
			drm_crtc_index(&mtk_crtc->base), __func__);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		CRTC_MMP_EVENT_END(index, ddic_read_cmd, 0, 1);
		return -EINVAL;
	} else if (mtk_crtc->ddp_mode == DDP_NO_USE) {
		DDPMSG("skip %s, ddp_mode: NO_USE\n",
			__func__);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		CRTC_MMP_EVENT_END(index, ddic_read_cmd, 0, 2);
		return -EINVAL;
	}

	output_comp = mtk_ddp_comp_request_output(mtk_crtc);
	if (unlikely(!output_comp)) {
		DDPPR_ERR("%s:invalid output comp\n", __func__);
		if (need_lock) {
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
			mutex_unlock(&private->commit.lock);
		}
		CRTC_MMP_EVENT_END(index, ddic_read_cmd, 0, 3);
		return -EINVAL;
	}

	CRTC_MMP_MARK(index, ddic_read_cmd, 1, 0);

	/* Kick idle */
	mtk_drm_idlemgr_kick(__func__, crtc, 0);

	CRTC_MMP_MARK(index, ddic_read_cmd, 2, 0);

	/* DSI_READ_DDIC_CMD */
	if (output_comp)
		ret = mtk_ddp_comp_io_cmd(output_comp, NULL, DSI_READ_DDIC_CMD,
				cmd_msg);

	CRTC_MMP_MARK(index, ddic_read_cmd, 3, 0);

	DDPMSG("%s -\n", __func__);
	if (need_lock) {
		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		mutex_unlock(&private->commit.lock);
	}
	CRTC_MMP_EVENT_END(index, ddic_read_cmd, (unsigned long)crtc, 4);

	return ret;
}

void ddic_dsi_send_cmd_test(unsigned int case_num)
{
	unsigned int i = 0, j = 0;
	int ret;
	struct mtk_ddic_dsi_msg *cmd_msg =
		vmalloc(sizeof(struct mtk_ddic_dsi_msg));
	u8 tx[10] = {0};
	u8 tx_1[10] = {0};

	DDPMSG("%s start case_num:%d\n", __func__, case_num);

	if (!cmd_msg) {
		DDPPR_ERR("cmd msg is NULL\n");
		return;
	}
	memset(cmd_msg, 0, sizeof(struct mtk_ddic_dsi_msg));

	switch (case_num) {
	case 1:
	{
		/* Send 0x34 */
		cmd_msg->channel = 0;
		cmd_msg->flags = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x05;
		tx[0] = 0x34;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		break;
	}
	case 2:
	{
		/* Send 0x35:0x00 */
		cmd_msg->channel = 0;
		cmd_msg->flags = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x15;
		tx[0] = 0x35;
		tx[1] = 0x00;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 2;

		break;
	}
	case 3:
	{
		/* Send 0x28 */
		cmd_msg->channel = 0;
		cmd_msg->flags |= MIPI_DSI_MSG_USE_LPM;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x05;
		tx[0] = 0x28;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		break;
	}
	case 4:
	{
		/* Send 0x29 */
		cmd_msg->channel = 0;
		cmd_msg->flags |= MIPI_DSI_MSG_USE_LPM;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x05;
		tx[0] = 0x29;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		break;
	}
	case 5:
	{
		/* Multiple cmd UT case */
		cmd_msg->channel = 0;
		cmd_msg->flags = 0;
		/*	cmd_msg->flags |= MIPI_DSI_MSG_USE_LPM; */
		cmd_msg->tx_cmd_num = 2;

		/* Send 0x34 */
		cmd_msg->type[0] = 0x05;
		tx[0] = 0x34;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		/* Send 0x28 */
		cmd_msg->type[1] = 0x05;
		tx_1[0] = 0x28;
		cmd_msg->tx_buf[1] = tx_1;
		cmd_msg->tx_len[1] = 1;

		break;
	}
	case 6:
	{
		/* Multiple cmd UT case */
		cmd_msg->channel = 0;
		cmd_msg->flags = 0;
		/*	cmd_msg->flags |= MIPI_DSI_MSG_USE_LPM; */
		cmd_msg->tx_cmd_num = 2;

		/* Send 0x35 */
		cmd_msg->type[0] = 0x15;
		tx[0] = 0x35;
		tx[1] = 0x00;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 2;

		/* Send 0x29 */
		cmd_msg->type[1] = 0x05;
		tx_1[0] = 0x29;
		cmd_msg->tx_buf[1] = tx_1;
		cmd_msg->tx_len[1] = 1;

		break;
	}
	default:
		DDPMSG("%s no this test case:%d\n", __func__, case_num);
		break;
	}

	DDPMSG("send lcm tx_cmd_num:%d\n", (int)cmd_msg->tx_cmd_num);
	for (i = 0; i < (int)cmd_msg->tx_cmd_num; i++) {
		DDPMSG("send lcm tx_len[%d]=%d\n",
			i, (int)cmd_msg->tx_len[i]);
		for (j = 0; j < (int)cmd_msg->tx_len[i]; j++) {
			DDPMSG(
				"send lcm type[%d]=0x%x, tx_buf[%d]--byte:%d,val:0x%x\n",
				i, cmd_msg->type[i], i, j,
				*(char *)(cmd_msg->tx_buf[i] + j));
		}
	}

	ret = mtk_ddic_dsi_send_cmd(cmd_msg, true, true);
	if (ret != 0) {
		DDPPR_ERR("mtk_ddic_dsi_send_cmd error\n");
		goto  done;
	}
done:
	vfree(cmd_msg);

	DDPMSG("%s end -\n", __func__);
}

void ddic_dsi_send_switch_pgt(unsigned int cmd_num, u8 addr,
	u8 val1, u8 val2, u8 val3, u8 val4, u8 val5, u8 val6)
{
	unsigned int i = 0, j = 0;
	int ret;
	struct mtk_ddic_dsi_msg *cmd_msg =
		vmalloc(sizeof(struct mtk_ddic_dsi_msg));
	u8 tx[10] = {0};

	DDPMSG("%s start case_num:%d\n", __func__, val3);

	if (!cmd_msg) {
		DDPPR_ERR("cmd msg is NULL\n");
		return;
	}

	if (!cmd_num)
		return;
	memset(cmd_msg, 0, sizeof(struct mtk_ddic_dsi_msg));

	switch (cmd_num) {
	case 1:
		cmd_msg->type[0] = 0x05;
		break;
	case 2:
		cmd_msg->type[0] = 0x15;
		break;
	default:
		cmd_msg->type[0] = 0x39;
		break;
	}

	/* Send 0x35:0x00 */
	cmd_msg->channel = 0;
	cmd_msg->flags |= MIPI_DSI_MSG_USE_LPM;
	cmd_msg->tx_cmd_num = 1;
	tx[0] = addr;//0xFF;
	tx[1] = val1;//0x78;
	tx[2] = val2;//0x35;
	tx[3] = val3;
	tx[4] = val4;
	tx[5] = val5;
	tx[6] = val6;
	cmd_msg->tx_buf[0] = tx;
	cmd_msg->tx_len[0] = cmd_num;

	DDPMSG("send lcm tx_cmd_num:%d\n", (int)cmd_msg->tx_cmd_num);
	for (i = 0; i < (int)cmd_msg->tx_cmd_num; i++) {
		DDPMSG("send lcm tx_len[%d]=%d\n",
			i, (int)cmd_msg->tx_len[i]);
		for (j = 0; j < (int)cmd_msg->tx_len[i]; j++) {
			DDPMSG(
				"send lcm type[%d]=0x%x, tx_buf[%d]--byte:%d,val:0x%x\n",
				i, cmd_msg->type[i], i, j,
				*(char *)(cmd_msg->tx_buf[i] + j));
		}
	}

	ret = mtk_ddic_dsi_send_cmd(cmd_msg, true, true);
	if (ret != 0) {
		DDPPR_ERR("mtk_ddic_dsi_send_cmd error\n");
		goto  done;
	}
done:
	vfree(cmd_msg);

	DDPMSG("%s end -\n", __func__);
}

void ddic_dsi_read_cm_cmd(u8 cm_addr)
{
	unsigned int i = 0, j = 0;
	unsigned int ret_dlen = 0;
	int ret;
	struct mtk_ddic_dsi_msg *cmd_msg =
		vmalloc(sizeof(struct mtk_ddic_dsi_msg));
	u8 tx[10] = {0};

	if (!cmd_msg)
		return;

	DDPMSG("%s start case_num:%d\n", __func__, cm_addr);

	memset(cmd_msg, 0, sizeof(struct mtk_ddic_dsi_msg));

	/* Read 0x0A = 0x1C */
	cmd_msg->channel = 0;
	cmd_msg->tx_cmd_num = 1;
	cmd_msg->type[0] = 0x06;
	tx[0] = cm_addr;
	cmd_msg->tx_buf[0] = tx;
	cmd_msg->tx_len[0] = 1;

	cmd_msg->rx_cmd_num = 1;
	cmd_msg->rx_buf[0] = kmalloc(4 * sizeof(unsigned char),
			GFP_ATOMIC);
	memset(cmd_msg->rx_buf[0], 0, 4);
	cmd_msg->rx_len[0] = 1;

	ret = mtk_ddic_dsi_read_cmd(cmd_msg, true);
	if (ret != 0) {
		DDPPR_ERR("%s error\n", __func__);
		goto  done;
	}

	for (i = 0; i < cmd_msg->rx_cmd_num; i++) {
		ret_dlen = cmd_msg->rx_len[i];
		DDPMSG("read lcm addr:0x%x--dlen:%d--cmd_idx:%d\n",
			*(char *)(cmd_msg->tx_buf[i]), ret_dlen, i);
		for (j = 0; j < ret_dlen; j++) {
			DDPMSG("read lcm addr:0x%x--byte:%d,val:0x%x\n",
				*(char *)(cmd_msg->tx_buf[i]), j,
				*(char *)(cmd_msg->rx_buf[i] + j));
		}
	}

done:
	for (i = 0; i < cmd_msg->rx_cmd_num; i++)
		kfree(cmd_msg->rx_buf[i]);
	vfree(cmd_msg);

	DDPMSG("%s end -\n", __func__);
}

void ddic_dsi_read_cmd_test(unsigned int case_num)
{
	unsigned int j = 0;
	unsigned int ret_dlen = 0;
	int ret;
	struct mtk_ddic_dsi_msg *cmd_msg =
		vmalloc(sizeof(struct mtk_ddic_dsi_msg));
	u8 tx[10] = {0};

	DDPMSG("%s start case_num:%d\n", __func__, case_num);

	if (!cmd_msg) {
		DDPPR_ERR("cmd msg is NULL\n");
		return;
	}
	memset(cmd_msg, 0, sizeof(struct mtk_ddic_dsi_msg));

	switch (case_num) {
	case 1:
	{
		/* Read 0x0A = 0x1C */
		cmd_msg->channel = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x06;
		tx[0] = 0x0A;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		cmd_msg->rx_cmd_num = 1;
		cmd_msg->rx_buf[0] = vmalloc(4 * sizeof(unsigned char));
		memset(cmd_msg->rx_buf[0], 0, 4);
		cmd_msg->rx_len[0] = 1;

		break;
	}
	case 2:
	{
		/* Read 0xe8 = 0x00,0x01,0x23,0x00 */
		cmd_msg->channel = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x06;
		tx[0] = 0xe8;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		cmd_msg->rx_cmd_num = 1;
		cmd_msg->rx_buf[0] = vmalloc(8 * sizeof(unsigned char));
		memset(cmd_msg->rx_buf[0], 0, 4);
		cmd_msg->rx_len[0] = 4;

		break;
	}
	case 3:
	{
/*
 * Read 0xb6 =
 *	0x30,0x6b,0x00,0x06,0x03,0x0A,0x13,0x1A,0x6C,0x18
 */
		cmd_msg->channel = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x06;
		tx[0] = 0xb6;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		cmd_msg->rx_cmd_num = 1;
		cmd_msg->rx_buf[0] = vmalloc(20 * sizeof(unsigned char));
		memset(cmd_msg->rx_buf[0], 0, 20);
		cmd_msg->rx_len[0] = 10;

		break;
	}
	case 4:
	{
		/* Read 0x0e = 0x80 */
		cmd_msg->channel = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x06;
		tx[0] = 0x0e;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		cmd_msg->rx_cmd_num = 1;
		cmd_msg->rx_buf[0] = vmalloc(4 * sizeof(unsigned char));
		memset(cmd_msg->rx_buf[0], 0, 4);
		cmd_msg->rx_len[0] = 1;

		break;
	}
	case 5:
	{
		/* Read 0xe8 = 0x00,0x01,0x23,0x00 */
		cmd_msg->channel = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x06;
		tx[0] = 0x83;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		cmd_msg->rx_cmd_num = 1;
		cmd_msg->rx_buf[0] = vmalloc(8 * sizeof(unsigned char));
		memset(cmd_msg->rx_buf[0], 0, 4);
		cmd_msg->rx_len[0] = 4;

		break;
	}
	case 6:
	{
		/* Read 0xe8 = 0x00,0x01,0x23,0x00 */
		cmd_msg->channel = 0;
		cmd_msg->tx_cmd_num = 1;
		cmd_msg->type[0] = 0x06;
		tx[0] = 0x51;
		cmd_msg->tx_buf[0] = tx;
		cmd_msg->tx_len[0] = 1;

		cmd_msg->rx_cmd_num = 1;
		cmd_msg->rx_buf[0] = vmalloc(8 * sizeof(unsigned char));
		memset(cmd_msg->rx_buf[0], 0, 4);
		cmd_msg->rx_len[0] = 4;

		break;
	}

	default:
		DDPMSG("%s no this test case:%d\n", __func__, case_num);
		break;
	}

	ret = mtk_ddic_dsi_read_cmd(cmd_msg, true);
	if (ret != 0) {
		DDPPR_ERR("%s error\n", __func__);
		goto  done;
	}

	ret_dlen = cmd_msg->rx_len[0];
	DDPMSG("read lcm addr:0x%x--dlen:%d\n",
		*(char *)(cmd_msg->tx_buf[0]), ret_dlen);
	for (j = 0; j < ret_dlen; j++) {
		DDPMSG("read lcm addr:0x%x--byte:%d,val:0x%x\n",
			*(char *)(cmd_msg->tx_buf[0]), j,
			*(char *)(cmd_msg->rx_buf[0] + j));
	}

done:
	vfree(cmd_msg->rx_buf[0]);
	vfree(cmd_msg);

	DDPMSG("%s end -\n", __func__);
}

#if IS_ENABLED(CONFIG_DRM_PANEL_MCD_COMMON)
int mtk_drm_wait_one_vblank(void)
{
	struct drm_crtc *crtc;
	int ret;

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (!crtc) {
		DDPPR_ERR("find crtc fail\n");
		return -EINVAL;
	}

	ret = drm_crtc_vblank_get(crtc);
	if (ret < 0) {
		pr_err("%s: failed to get vblank\n", __func__);
		return -EINVAL;
	}
	drm_crtc_wait_one_vblank(crtc);
	drm_crtc_vblank_put(crtc);

	return 0;
}
EXPORT_SYMBOL(mtk_drm_wait_one_vblank);
#endif


int mtk_dprec_mmp_dump_ovl_layer(struct mtk_plane_state *plane_state)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_crtc_ddp_ctx *ddp_ctx;
	struct mtk_ddp_comp *comp;
	int global_lye_num;

	if (!gCaptureOVLEn)
		return -1;

	crtc = plane_state->crtc;
	mtk_crtc = to_mtk_crtc(crtc);
	ddp_ctx = mtk_crtc->ddp_ctx;
	if (ddp_ctx[mtk_crtc->ddp_mode].ovl_comp_nr[0] != 0)
		comp = ddp_ctx[mtk_crtc->ddp_mode].ovl_comp[0][0];
	else
		comp = ddp_ctx[mtk_crtc->ddp_mode].ddp_comp[0][0];
	global_lye_num = plane_state->comp_state.lye_id;
	if (mtk_ddp_comp_get_type(comp->id) == MTK_DISP_OVL) {
		if (plane_state->comp_state.comp_id != comp->id)
			global_lye_num += mtk_ovl_layer_num(comp);
	}

	if ((gCaptureAssignLayer != global_lye_num) && (gCaptureAssignLayer != -1))
		return -1;

	mtk_drm_mmp_ovl_layer(plane_state, gCapturePriLayerDownX,
			gCapturePriLayerDownY, global_lye_num);

	DDPINFO("%s, gCapturePriLayerEnable is %d\n",
		__func__, gCaptureOVLEn);
	return 0;
}

int mtk_dprec_mmp_dump_wdma_layer(struct drm_crtc *crtc,
	struct drm_framebuffer *wb_fb)
{
	if (!gCaptureWDMAEn)
		return -1;

	if (mtk_drm_fb_is_secure(wb_fb)) {
		DDPINFO("%s, wb_fb is secure\n", __func__);
		return -1;
	}

	mtk_drm_mmp_wdma_buffer(crtc, wb_fb,
		gCaptureOutLayerDownX, gCaptureOutLayerDownY);

	DDPINFO("%s, gCaptureOutLayerEnable is %d\n",
		__func__, gCaptureWDMAEn);
	return 0;
}

int mtk_dprec_mmp_dump_cwb_buffer(struct drm_crtc *crtc,
		void *buffer, unsigned int buf_idx)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if (gCaptureWDMAEn && mtk_crtc->cwb_info) {
		mtk_drm_mmp_cwb_buffer(crtc, mtk_crtc->cwb_info,
					buffer, buf_idx);
		return 0;
	}
	DDPDBG("%s, gCaptureWDMAEn is %d\n",
		__func__, gCaptureWDMAEn);
	return -1;
}

static void user_copy_done_function(void *buffer,
	enum CWB_BUFFER_TYPE type)
{
	DDPMSG("[capture] I get buffer:0x%x, type:%d\n",
			buffer, type);
	complete(&cwb_cmp);
}

static const struct mtk_cwb_funcs user_cwb_funcs = {
	.copy_done = user_copy_done_function,
};

static void mtk_drm_cwb_info_init(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	int crtc_idx = drm_crtc_index(&mtk_crtc->base);
	struct mtk_drm_private *priv = mtk_crtc->base.dev->dev_private;
	struct mtk_cwb_info *cwb_info = mtk_crtc->cwb_info;

	if (!cwb_info) {
		DDPPR_ERR("%s: cwb_info not found\n", __func__);
		return;
	} else if (!cwb_info->enable)
		return;

	cwb_info->count = 0;

	if (cwb_info->scn == NONE)
		cwb_info->scn = WDMA_WRITE_BACK;

	/* Check if wdith height size will be affect by resolution switch */
	mtk_crtc_set_width_height(&(cwb_info->src_roi.width), &(cwb_info->src_roi.height),
		crtc, (cwb_info->scn == WDMA_WRITE_BACK));

	if (crtc_idx == 0) {
		if (cwb_info->scn == WDMA_WRITE_BACK)
			cwb_info->comp = priv->ddp_comp[DDP_COMPONENT_WDMA0];
		else if (priv->data->mmsys_id == MMSYS_MT6985
			&& cwb_info->scn == WDMA_WRITE_BACK_OVL)
			cwb_info->comp = priv->ddp_comp[DDP_COMPONENT_OVLSYS_WDMA1];
	}

	if (!cwb_info->buffer[0].dst_roi.width ||
		!cwb_info->buffer[0].dst_roi.height) {
		mtk_rect_make(&cwb_info->buffer[0].dst_roi, 0, 0,
			crtc->state->adjusted_mode.hdisplay,
			crtc->state->adjusted_mode.hdisplay);
		mtk_rect_make(&cwb_info->buffer[1].dst_roi, 0, 0,
			crtc->state->adjusted_mode.hdisplay,
			crtc->state->adjusted_mode.hdisplay);
	}

	/*alloc && config two fb*/
	if (!cwb_info->buffer[0].fb)
		set_cwb_info_buffer(crtc, 0);

	DDPMSG("[capture] enable capture, roi:(%d,%d,%d,%d)\n",
		cwb_info->buffer[0].dst_roi.x,
		cwb_info->buffer[0].dst_roi.y,
		cwb_info->buffer[0].dst_roi.width,
		cwb_info->buffer[0].dst_roi.height);
}

bool mtk_drm_cwb_enable(int en,
			const struct mtk_cwb_funcs *funcs,
			enum CWB_BUFFER_TYPE type)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_cwb_info *cwb_info;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return false;
	}
	mtk_crtc = to_mtk_crtc(crtc);

	if (!mtk_crtc->cwb_info) {
		mtk_crtc->cwb_info = kzalloc(sizeof(struct mtk_cwb_info),
			GFP_KERNEL);
		DDPMSG("%s: need allocate memory\n", __func__);
	}
	if (!mtk_crtc->cwb_info) {
		DDPPR_ERR("%s: allocate memory fail\n", __func__);
		return false;
	}

	cwb_info = mtk_crtc->cwb_info;
	if (cwb_info->enable == en) {
		DDPMSG("[capture] en:%d already effective\n", en);
		return true;
	}
	cwb_info->funcs = funcs;
	cwb_info->type = type;

	DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	cwb_info->enable = en;
	if (en)
		mtk_drm_cwb_info_init(crtc);
	else
		DDPMSG("[capture] disable capture");
	DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);

	return true;
}

bool mtk_drm_set_cwb_roi(struct mtk_rect rect)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_cwb_info *cwb_info;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return false;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	if (!mtk_crtc->cwb_info) {
		mtk_crtc->cwb_info = kzalloc(sizeof(struct mtk_cwb_info),
			GFP_KERNEL);
			DDPMSG("%s: need allocate memory\n", __func__);
	}
	if (!mtk_crtc->cwb_info) {
		DDPPR_ERR("%s: allocate memory fail\n", __func__);
		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		return false;
	}
	cwb_info = mtk_crtc->cwb_info;

	if (cwb_info->scn == NONE)
		cwb_info->scn = WDMA_WRITE_BACK;

	/* Check if wdith height size will be affect by resolution switch */
	mtk_crtc_set_width_height(&(cwb_info->src_roi.width), &(cwb_info->src_roi.height),
		crtc, (cwb_info->scn == WDMA_WRITE_BACK));

	if (rect.x >= cwb_info->src_roi.width ||
		rect.y >= cwb_info->src_roi.height ||
		!rect.width || !rect.height) {
		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		return false;
	}

	if (rect.x + rect.width > cwb_info->src_roi.width)
		rect.width = cwb_info->src_roi.width - rect.x;
	if (rect.y + rect.height > cwb_info->src_roi.height)
		rect.height = cwb_info->src_roi.height - rect.y;

	if (!cwb_info->buffer[0].fb)
		set_cwb_info_buffer(crtc, 0);

	/* update roi */
	mtk_rect_make(&cwb_info->buffer[0].dst_roi,
		rect.x, rect.y, rect.width, rect.height);
	mtk_rect_make(&cwb_info->buffer[1].dst_roi,
		rect.x, rect.y, rect.width, rect.height);

	DDPMSG("[capture] change roi:(%d,%d,%d,%d)\n",
		cwb_info->buffer[0].dst_roi.x,
		cwb_info->buffer[0].dst_roi.y,
		cwb_info->buffer[0].dst_roi.width,
		cwb_info->buffer[0].dst_roi.height);

	DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
	return true;

}

void mtk_wakeup_pf_wq(unsigned int m_id)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	unsigned int pf_idx;
	unsigned int crtc_idx;
	struct mtk_drm_private *drm_priv;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	if (m_id == 3) {
		drm_for_each_crtc(crtc, drm_dev)
			if (drm_crtc_index(crtc) == 3)
				break;
	} else {
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
	}

	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	crtc_idx = drm_crtc_index(crtc);
	mtk_crtc = to_mtk_crtc(crtc);

	if (!mtk_crtc || !mtk_crtc->base.dev) {
		DDPPR_ERR("%s errors with NULL mtk_crtc or base.dev\n",
			__func__);
		return;
	}

	drm_priv = mtk_crtc->base.dev->dev_private;

	if (drm_priv &&
		mtk_crtc_is_frame_trigger_mode(&mtk_crtc->base)) {
		pf_idx = readl(mtk_get_gce_backup_slot_va(mtk_crtc,
			DISP_SLOT_PRESENT_FENCE(crtc_idx)));
		atomic_set(&drm_priv->crtc_rel_present[crtc_idx], pf_idx);

		atomic_set(&mtk_crtc->pf_event, 1);
		wake_up_interruptible(&mtk_crtc->present_fence_wq);
	}
}

void mtk_drm_cwb_backup_copy_size(void)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_cwb_info *cwb_info;
	struct mtk_ddp_comp *comp;
	int left_w = 0;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	mtk_crtc = to_mtk_crtc(crtc);
	cwb_info = mtk_crtc->cwb_info;

	if (!cwb_info)
		return;

	if (!cwb_info->comp) {
		DDPPR_ERR("[capture] cwb enable, but has not comp\n");
		return;
	}

	comp = cwb_info->comp;
	mtk_ddp_comp_io_cmd(comp, NULL, WDMA_READ_DST_SIZE, cwb_info);
	if (mtk_crtc->is_dual_pipe) {
		struct mtk_drm_private *priv = mtk_crtc->base.dev->dev_private;

		left_w = cwb_info->copy_w;
		comp = priv->ddp_comp
				[dual_pipe_comp_mapping(priv->data->mmsys_id, comp->id)];
		mtk_ddp_comp_io_cmd(comp, NULL, WDMA_READ_DST_SIZE, cwb_info);
		cwb_info->copy_w += left_w;
	}
}

bool mtk_drm_set_cwb_user_buf(void *user_buffer, enum CWB_BUFFER_TYPE type)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_cwb_info *cwb_info;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	/* this debug cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return false;
	}
	mtk_crtc = to_mtk_crtc(crtc);
	cwb_info = mtk_crtc->cwb_info;

	if (!cwb_info)
		return false;

	DDP_MUTEX_LOCK(&mtk_crtc->cwb_lock, __func__, __LINE__);
	cwb_info->type = type;
	cwb_info->user_buffer = user_buffer;
	DDP_MUTEX_UNLOCK(&mtk_crtc->cwb_lock, __func__, __LINE__);
	DDPMSG("[capture] User set buffer:0x%x, type:%d\n",
			user_buffer, type);

	return true;
}

static void mtk_crtc_set_cm_tune_para(
	unsigned int en, unsigned int cm_c00, unsigned char cm_c01,
	unsigned int cm_c02, unsigned int cm_c10, unsigned char cm_c11,
	unsigned int cm_c12, unsigned int cm_c20, unsigned char cm_c21,
	unsigned int cm_c22)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_panel_cm_params *cm_tune_params;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	if (!mtk_crtc->panel_cm_params) {
		mtk_crtc->panel_cm_params = kzalloc(sizeof(struct mtk_panel_cm_params),
			GFP_KERNEL);
		DDPMSG("%s: need allocate memory\n", __func__);
	}
	if (!mtk_crtc->panel_cm_params) {
		DDPPR_ERR("%s: allocate memory fail\n", __func__);
		return;
	}

	cm_tune_params = mtk_crtc->panel_cm_params;
	cm_tune_params->enable = en;
	cm_tune_params->cm_c00 = cm_c00;
	cm_tune_params->cm_c01 = cm_c01;
	cm_tune_params->cm_c02 = cm_c02;
	cm_tune_params->cm_c10 = cm_c10;
	cm_tune_params->cm_c11 = cm_c11;
	cm_tune_params->cm_c12 = cm_c12;
	cm_tune_params->cm_c20 = cm_c20;
	cm_tune_params->cm_c21 = cm_c21;
	cm_tune_params->cm_c22 = cm_c22;

	DDPINFO("%s,cm_matrix:0x%x count:%d\n", __func__, en,
			cm_c00);

	//DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	//DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
}

bool mtk_crtc_spr_tune_enable(
	unsigned int en)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_panel_spr_params *spr_params;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return false;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	if (!mtk_crtc->panel_spr_params) {
		mtk_crtc->panel_spr_params = kzalloc(sizeof(struct mtk_panel_spr_params),
			GFP_KERNEL);
		DDPMSG("%s: need allocate memory\n", __func__);
	}
	if (!mtk_crtc->panel_spr_params) {
		DDPPR_ERR("%s: allocate memory fail\n", __func__);
		return false;
	}

	spr_params = mtk_crtc->panel_spr_params;

	DDPINFO("%s,spr_tune_en:%d\n", __func__, en);

	//DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	spr_params->enable = en;
	return true;
	//DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
}

static void mtk_crtc_set_spr_tune_para(
	unsigned int color_type, unsigned int count, unsigned char para_list)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_panel_spr_params *spr_params;
	struct spr_color_params *spr_tune_params;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
			typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	if (color_type >= SPR_COLOR_PARAMS_TYPE_NUM) {
		DDPMSG("color_type:%d do not support\n", color_type);
		return;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	if (!mtk_crtc->panel_spr_params) {
		mtk_crtc->panel_spr_params = kzalloc(sizeof(struct mtk_panel_spr_params),
			GFP_KERNEL);
		DDPMSG("%s: need allocate memory\n", __func__);
	}
	if (!mtk_crtc->panel_spr_params) {
		DDPPR_ERR("%s: allocate memory fail\n", __func__);
		return;
	}

	spr_params = mtk_crtc->panel_spr_params;
	spr_tune_params = &spr_params->spr_color_params[color_type];
	if (!spr_tune_params) {
		spr_tune_params = kzalloc(sizeof(struct spr_color_params),
			GFP_KERNEL);
		DDPMSG("%s: need allocate memory\n", __func__);
	}
	if (!spr_tune_params) {
		DDPPR_ERR("%s: allocate memory fail\n", __func__);
		return;
	}
	if (count >= (ARRAY_SIZE(spr_tune_params->para_list))) {
		DDPMSG("count:%d do not support\n", count);
		return;
	}

	spr_tune_params->tune_list[count] = 1;
	spr_tune_params->para_list[count] = para_list;
	spr_tune_params->count = 1;

	DDPINFO("%s,spr_set:0x%x count:%d\n", __func__, para_list,
			count);

	//DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
	//DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
}

int mtk_drm_ioctl_pq_get_persist_property(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int i, ret = 0;
	unsigned int pq_persist_property[32];
	struct mtk_drm_private *private = dev->dev_private;
	struct drm_crtc *crtc = private->crtc[0];

	memset(pq_persist_property, 0, sizeof(pq_persist_property));
	memcpy(pq_persist_property, (unsigned int *)data, sizeof(pq_persist_property));

	for (i = 0; i < DISP_PQ_PROPERTY_MAX; i++) {
		m_old_pq_persist_property[i] = m_new_pq_persist_property[i];
		m_new_pq_persist_property[i] = pq_persist_property[i];
	}

	DDPFUNC("+");

	if (m_old_pq_persist_property[DISP_PQ_COLOR_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_COLOR_BYPASS])
		disp_color_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_COLOR_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_CCORR_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_CCORR_BYPASS])
		disp_ccorr_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_CCORR_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_GAMMA_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_GAMMA_BYPASS])
		disp_gamma_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_GAMMA_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_DITHER_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_DITHER_BYPASS])
		disp_dither_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_DITHER_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_AAL_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_AAL_BYPASS])
		disp_aal_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_AAL_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_C3D_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_C3D_BYPASS])
		disp_c3d_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_C3D_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_TDSHP_BYPASS] !=
		m_new_pq_persist_property[DISP_PQ_TDSHP_BYPASS])
		disp_tdshp_set_bypass(crtc, m_new_pq_persist_property[DISP_PQ_TDSHP_BYPASS]);

	if (m_old_pq_persist_property[DISP_PQ_DITHER_COLOR_DETECT] !=
		m_new_pq_persist_property[DISP_PQ_DITHER_COLOR_DETECT])
		disp_dither_set_color_detect(crtc,
			m_new_pq_persist_property[DISP_PQ_DITHER_COLOR_DETECT]);

	DDPFUNC("-");

	return ret;
}

static void mtk_get_panels_info(void)
{
	struct mtk_drm_private *priv = NULL;
	struct mtk_ddp_comp *output_comp;
	struct mtk_drm_panels_info *panel_ctx;
	int i;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	priv = drm_dev->dev_private;

	output_comp = mtk_ddp_comp_request_output(to_mtk_crtc(priv->crtc[0]));
	panel_ctx = vzalloc(sizeof(struct mtk_drm_panels_info));
	if (!panel_ctx) {
		DDPPR_ERR("%s panel_info alloc failed\n", __func__);
		return;
	}

	/* notify driver user does not know how many DSI connector exist */
	panel_ctx->connector_cnt = -1;

	mtk_ddp_comp_io_cmd(output_comp, NULL, GET_ALL_CONNECTOR_PANEL_NAME, panel_ctx);

	DDPMSG("get panel_info_ctx connector_cnt %d default %d\n",
			panel_ctx->connector_cnt, panel_ctx->default_connector_id);
	if (panel_ctx->connector_cnt <= 0) {
		DDPPR_ERR("%s invalid connector cnt\n", __func__);
		goto out2;
	}

	panel_ctx->connector_obj_id = vmalloc(sizeof(unsigned int) * panel_ctx->connector_cnt);
	panel_ctx->panel_name = vmalloc(sizeof(char *) * panel_ctx->connector_cnt);
	if (!panel_ctx->connector_obj_id || !panel_ctx->panel_name) {
		DDPPR_ERR("%s ojb_id or panel_name alloc fail\n", __func__);
		goto out1;
	}

	for (i = 0 ; i < panel_ctx->connector_cnt ; ++i) {
		panel_ctx->panel_name[i] = vmalloc(sizeof(char) * 64);
		if (!panel_ctx->panel_name[i]) {
			DDPPR_ERR("%s alloc panel_name fail\n", __func__);
			goto out0;
		}
	}

	mtk_ddp_comp_io_cmd(output_comp, NULL, GET_ALL_CONNECTOR_PANEL_NAME, panel_ctx);

	for (i = 0 ; i < panel_ctx->connector_cnt ; ++i)
		DDPMSG("%s get connector_id %d, panel_name %s, panel_id %u\n", __func__,
				panel_ctx->connector_obj_id[i], panel_ctx->panel_name[i],
				panel_ctx->panel_id);

out0:
	for (i = 0 ; i < panel_ctx->connector_cnt ; ++i)
		vfree(panel_ctx->panel_name[i]);
out1:
	vfree(panel_ctx->panel_name);
	vfree(panel_ctx->connector_obj_id);
out2:
	vfree(panel_ctx);
}

int mtk_drm_add_cb_data(struct cb_data_store *cb_data, unsigned int crtc_id)
{
	struct cb_data_store *tmp_cb_data = NULL;
	int search = 0;
	unsigned long flags;
	if (crtc_id >= MAX_CRTC) {
		DDPMSG("%s, crtc_id is invalid\n", __func__);
		return -1;
	}

	spin_lock_irqsave(&cb_data_clock_lock, flags);
	list_for_each_entry(tmp_cb_data, &cb_data_list[crtc_id], link) {
		if (!memcmp(&tmp_cb_data->data, &cb_data->data,
				sizeof(struct cmdq_cb_data))) {
			search = 1;
			break;
		}
	}
	if (search) {
		spin_unlock_irqrestore(&cb_data_clock_lock, flags);
		return -1;
	}

	DDPINFO("%s id %d data0x%08x\n", __func__, crtc_id, cb_data->data.data);
	list_add_tail(&cb_data->link, &cb_data_list[crtc_id]);
	spin_unlock_irqrestore(&cb_data_clock_lock, flags);

	return 0;
}

struct cb_data_store *mtk_drm_get_cb_data(unsigned int crtc_id)
{
	struct cb_data_store *tmp_cb_data = NULL;
	unsigned long flags;

	spin_lock_irqsave(&cb_data_clock_lock, flags);

	if (crtc_id < MAX_CRTC &&
		(!list_empty(&cb_data_list[crtc_id])))
		tmp_cb_data = list_first_entry(&cb_data_list[crtc_id],
			struct cb_data_store, link);
	spin_unlock_irqrestore(&cb_data_clock_lock, flags);

	return tmp_cb_data;
}

void mtk_drm_del_cb_data(struct cmdq_cb_data data, unsigned int crtc_id)
{
	struct cb_data_store *tmp_cb_data = NULL;
	unsigned long flags;

	if (!data.data) {
		DDPMSG("%s, data==NULL\n", __func__);
		return;
	}

	if (crtc_id >= MAX_CRTC) {
		DDPMSG("%s, crtc_id is invalid\n", __func__);
		return;
	}

	spin_lock_irqsave(&cb_data_clock_lock, flags);
	list_for_each_entry(tmp_cb_data, &cb_data_list[crtc_id], link) {
		if (!memcmp(&tmp_cb_data->data, &data,
				sizeof(struct cmdq_cb_data))) {
			DDPINFO("%s id %d data0x%08x\n", __func__, crtc_id, data.data);
			list_del_init(&tmp_cb_data->link);
			break;
		}
	}
	kfree(tmp_cb_data);
	spin_unlock_irqrestore(&cb_data_clock_lock, flags);
}

#if IS_ENABLED(CONFIG_MTK_DISP_DEBUG)
static bool is_comp_addr(uint32_t addr, struct mtk_ddp_comp *comp)
{
	uint32_t range = 0x1000;

	if (mtk_ddp_comp_get_type(comp->id) == MTK_DISP_ODDMR)
		range = 0x2000;
	if (addr >= comp->regs_pa && addr < comp->regs_pa + range)
		return true;
	return false;
}

static bool is_disp_reg(uint32_t addr, char *comp_name, uint32_t comp_name_len)
{
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *comp;
	int i, j;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	drm_for_each_crtc(crtc, drm_dev) {
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			continue;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		if (!crtc->enabled || mtk_crtc->ddp_mode == DDP_NO_USE)
			continue;

		for_each_comp_in_cur_crtc_path(comp, mtk_crtc, i, j) {
			if (is_comp_addr(addr, comp)) {
				mtk_ddp_comp_get_name(comp, comp_name, comp_name_len);
				return true;
			}
		}

		if (mtk_crtc->is_dual_pipe) {
			for_each_comp_in_dual_pipe(comp, mtk_crtc, i, j) {
				if (is_comp_addr(addr, comp)) {
					mtk_ddp_comp_get_name(comp, comp_name, comp_name_len);
					return true;
				}
			}
		}
	}
	return false;
}
#endif

#if defined (CONFIG_DSI_PRE_OFF)
static void ipanic_dsi_off(void)
{
	struct mtk_ddp_comp *comp;
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	static int count = 1;

	if (!count) {
		DDPINFO("%s: is already called", __func__);
		return;
	}

	count = 0;

	DDPINFO("%s: +", __func__);

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}
	/* this debug cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}

	mtk_crtc = to_mtk_crtc(crtc);
	comp = mtk_ddp_comp_request_output(mtk_crtc);
	if (!comp || !comp->funcs || !comp->funcs->io_cmd) {
		DDPINFO("cannot find output component\n");
		return;
	}

	mtk_drm_set_frame_skip(1);

	comp->funcs->io_cmd(comp, NULL, DSI_POWEROFF, NULL);

	DDPINFO("%s: -", __func__);
}
#else
static void ipanic_lcm_reset(void)
{
	struct mtk_ddp_comp *comp;
	struct drm_crtc *crtc;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_drm_private *priv;
	int enable;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}
	/* this debug cmd only for crtc0 */
	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return;
	}
	priv = crtc->dev->dev_private;
	if (priv && priv->data && priv->data->mmsys_id != MMSYS_MT6835)
		return;

	mtk_crtc = to_mtk_crtc(crtc);
	comp = mtk_ddp_comp_request_output(mtk_crtc);
	if (!comp || !comp->funcs || !comp->funcs->io_cmd) {
		DDPINFO("cannot find output component\n");
		return;
	}

	enable = 0;
	comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
	mdelay(10);
	enable = 1;
	comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
}
#endif

static void process_dbg_opt(const char *opt)
{
	DDPINFO("display_debug cmd %s\n", opt);

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return;
	}

	if (strncmp(opt, "helper", 6) == 0) {
		/*ex: echo helper:DISP_OPT_BYPASS_OVL,0 > /d/mtkfb */
		char option[100] = "";
		char *tmp;
		int value, i, limited;
		enum MTK_DRM_HELPER_OPT helper_opt;
		struct mtk_drm_private *priv =  drm_dev->dev_private;
		int ret;

		tmp = (char *)(opt + 7);
		limited = strlen(tmp);
		for (i = 0; i < 99; i++) {    /* option[99] should be '\0' to aviod oob */
			if (i >= limited) {
				DDPPR_ERR("%s, out of bound read\n", __func__);
				return;
			}
			if (tmp[i] != ',' && tmp[i] != ' ')
				option[i] = tmp[i];
			else
				break;
		}
		tmp += i + 1;
		ret = sscanf(tmp, "%d\n", &value);
		if (ret != 1) {
			DDPPR_ERR("error to parse cmd %s: %s %s ret=%d\n", opt,
				  option, tmp, ret);
			return;
		}

		DDPMSG("will set option %s to %d\n", option, value);
		mtk_drm_helper_set_opt_by_name(priv->helper_opt, option, value);
		helper_opt =
			mtk_drm_helper_name_to_opt(priv->helper_opt, option);
		mtk_update_layering_opt_by_disp_opt(helper_opt, value);

		if (helper_opt == MTK_DRM_OPT_MML_PQ) {
			struct drm_crtc *crtc;
			struct mtk_drm_crtc *mtk_crtc;

			/* this debug cmd only for crtc0 */
			crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list, typeof(*crtc),
						head);
			if (IS_ERR_OR_NULL(crtc)) {
				DDPMSG("find crtc fail\n");
				return;
			}
			mtk_crtc = to_mtk_crtc(crtc);
			if (mtk_crtc)
				mtk_crtc->is_force_mml_scen = !!value;
		}
	} else if (strncmp(opt, "mobile:", 7) == 0) {
		if (strncmp(opt + 7, "on", 2) == 0)
			g_mobile_log = 1;
		else if (strncmp(opt + 7, "off", 3) == 0)
			g_mobile_log = 0;
	} else if (strncmp(opt, "msync_debug:", 12) == 0) {
		if (strncmp(opt + 12, "on", 2) == 0)
			g_msync_debug = 1;
		else if (strncmp(opt + 12, "off", 3) == 0)
			g_msync_debug = 0;
	} else if (strncmp(opt, "msync_dy:", 9) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		if (strncmp(opt + 9, "on", 2) == 0)
			mtk_crtc->msync2.msync_dy.dy_en = 1;
		else if (strncmp(opt + 9, "off", 3) == 0)
			mtk_crtc->msync2.msync_dy.dy_en = 0;
	} else if (strncmp(opt, "fence:", 6) == 0) {
		if (strncmp(opt + 6, "on", 2) == 0)
			g_fence_log = 1;
		else if (strncmp(opt + 6, "off", 3) == 0)
			g_fence_log = 0;
	} else if (strncmp(opt, "irq:", 4) == 0) {
		if (strncmp(opt + 4, "on", 2) == 0)
			g_irq_log = 1;
		else if (strncmp(opt + 4, "off", 3) == 0)
			g_irq_log = 0;
	} else if (strncmp(opt, "detail:", 7) == 0) {
		if (strncmp(opt + 7, "on", 2) == 0)
			g_detail_log = 1;
		else if (strncmp(opt + 7, "off", 3) == 0)
			g_detail_log = 0;
	} else if (strncmp(opt, "gpuc_dp:", 8) == 0) {
		if (strncmp(opt + 8, "on", 2) == 0)
			g_gpuc_direct_push = 1;
		else if (strncmp(opt + 8, "off", 3) == 0)
			g_gpuc_direct_push = 0;
	} else if (strncmp(opt, "ovl_bwm_debug:", 14) == 0) {
		if (strncmp(opt + 14, "on", 2) == 0)
			g_ovl_bwm_debug = 1;
		else if (strncmp(opt + 14, "off", 3) == 0)
			g_ovl_bwm_debug = 0;
	} else if (strncmp(opt, "profile:", 8) == 0) {
		if (strncmp(opt + 8, "on", 2) == 0)
			g_profile_log = 1;
		else if (strncmp(opt + 8, "off", 3) == 0)
			g_profile_log = 0;
	} else if (strncmp(opt, "trace:", 6) == 0) {
		if (strncmp(opt + 6, "on", 2) == 0)
			g_trace_log = 1;
		else if (strncmp(opt + 6, "off", 3) == 0)
			g_trace_log = 0;
	} else if (strncmp(opt, "logger:", 7) == 0) {
		if (strncmp(opt + 7, "on", 2) == 0) {
			init_log_buffer();
			logger_enable = 1;
		} else if (strncmp(opt + 7, "off", 3) == 0) {
			logger_enable = 0;
		}
	} else if (strncmp(opt, "diagnose", 8) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		drm_for_each_crtc(crtc, drm_dev) {
			if (IS_ERR_OR_NULL(crtc)) {
				DDPPR_ERR("find crtc fail\n");
				continue;
			}

			mtk_crtc = to_mtk_crtc(crtc);
			if (!crtc->enabled
				|| mtk_crtc->ddp_mode == DDP_NO_USE)
				continue;

			mtk_drm_crtc_analysis(crtc);
			mtk_drm_crtc_dump(crtc);
		}
	} else if (is_bdg_supported() && strncmp(opt, "bdg_dump", 8) == 0) {
		bdg_dsi_dump_reg(DISP_BDG_DSI0);
	} else if (is_bdg_supported() && strncmp(opt, "set_data_rate:", 14) == 0) {
		unsigned int data_rate = 0;
		int ret = -1;

		ret = sscanf(opt, "set_data_rate:%d\n",
			&data_rate);
		if (ret != 1) {
			DDPMSG("[error]%d error to parse set_data_rate cmd %s\n",
				__LINE__, opt);
			return;
		}

		set_bdg_data_rate(data_rate);

	} else if (is_bdg_supported() && !strncmp(opt, "set_mask_spi:", 13)) {
		unsigned int addr = 0, val = 0, mask = 0;
		int ret = -1;

		ret = sscanf(opt, "set_mask_spi:addr=0x%x,mask=0x%x,val=0x%x\n",
			&addr, &mask, &val);
		if (ret != 3) {
			DDPMSG("[error]%d error to parse set_mt6382_spi cmd %s\n",
				__LINE__, opt);
			return;
		}

		ret = mtk_spi_mask_write(addr, mask, val);
		if (ret < 0) {
			DDPMSG("[error]write mt6382 fail,addr:0x%x, val:0x%x\n",
				addr, val);
			return;
		}
	} else if (is_bdg_supported() && !strncmp(opt, "set_mt6382_spi:", 15)) {
		unsigned int addr = 0, val = 0;
		int ret = -1;

		ret = sscanf(opt, "set_mt6382_spi:addr=0x%x,val=0x%x\n",
			&addr, &val);
		if (ret != 2) {
			DDPMSG("[error]%d error to parse set_mt6382_spi cmd %s\n",
				__LINE__, opt);
			return;
		}

		ret = mtk_spi_write(addr, val);
		if (ret < 0) {
			DDPMSG("[error]write mt6382 fail,addr:0x%x, val:0x%x\n",
				addr, val);
			return;
		}

	} else if (is_bdg_supported() && !strncmp(opt, "read_mt6382_spi:", 16)) {
		unsigned int addr = 0, val = 0;
		int ret = -1;

		ret = sscanf(opt, "read_mt6382_spi:addr=0x%x\n", &addr);
		if (ret != 1) {
			DDPMSG("[error]%d error to parse read_mt6382_spi cmd %s\n",
				__LINE__, opt);
			return;
		}

		val = mtk_spi_read(addr);
		DDPMSG("mt6382 read addr:0x%08x, val:0x%08x\n", addr, val);

	} else if (is_bdg_supported() && strncmp(opt, "check", 5) == 0) {
		if (check_stopstate(NULL) == 0)
			bdg_tx_start(DISP_BDG_DSI0, NULL);
		mdelay(100);
		return;
	} else if (strncmp(opt, "repaint", 7) == 0) {
		drm_trigger_repaint(DRM_REPAINT_FOR_IDLE, drm_dev);
	} else if (strncmp(opt, "dalprintf", 9) == 0) {
		struct drm_crtc *crtc;

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);

		if (!crtc) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		DAL_Printf("DAL printf\n");
	} else if (strncmp(opt, "dalclean", 8) == 0) {
		struct drm_crtc *crtc;

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);

		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		DAL_Clean();
	} else if (strncmp(opt, "path_switch:", 11) == 0) {
		struct drm_crtc *crtc;
		int path_sel, ret;

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);

		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}
		ret = sscanf(opt, "path_switch:%d\n", &path_sel);
		mtk_crtc_path_switch(crtc, path_sel, 1);
	} else if (strncmp(opt, "enable_idlemgr:", 15) == 0) {
		char *p = (char *)opt + 15;
		unsigned int flg = 0;
		struct drm_crtc *crtc;
		int ret;

		ret = kstrtouint(p, 0, &flg);
		if (ret) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_drm_set_idlemgr(crtc, flg, 1);
	} else if (strncmp(opt, "idle_wait:", 10) == 0) {
		unsigned long long idle_check_interval = 0;
		struct drm_crtc *crtc;
		int ret;

		ret = sscanf(opt, "idle_wait:%llu\n", &idle_check_interval);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		idle_check_interval = max(idle_check_interval, 17ULL);
		mtk_drm_set_idle_check_interval(crtc, idle_check_interval);
		DDPMSG("change idle interval to %llu ms\n",
		       idle_check_interval);
	} else if (strncmp(opt, "hrt_bw", 6) == 0) {
		struct mtk_drm_private *priv = drm_dev->dev_private;

		DDPINFO("HRT test+\n");
		if (mtk_drm_helper_get_opt(priv->helper_opt,
				MTK_DRM_OPT_MMQOS_SUPPORT))
			mtk_disp_hrt_bw_dbg();
		DDPINFO("HRT test-\n");
	} else if (strncmp(opt, "lcm_dump", 8) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (!comp) {
			DDPINFO("cannot find output component\n");
			return;
		}
		mtk_ddp_comp_io_cmd(comp, NULL,
			DSI_DUMP_LCM_INFO, NULL);
		DDPMSG("%s, finished lcm dump\n", __func__);
	} else if (strncmp(opt, "lcm0_cust", 9) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct lcm_sample_cust_data *cust_data =
				kzalloc(sizeof(struct lcm_sample_cust_data), GFP_KERNEL);

		if (cust_data == NULL) {
			DDPMSG("%s, %d, failed to allocate buffer\n",
				__func__, __LINE__);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			kfree(cust_data);
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (!comp || !comp->funcs || !comp->funcs->io_cmd) {
			DDPINFO("cannot find output component\n");
			kfree(cust_data);
			return;
		}

		cust_data->name = kzalloc(sizeof(128), GFP_KERNEL);
		if (!IS_ERR_OR_NULL(cust_data->name)) {
			DDPMSG("%s, %d, get cust name\n",
				__func__, __LINE__);
			cust_data->cmd = 0;
			comp->funcs->io_cmd(comp, NULL, LCM_CUST_FUNC, (void *)cust_data);
			DDPMSG("%s, %d, >>>> cmd:%d name:%s\n",
				__func__, __LINE__, cust_data->cmd, cust_data->name);
			kfree(cust_data->name);
		}

		DDPMSG("%s, %d, get cust type\n",
			__func__, __LINE__);
		cust_data->cmd = 1;
		comp->funcs->io_cmd(comp, NULL, LCM_CUST_FUNC, (void *)cust_data);
		DDPMSG("%s, %d, >>>> cmd:%d type:0x%x\n",
			__func__, __LINE__, cust_data->cmd, cust_data->type);

		DDPMSG("%s, %d, do cust pre-prepare\n",
			__func__, __LINE__);
		cust_data->cmd = 2;
		comp->funcs->io_cmd(comp, NULL, LCM_CUST_FUNC, (void *)cust_data);

		kfree(cust_data);
	} else if (strncmp(opt, "lcm0_reset", 10) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		int enable;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (!comp || !comp->funcs || !comp->funcs->io_cmd) {
			DDPINFO("cannot find output component\n");
			return;
		}
		enable = 1;
		comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
		msleep(20);
		enable = 0;
		comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
		msleep(20);
		enable = 1;
		comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
	} else if (strncmp(opt, "lcm1_reset", 10) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_drm_private *priv = (drm_dev) ? drm_dev->dev_private : NULL;
		int enable, i;

		if (IS_ERR_OR_NULL(priv)) {
			DDPPR_ERR("%s:%d invalid priv\n", __func__, __LINE__);
			return;
		}

		/* debug_cmd lcm0_reset handle crtc0 already */
		for (i = 1 ; i < MAX_CRTC ; ++i) {
			crtc = priv->crtc[i];
			if (!crtc) {
				DDPPR_ERR("find crtc fail\n");
				return;
			}

			mtk_crtc = to_mtk_crtc(crtc);
			comp = mtk_ddp_comp_request_output(mtk_crtc);
			if (comp && mtk_ddp_comp_get_type(comp->id) == MTK_DSI)
				break;
		}

		if (!comp || !comp->funcs || !comp->funcs->io_cmd) {
			DDPINFO("cannot find output component\n");
			return;
		}
		enable = 1;
		comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
		msleep(20);
		enable = 0;
		comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
		msleep(20);
		enable = 1;
		comp->funcs->io_cmd(comp, NULL, LCM_RESET, &enable);
	} else if (strncmp(opt, "backlight:", 10) == 0) {
		unsigned int level;
		int ret;

		ret = sscanf(opt, "backlight:%u\n", &level);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		__mtkfb_set_backlight_level(level, 0, 0x1<<SET_BACKLIGHT_LEVEL, false);
	} else if (strncmp(opt, "backlight_elvss:", 16) == 0) {
		unsigned int level;
		int ret;

		ret = sscanf(opt, "backlight_elvss:%u\n", &level);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		__mtkfb_set_backlight_level(level, 0,
				(0x1 << SET_BACKLIGHT_LEVEL) | (0x1 << SET_ELVSS_PN), false);
	} else if (strncmp(opt, "conn_backlight:", 15) == 0) {
		unsigned int level;
		unsigned int conn_id;
		int ret;

		ret = sscanf(opt, "conn_backlight:%u,%u\n", &conn_id, &level);
		if (ret != 2) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		mtk_drm_set_conn_backlight_level(conn_id, level, 0, 0x1<<SET_BACKLIGHT_LEVEL);
	} else if (strncmp(opt, "elvss:", 6) == 0) {
		unsigned int level;
		int ret;

		ret = sscanf(opt, "elvss:%u\n", &level);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		__mtkfb_set_backlight_level(level, 0, (0x1 << SET_ELVSS_PN), false);
	} else if (strncmp(opt, "backlight_grp:", 14) == 0) {
		unsigned int level;
		int ret;

		ret = sscanf(opt, "backlight_grp:%u\n", &level);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		__mtkfb_set_backlight_level(level, 0, 0x1<<SET_BACKLIGHT_LEVEL, true);
	} else if (!strncmp(opt, "aod_bl:", 7)) {
		unsigned int level;
		int ret;

		ret = sscanf(opt, "aod_bl:%u\n", &level);
		if (ret != 1) {
			DDPPR_ERR("%d fail to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		mtkfb_set_aod_backlight_level(level);
	} else if (strncmp(opt, "dump_fake_engine", 16) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		dump_fake_engine(mtk_crtc->config_regs);
	} else if (!strncmp(opt, "fake_engine:", 12)) {
		unsigned int en, idx, wr_en, rd_en, wr_pat1, wr_pat2, latency,
				preultra_cnt, ultra_cnt;
		struct drm_crtc *crtc;
		int ret = 0;

		ret = sscanf(opt, "fake_engine:%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
				&idx, &en, &wr_en, &rd_en, &wr_pat1, &wr_pat2,
				&latency, &preultra_cnt, &ultra_cnt);

		if (ret != 9) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_drm_idlemgr_kick(__func__, crtc, 1);
		mtk_drm_set_idlemgr(crtc, 0, 1);
		fake_engine(crtc, idx, en, wr_en, rd_en, wr_pat1, wr_pat2,
			latency, preultra_cnt, ultra_cnt);
	} else if (!strncmp(opt, "set_msync_cmd_level_tb:", 23)) {
		unsigned int level_id, level_fps, max_fps, min_fps;
		int ret = 0;

		ret = sscanf(opt, "set_msync_cmd_level_tb:%d,%d,%d,%d\n",
				&level_id, &level_fps, &max_fps, &min_fps);

		DDPINFO("ret:%d level_id;%d, level_fps:%d, max_fps:%d, min_fps:%d\n",
				ret, level_id, level_fps, max_fps, min_fps);
		if (ret != 4) {
			DDPPR_ERR("%d error to parse cmd %s\n",
					__LINE__, opt);
			return;
		}

		mtk_drm_set_msync_cmd_level_table(
				level_id, level_fps, max_fps, min_fps);
	} else if (!strncmp(opt, "get_msync_cmd_level_tb", 22)) {

		DDPINFO("get_msync_cmd_level_tb cmd\n");
		mtk_drm_get_msync_cmd_level_table();

	} else if (!strncmp(opt, "clear_msync_cmd_level_tb", 24)) {

		DDPINFO("clear_msync_cmd_level_tb cmd\n");
		mtk_drm_clear_msync_cmd_level_table();

	} else if (!strncmp(opt, "cat_compress_ratio_tb", 21)) {
		int i = 0;

		DDPINFO("BWMT===== normal_layer_compress_ratio_tb =====\n");
		DDPINFO("BWMT===== Item   Frame   Key   avg   peak   valid   active =====\n");
		for (i = 0; i < MAX_FRAME_RATIO_NUMBER*MAX_LAYER_RATIO_NUMBER; i++) {
			if ((normal_layer_compress_ratio_tb[i].key_value) &&
					(normal_layer_compress_ratio_tb[i].average_ratio != 0) &&
					(normal_layer_compress_ratio_tb[i].peak_ratio != 0))
				DDPINFO("BWMT===== %4d   %u   %lu   %u   %u   %u   %u =====\n", i,
					normal_layer_compress_ratio_tb[i].frame_idx,
					normal_layer_compress_ratio_tb[i].key_value,
					normal_layer_compress_ratio_tb[i].average_ratio,
					normal_layer_compress_ratio_tb[i].peak_ratio,
					normal_layer_compress_ratio_tb[i].valid,
					normal_layer_compress_ratio_tb[i].active);
		}
		DDPINFO("BWMT===== fbt_layer_compress_ratio_tb =====\n");
		DDPINFO("BWMT===== Item   Frame   Key   avg   peak   valid   active =====\n");
		for (i = 0; i < MAX_FRAME_RATIO_NUMBER; i++) {
			if ((fbt_layer_compress_ratio_tb[i].key_value) &&
					(fbt_layer_compress_ratio_tb[i].average_ratio != 0) &&
					(fbt_layer_compress_ratio_tb[i].peak_ratio != 0))
				DDPINFO("BWMT===== %4d   %u   %lu   %u   %u   %u   %u =====\n", i,
					fbt_layer_compress_ratio_tb[i].frame_idx,
					fbt_layer_compress_ratio_tb[i].key_value,
					fbt_layer_compress_ratio_tb[i].average_ratio,
					fbt_layer_compress_ratio_tb[i].peak_ratio,
					fbt_layer_compress_ratio_tb[i].valid,
					fbt_layer_compress_ratio_tb[i].active);
		}
		DDPINFO("BWMT===== unchanged_compress_ratio_table =====\n");
		DDPINFO("BWMT===== Item   Frame   Key   avg   peak   valid   active =====\n");
		for (i = 0; i < MAX_LAYER_RATIO_NUMBER; i++) {
			if ((unchanged_compress_ratio_table[i].key_value) &&
					(unchanged_compress_ratio_table[i].average_ratio != 0) &&
					(unchanged_compress_ratio_table[i].peak_ratio != 0))
				DDPINFO("BWMT===== %4d   %u   %lu   %u   %u   %u   %u =====\n", i,
					unchanged_compress_ratio_table[i].frame_idx,
					unchanged_compress_ratio_table[i].key_value,
					unchanged_compress_ratio_table[i].average_ratio,
					unchanged_compress_ratio_table[i].peak_ratio,
					unchanged_compress_ratio_table[i].valid,
					unchanged_compress_ratio_table[i].active);
		}
	} else if (strncmp(opt, "checkt", 6) == 0) { /* check trigger */
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);

		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		mtk_crtc_check_trigger(mtk_crtc, false, true);
	} else if (strncmp(opt, "checkd", 6) == 0) { /* check trigger delay */
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		mtk_crtc_check_trigger(mtk_crtc, true, true);
	} else if (!strncmp(opt, "fake_layer:", 11)) {
		unsigned int mask;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		int ret = 0;

		ret = sscanf(opt, "fake_layer:0x%x\n", &mask);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_drm_idlemgr_kick(__func__, crtc, 1);
		mtk_drm_set_idlemgr(crtc, 0, 1);

		prepare_fake_layer_buffer(crtc);

		mtk_crtc = to_mtk_crtc(crtc);
		if (!mask && mtk_crtc->fake_layer.fake_layer_mask)
			mtk_crtc->fake_layer.first_dis = true;
		mtk_crtc->fake_layer.fake_layer_mask = mask;

		DDPINFO("fake_layer:0x%x enable\n", mask);
	} else if (!strncmp(opt, "mipi_ccci:", 10)) {
		unsigned int en, ret;

		ret = sscanf(opt, "mipi_ccci:%d\n", &en);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		DDPINFO("mipi_ccci:%d\n", en);
		mtk_disp_mipi_ccci_callback(en, 0);
	} else if (strncmp(opt, "aal:", 4) == 0) {
		disp_aal_debug(opt + 4);
	} else if (strncmp(opt, "c3d:", 4) == 0) {
		mtk_disp_c3d_debug(opt + 4);
	} else if (strncmp(opt, "oddmr:", 4) == 0) {
		mtk_disp_oddmr_debug(opt + 6);
	} else if (strncmp(opt, "aee:", 4) == 0) {
		DDPAEE("trigger aee dump of mmproile\n");
	} else if (strncmp(opt, "send_ddic_test:", 15) == 0) {
		unsigned int case_num, ret;

		ret = sscanf(opt, "send_ddic_test:%d\n", &case_num);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("send_ddic_test:%d\n", case_num);

		ddic_dsi_send_cmd_test(case_num);
	} else if (strncmp(opt, "read_ddic_test:", 15) == 0) {
		unsigned int case_num, ret;

		ret = sscanf(opt, "read_ddic_test:%d\n", &case_num);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("read_ddic_test:%d\n", case_num);

		ddic_dsi_read_cmd_test(case_num);
	} else if (strncmp(opt, "ddic_page_switch:", 17) == 0) {
		u8 addr, val1, val2, val3;
		u8 val4, val5, val6;
		unsigned int cmd_num, ret;

		ret = sscanf(opt, "ddic_page_switch:%d,%x,%x,%x,%x,%x,%x,%x\n",
				&cmd_num, &addr, &val1, &val2, &val3,
				&val4, &val5, &val6);

		if (ret != (cmd_num + 1)) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("ddic_spr_switch:%d\n", cmd_num);

		ddic_dsi_send_switch_pgt(cmd_num, addr, val1, val2, val3,
			val4, val5, val6);
	} else if (strncmp(opt, "read_cm:", 8) == 0) {
		u8 addr;
		unsigned int ret;

		ret = sscanf(opt, "read_cm:%x\n", &addr);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}
		DDPMSG("read_cm:%d\n", addr);
		ddic_dsi_read_cm_cmd(addr);
	} else if (strncmp(opt, "ap_spr_cm_bypass:", 17) == 0) {
		unsigned int spr_bypass, cm_bypass, ret;

		ret = sscanf(opt, "ap_spr_cm_bypass:%d,%d\n", &spr_bypass, &cm_bypass);
		if (ret != 2) {
			DDPPR_ERR("%d error to set ap_spr_cm_bypass %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("ap_spr_cm_bypass:%d, %d\n", spr_bypass, cm_bypass);

		disp_spr_bypass = spr_bypass;
		disp_cm_bypass = cm_bypass;
	} else if (strncmp(opt, "disp_cm_set:", 12) == 0) {
		unsigned int en, ret;
		unsigned int cm_c00, cm_c01, cm_c02;
		unsigned int cm_c10, cm_c11, cm_c12;
		unsigned int cm_c20, cm_c21, cm_c22;

		ret = sscanf(opt, "disp_cm_set:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			&en, &cm_c00, &cm_c01, &cm_c02, &cm_c10, &cm_c11,
			&cm_c12, &cm_c20, &cm_c21, &cm_c22);
		if (ret != 10) {
			DDPPR_ERR("%d error to set disp_cm_set %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("disp_cm_set:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			en, cm_c00, cm_c01, cm_c02, cm_c10, cm_c11,
			cm_c12, cm_c20, cm_c21, cm_c22);

		mtk_crtc_set_cm_tune_para(en, cm_c00, cm_c01, cm_c02, cm_c10, cm_c11,
			cm_c12, cm_c20, cm_c21, cm_c22);
	} else if (strncmp(opt, "disp_spr_set:", 13) == 0) {
		unsigned int type, tune_num, tune_val, ret;

		ret = sscanf(opt, "disp_spr_set:%d,%d,%d\n", &type,
			&tune_num, &tune_val);
		if (ret != 3) {
			DDPPR_ERR("%d error to set disp_spr_set %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("disp_spr_set:%d, %d, %d\n", type, tune_num, tune_val);

		mtk_crtc_set_spr_tune_para(type, tune_num, tune_val);
	} else if (strncmp(opt, "disp_spr_tune_en:", 17) == 0) {
		unsigned int en, ret;

		ret = sscanf(opt, "disp_spr_tune_en:%d\n", &en);
		if (ret != 1) {
			DDPPR_ERR("%d error to set disp_spr_tune_en %s\n",
				__LINE__, opt);
			return;
		}

		DDPMSG("disp_spr_tune_en:%d\n", en);

		mtk_crtc_spr_tune_enable(en);
	} else if (!strncmp(opt, "chg_mipi:", 9)) {
		int ret;
		unsigned int rate;
		struct drm_crtc *crtc;

		ret = sscanf(opt, "chg_mipi:%u\n", &rate);
		if (ret != 1) {
			DDPMSG("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}
		DDPMSG("chg_mipi:%u  1\n", rate);

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
						typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPMSG("find crtc fail\n");
			return;
		}
		DDPMSG("chg_mipi:%u  2\n", rate);

		mtk_mipi_clk_change(crtc, rate);

	} else if (strncmp(opt, "mipi_volt:", 10) == 0) {
		char *p = (char *)opt + 10;
		int ret;

		ret = kstrtouint(p, 0, &mipi_volt);
		if (ret) {
			DDPMSG("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		DDPMSG("mipi_volt change :%d\n",
		       mipi_volt);
	} else if (strncmp(opt, "dump_layer:", 11) == 0) {
		int ret;
		unsigned int dump_en;
		unsigned int downSampleX, downSampleY;
		int layer_id;

		DDPMSG("get dump\n");
		ret = sscanf(opt, "dump_layer:%d,%d,%d,%d\n", &dump_en,
			     &downSampleX, &downSampleY, &layer_id);
		if (ret != 4) {
			DDPMSG("error to parse cmd\n");
			return;
		}

		if (downSampleX)
			gCapturePriLayerDownX = downSampleX;
		if (downSampleY)
			gCapturePriLayerDownY = downSampleY;
		gCaptureAssignLayer = layer_id;
		gCaptureOVLEn = dump_en;
		DDPMSG("dump params (%d,%d,%d,%d)\n", gCaptureOVLEn,
			gCapturePriLayerDownX, gCapturePriLayerDownY, gCaptureAssignLayer);
	} else if (strncmp(opt, "dump_out_layer:", 15) == 0) {
		int ret;
		unsigned int dump_en;
		unsigned int downSampleX, downSampleY;

		DDPMSG("get dump\n");
		ret = sscanf(opt, "dump_out_layer:%d,%d,%d\n", &dump_en,
			     &downSampleX, &downSampleY);
		if (ret != 3) {
			DDPMSG("error to parse cmd\n");
			return;
		}

		if (downSampleX)
			gCaptureOutLayerDownX = downSampleX;
		if (downSampleY)
			gCaptureOutLayerDownY = downSampleY;
		gCaptureWDMAEn = dump_en;
		DDPMSG("dump params (%d,%d,%d)\n", gCaptureWDMAEn,
			gCaptureOutLayerDownX, gCaptureOutLayerDownY);
	} else if (strncmp(opt, "dump_user_buffer:", 17) == 0) {
		int ret;
		unsigned int dump_en;

		DDPMSG("get dump\n");
		ret = sscanf(opt, "dump_user_buffer:%d\n", &dump_en);
		if (ret != 1) {
			DDPMSG("error to parse cmd\n");
			return;
		}
		gCaptureWDMAEn = dump_en;
	} else if (strncmp(opt, "dptx:", 5) == 0) {
		mtk_dp_debug(opt + 5);
	} else if (strncmp(opt, "dpintf_dump:", 12) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		drm_for_each_crtc(crtc, drm_dev) {
			if (IS_ERR_OR_NULL(crtc)) {
				DDPPR_ERR("find crtc fail\n");
				continue;
			}
			DDPINFO("------find crtc------");
			mtk_crtc = to_mtk_crtc(crtc);
			if (!crtc->enabled
				|| mtk_crtc->ddp_mode == DDP_NO_USE)
				continue;

			mtk_crtc = to_mtk_crtc(crtc);
			comp = mtk_ddp_comp_request_output(mtk_crtc);
			if (comp)
				mtk_dp_intf_dump(comp);
		}
	} else if (strncmp(opt, "arr4_enable", 11) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_dsi_lfr_con lfr_con = {0};

		lfr_con.lfr_mode     = mtk_dbg_get_lfr_mode_value();
		lfr_con.lfr_type     = mtk_dbg_get_lfr_type_value();
		lfr_con.lfr_enable   = mtk_dbg_get_lfr_enable_value();
		lfr_con.lfr_vse_dis  = mtk_dbg_get_lfr_vse_dis_value();
		lfr_con.lfr_skip_num = mtk_dbg_get_lfr_skip_num_value();

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (comp)
			comp->funcs->io_cmd(comp, NULL, DSI_LFR_SET, &lfr_con);

	} else if (strncmp(opt, "LFR_update", 10) == 0) {
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (comp)
			comp->funcs->io_cmd(comp, NULL, DSI_LFR_UPDATE, NULL);

	} else if (strncmp(opt, "LFR_status_check", 16) == 0) {
		//unsigned int data = mtk_dbg_get_LFR_value();
		struct mtk_ddp_comp *comp;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (comp)
			comp->funcs->io_cmd(comp, NULL, DSI_LFR_STATUS_CHECK, NULL);

	} else if (strncmp(opt, "tui:", 4) == 0) {
		unsigned int en, ret;

		ret = sscanf(opt, "tui:%d\n", &en);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		if (en)
			display_enter_tui();
		else
			display_exit_tui();
	} else if (strncmp(opt, "cwb_en:", 7) == 0) {
		unsigned int ret, enable;

		/* this debug cmd only for crtc0 */
		ret = sscanf(opt, "cwb_en:%d\n", &enable);
		if (ret != 1) {
			DDPMSG("error to parse cmd\n");
			return;
		}

		mtk_drm_cwb_enable(enable, &user_cwb_funcs, IMAGE_ONLY);
	} else if (strncmp(opt, "cwb_roi:", 8) == 0) {
		unsigned int ret, offset_x, offset_y, clip_w, clip_h;
		struct mtk_rect rect;

		/* this debug cmd only for crtc0 */
		ret = sscanf(opt, "cwb_roi:%d,%d,%d,%d\n", &offset_x,
			     &offset_y, &clip_w, &clip_h);
		if (ret != 4) {
			DDPMSG("error to parse cmd\n");
			return;
		}
		rect.x = offset_x;
		rect.y = offset_y;
		rect.width = clip_w;
		rect.height = clip_h;

		mtk_drm_set_cwb_roi(rect);
	} else if (strncmp(opt, "cwb:", 4) == 0) {
		unsigned int ret, enable, offset_x, offset_y;
		unsigned int clip_w, clip_h;
		struct mtk_rect rect;

		/* this debug cmd only for crtc0 */
		ret = sscanf(opt, "cwb:%d,%d,%d,%d,%d\n", &enable,
				&offset_x, &offset_y,
				&clip_w, &clip_h);
		if (ret != 5) {
			DDPMSG("error to parse cmd\n");
			return;
		}
		rect.x = offset_x;
		rect.y = offset_y;
		rect.width = clip_w;
		rect.height = clip_h;

		mtk_drm_set_cwb_roi(rect);
		mtk_drm_cwb_enable(enable, &user_cwb_funcs, IMAGE_ONLY);
	} else if (strncmp(opt, "cwb_get_buffer", 14) == 0) {
		u8 *user_buffer;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_cwb_info *cwb_info;
		int width, height, size, ret;
		int Bpp;

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		cwb_info = mtk_crtc->cwb_info;
		if (!cwb_info)
			return;

		DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
		width = cwb_info->src_roi.width;
		height = cwb_info->src_roi.height;
		Bpp = mtk_get_format_bpp(cwb_info->buffer[0].fb->format->format);
		size = sizeof(u8) * width * height * Bpp;
		user_buffer = vmalloc(size);
		mtk_drm_set_cwb_user_buf((void *)user_buffer, IMAGE_ONLY);
		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		DDPMSG("[capture] wait frame complete\n");
		ret = wait_for_completion_interruptible_timeout(&cwb_cmp,
			msecs_to_jiffies(3000));
		if (ret > 0)
			DDPMSG("[capture] frame complete done\n");
		else {
			DDPMSG("[capture] wait frame timeout(3s)\n");
			DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);
			mtk_drm_set_cwb_user_buf((void *)NULL, IMAGE_ONLY);
			DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);
		}
		vfree(user_buffer);
		reinit_completion(&cwb_cmp);
	} else if (strncmp(opt, "cwb_change_path:", 16) == 0) {
		int path, ret;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_cwb_info *cwb_info;

		ret = sscanf(opt, "cwb_change_path:%d\n", &path);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}
		mtk_crtc = to_mtk_crtc(crtc);
		if (!mtk_crtc->cwb_info) {
			mtk_crtc->cwb_info = kzalloc(sizeof(struct mtk_cwb_info),
				GFP_KERNEL);
			DDPMSG("%s: need allocate memory\n", __func__);
		}
		cwb_info = mtk_crtc->cwb_info;
		DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);

		if (path == 0)
			cwb_info->scn = WDMA_WRITE_BACK;
		else if (path == 1)
			cwb_info->scn = WDMA_WRITE_BACK_OVL;

		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);

	} else if (strncmp(opt, "cwb_change_color_format:", 24) == 0) {
		int ret, color;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;

		ret = sscanf(opt, "cwb_change_color_format:%d\n", &color);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n",
				__LINE__, opt);
			return;
		}

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}
		mtk_crtc = to_mtk_crtc(crtc);
		DDP_MUTEX_LOCK(&mtk_crtc->lock, __func__, __LINE__);

		if (!mtk_crtc->cwb_info) {
			mtk_crtc->cwb_info = kzalloc(sizeof(struct mtk_cwb_info), GFP_KERNEL);
		} else if (mtk_crtc->cwb_info && mtk_crtc->cwb_info->buffer[0].fb != NULL) {
			drm_framebuffer_put(mtk_crtc->cwb_info->buffer[0].fb);
			drm_framebuffer_put(mtk_crtc->cwb_info->buffer[1].fb);
		}
		set_cwb_info_buffer(crtc, color);

		DDP_MUTEX_UNLOCK(&mtk_crtc->lock, __func__, __LINE__);

	} else if (strncmp(opt, "fake_wcg", 8) == 0) {
		unsigned int fake_hdr_en = 0;
		struct drm_crtc *crtc;
		struct mtk_panel_params *params = NULL;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_ddp_comp *output_comp;
		struct mtk_crtc_state *state;
		unsigned int mode_cont, cur_mode_idx, i;
		int ret;

		ret = sscanf(opt, "fake_wcg:%u\n", &fake_hdr_en);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}
		mtk_crtc = to_mtk_crtc(crtc);

		state = to_mtk_crtc_state(mtk_crtc->base.state);
		cur_mode_idx = state->prop_val[CRTC_PROP_DISP_MODE_IDX];
		output_comp = mtk_ddp_comp_request_output(mtk_crtc);
		if (!output_comp) {
			DDPMSG("output_comp is null!\n");
			return;
		}
		mtk_ddp_comp_io_cmd(output_comp, NULL, DSI_GET_MODE_CONT, &mode_cont);

		DDPINFO("set panel color_mode info: mode_cont = %d, cur_mode_idx = %d\n",
							mode_cont, cur_mode_idx);

		for (i = 0; i < mode_cont; i++) {
			mtk_ddp_comp_io_cmd(output_comp, NULL, DSI_SET_PANEL_PARAMS_BY_IDX, &i);
			params = mtk_drm_get_lcm_ext_params(crtc);
			if (!params) {
				DDPINFO("[Fake HDR] find lcm ext fail[%d]\n", i);
				return;
			}
			params->lcm_color_mode = (fake_hdr_en) ?
				MTK_DRM_COLOR_MODE_DISPLAY_P3 : MTK_DRM_COLOR_MODE_NATIVE;
		}
		mtk_ddp_comp_io_cmd(output_comp, NULL, DSI_SET_PANEL_PARAMS_BY_IDX, &cur_mode_idx);

		DDPINFO("set panel color_mode to %d\n", params->lcm_color_mode);
	} else if (strncmp(opt, "fake_mode:", 10) == 0) {
		unsigned int en = 0;
		struct drm_crtc *crtc;
		struct mtk_panel_funcs *funcs;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_ddp_comp *comp;
		int tmp;
		int ret;

		ret = sscanf(opt, "fake_mode:%u\n", &en);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		if (en != 0 && en != 1) {
			DDPPR_ERR("[Fake mode] not support cmd param=%d\n", en);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		funcs = mtk_drm_get_lcm_ext_funcs(crtc);
		if (!funcs || !funcs->set_value) {
			DDPPR_ERR("[Fake mode] find lcm funcs->debug_set fail\n");
			return;
		}
		funcs->set_value(en);
		DDPINFO("[Fake mode] set panel debug to %d\n", en);

		mtk_crtc = to_mtk_crtc(crtc);
		comp = mtk_ddp_comp_request_output(mtk_crtc);
		mtk_ddp_comp_io_cmd(comp, NULL,	DSI_FILL_MODE_BY_CONNETOR, mtk_crtc);
		tmp = mtk_crtc->avail_modes_num;
		mtk_ddp_comp_io_cmd(comp, NULL,	DSI_SET_CRTC_AVAIL_MODES, mtk_crtc);
		DDPINFO("[Fake mode] avail_modes_num:%d->%d\n",
							tmp, mtk_crtc->avail_modes_num);
#if IS_ENABLED(CONFIG_MTK_DISP_DEBUG)
	} else if (strncmp(opt, "after_commit:", strlen("after_commit:")) == 0) {
		int ret;

		memset(&g_wr_reg, 0, sizeof(g_wr_reg));
		ret = sscanf(opt, "after_commit:%u\n", &g_wr_reg.after_commit);
		if (ret != 1) {
			DDPPR_ERR("[reg_dbg] error to parse cmd %s\n", opt);
			return;
		}
		DDPMSG("[reg_dbg] set after_commit:%u\n", g_wr_reg.after_commit);
	} else if (strncmp(opt, "gce_wr:", strlen("gce_wr:")) == 0) {
		uint32_t addr, val, mask;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct cmdq_pkt *handle;
		char comp_name[64] = {0};
		int ret;

		ret = sscanf(opt, "gce_wr:%x,%x,%x\n", &addr, &val, &mask);
		if (ret != 3) {
			DDPPR_ERR("[reg_dbg] error to parse cmd %s\n", opt);
			return;
		}

		if (!is_disp_reg(addr, comp_name, sizeof(comp_name))) {
			DDPPR_ERR("[reg_dbg] not display register!\n");
			return;
		}

		DDPMSG("[reg_dbg] comp:%s, addr:0x%x, val:0x%x, mask:0x%x\n",
				comp_name, addr, val, mask);
		if (g_wr_reg.after_commit == 1) {
			g_wr_reg.reg[g_wr_reg.index].addr = addr;
			g_wr_reg.reg[g_wr_reg.index].val = val;
			g_wr_reg.reg[g_wr_reg.index].mask = mask;
			if (g_wr_reg.index < 63)
				g_wr_reg.index++;
		} else {
			/* this debug cmd only for crtc0 */
			crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
						typeof(*crtc), head);
			if (IS_ERR_OR_NULL(crtc)) {
				DDPPR_ERR("[reg_dbg] find crtc fail\n");
				return;
			}

			mtk_crtc = to_mtk_crtc(crtc);

			mtk_crtc_pkt_create(&handle, &mtk_crtc->base,
					mtk_crtc->gce_obj.client[CLIENT_DSI_CFG]);
			cmdq_pkt_write(handle, mtk_crtc->gce_obj.base,
					addr, val, mask);
			cmdq_pkt_flush(handle);
			cmdq_pkt_destroy(handle);
		}
	} else if (strncmp(opt, "gce_rd:", strlen("gce_rd:")) == 0) {
		uint32_t addr, val;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct cmdq_pkt *handle;
		struct cmdq_pkt_buffer *cmdq_buf;
		int ret;

		ret = sscanf(opt, "gce_rd:%x\n", &addr);
		if (ret != 1) {
			DDPPR_ERR("[reg_dbg] error to parse cmd %s\n", opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("[reg_dbg] find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		cmdq_buf = &(mtk_crtc->gce_obj.buf);

		mtk_crtc_pkt_create(&handle, &mtk_crtc->base,
				mtk_crtc->gce_obj.client[CLIENT_DSI_CFG]);
		cmdq_pkt_mem_move(handle, NULL, addr,
				cmdq_buf->pa_base + DISP_SLOT_TE1_EN, CMDQ_THR_SPR_IDX1);
		cmdq_pkt_flush(handle);
		cmdq_pkt_destroy(handle);

		val = *(unsigned int *)(cmdq_buf->va_base + DISP_SLOT_TE1_EN);
		DDPMSG("[reg_dbg] gce_rd: addr(0x%x) = 0x%x\n", addr, val);
#endif
	} else if (strncmp(opt, "pq_dump", 7) == 0) {
		unsigned int dump_flag = 0;
		int ret;

		ret = sscanf(opt, "pq_dump:%x\n", &dump_flag);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		DDPMSG("pq start dump, dump flag:0x%x\n", dump_flag);
		if (dump_flag & 0x1)
			mtk_aal_regdump();
		if (dump_flag & 0x2)
			mtk_c3d_regdump();
		if (dump_flag & 0x4)
			mtk_ccorr_regdump();
		if (dump_flag & 0x8)
			mtk_color_regdump();
		if (dump_flag & 0x10)
			mtk_dither_regdump();
		if (dump_flag & 0x20)
			mtk_disp_tdshp_regdump();
		if (dump_flag & 0x40)
			mtk_dmdp_aal_regdump();
		if (dump_flag & 0x80)
			mtk_gamma_regdump();
	} else if (strncmp(opt, "esd_check", 9) == 0) {
		unsigned int esd_check_en = 0;
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		struct mtk_drm_esd_ctx *esd_ctx;
		int ret;

		ret = sscanf(opt, "esd_check:%u\n", &esd_check_en);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);
		esd_ctx = mtk_crtc->esd_ctx;
		if (esd_ctx != NULL) {
			esd_ctx->chk_en = esd_check_en;
			DDPINFO("set esd_check_en to %d\n", esd_check_en);
		} else {
			DDPINFO("esd_ctx is null!\n");
		}
	} else if (strncmp(opt, "mml_debug:", 10) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		int ret, value;

		ret = sscanf(opt + 10, "%d\n", &value);
		if (ret <= 0) {
			DDPMSG("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list, typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}
		mtk_crtc = to_mtk_crtc(crtc);

		mtk_crtc->mml_debug = value;
		DDPMSG("mml_debug:%s %s %s\n",
			value & DISP_MML_DBG_LOG ? "DBG_LOG" : "",
			value & DISP_MML_MMCLK_UNLIMIT ? "MMCLK_UNLIMIT" : "",
			value & DISP_MML_IR_CLEAR ? "IR_CLEAR" : "");
	} else if (strncmp(opt, "dual_te:", 8) == 0) {
		struct drm_crtc *crtc;

		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}
		if (strncmp(opt + 8, "1", 1) == 0) {
			mtk_drm_switch_te(crtc, 1, true);
			DDPMSG("switched to te1\n");
		} else if (strncmp(opt + 8, "0", 1) == 0) {
			mtk_drm_switch_te(crtc, 0, true);
			DDPMSG("switched to te0\n");
		} else {
			DDPMSG("dual_te parse error!\n");
		}
	} else if (strncmp(opt, "manual_mml_mode:", 16) == 0) {
		// 0 as not use manual
		if (strncmp(opt + 16, "0", 1) == 0)
			g_mml_mode = MML_MODE_UNKNOWN;
		else if (strncmp(opt + 16, "1", 1) == 0)
			g_mml_mode = MML_MODE_DIRECT_LINK;
		else if (strncmp(opt + 16, "2", 1) == 0)
			g_mml_mode = MML_MODE_RACING;
		else if (strncmp(opt + 16, "3", 1) == 0)
			g_mml_mode = MML_MODE_MML_DECOUPLE;
		else if (strncmp(opt + 16, "4", 1) == 0)
			g_mml_mode = MML_MODE_MDP_DECOUPLE;
		else if (strncmp(opt + 16, "-1", 2) == 0)
			g_mml_mode = MML_MODE_NOT_SUPPORT;
		DDPMSG("mml_mode:%d", g_mml_mode);
	} else if (strncmp(opt, "force_mml:", 10) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		int force_mml_scen = 0;

		if (strncmp(opt + 10, "1", 1) == 0)
			force_mml_scen = 1;
		else if (strncmp(opt + 10, "0", 1) == 0)
			force_mml_scen = 0;
		DDPMSG("disp_mml:%d", force_mml_scen);

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);

		if (IS_ERR_OR_NULL(crtc)) {
			DDPPR_ERR("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);

		if (mtk_crtc)
			mtk_crtc->is_force_mml_scen = force_mml_scen;
	} else if (strncmp(opt, "mml_cmd_ir:", 11) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		bool mml_cmd_ir = false;

		if (strncmp(opt + 11, "1", 1) == 0)
			mml_cmd_ir = true;
		else if (strncmp(opt + 11, "0", 1) == 0)
			mml_cmd_ir = false;
		DDPMSG("mml_cmd_ir:%d", mml_cmd_ir);

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);

		if (IS_ERR_OR_NULL(crtc)) {
			pr_info("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);

		if (mtk_crtc)
			mtk_crtc->mml_cmd_ir = mml_cmd_ir;
	} else if (strncmp(opt, "mml_enable_ir:", 14) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		bool mml_ir_enable = false;

		if (strncmp(opt + 14, "1", 1) == 0)
			mml_ir_enable = true;
		else if (strncmp(opt + 14, "0", 1) == 0)
			mml_ir_enable = false;
		DDPMSG("mml_enable_ir:%d", mml_ir_enable);

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);

		if (IS_ERR_OR_NULL(crtc)) {
			pr_info("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);

		if (mtk_crtc)
			mtk_crtc->mml_ir_enable = mml_ir_enable;
	} else if (strncmp(opt, "pf_ts_type:", 11) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		int ret, pf_ts_type;

		ret = sscanf(opt, "pf_ts_type:%d\n", &pf_ts_type);
		if (ret != 1) {
			DDPPR_ERR("%d error to parse cmd %s\n", __LINE__, opt);
			return;
		}

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			pr_info("find crtc fail\n");
			return;
		}
		mtk_crtc = to_mtk_crtc(crtc);

		if (mtk_crtc) {
			mtk_crtc->pf_ts_type = pf_ts_type;
			mtk_crtc->pf_time = 0;
		}
	} else if (strncmp(opt, "hrt_usage:", 10) == 0) {
		struct mtk_drm_private *priv = drm_dev->dev_private;
		int crtc_idx = 0;

		if (strncmp(opt + 10, "0", 1) == 0)
			crtc_idx = 0;
		else if (strncmp(opt + 10, "1", 1) == 0)
			crtc_idx = 1;
		else if (strncmp(opt + 10, "2", 1) == 0)
			crtc_idx = 2;

		if (strncmp(opt + 11, "1", 1) == 0)
			priv->usage[crtc_idx] = DISP_OPENING;
		else if (strncmp(opt + 11, "0", 1) == 0)
			priv->usage[crtc_idx] = DISP_ENABLE;
		DDPMSG("set crtc %d usage to %d", crtc_idx, priv->usage[crtc_idx]);
	} else if (strncmp(opt, "spr_ip_cfg:", 11) == 0) {
		struct drm_crtc *crtc;
		struct mtk_drm_crtc *mtk_crtc;
		char *tmp;
		char cmd[25] = "";
		unsigned int addr, value, len, idx;
		int ret;
		unsigned int i, j;
		unsigned int *spr_ip_params;

		DDPINFO("set spr ip start\n");

		/* this debug cmd only for crtc0 */
		crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
					typeof(*crtc), head);
		if (IS_ERR_OR_NULL(crtc)) {
			DDPMSG("find crtc fail\n");
			return;
		}

		mtk_crtc = to_mtk_crtc(crtc);

		if (!mtk_crtc->panel_ext->params->spr_params.spr_ip_params) {
			DDPINFO("spr_ip_params is null\n");
			return;
		}
		spr_ip_params = mtk_crtc->panel_ext->params->spr_params.spr_ip_params;

		tmp = (char *)(opt + 11);
		len = strlen(tmp);
		DDPINFO("len %d, tmp %s", len, tmp);

		for (i = 0, j = 0; i <= len; i++) {
			if (tmp[i] != ',' && i < len) {
				cmd[j] = tmp[i];
				j++;
			} else {
				if (i == len)
					j++;
				cmd[j] = '\n';

				ret = sscanf(cmd, "0x%x:0x%x", &addr, &value);
				if (ret != 2) {
					DDPMSG("ret %d, error to parse cmd %s", ret, cmd);
					return;
				}
				DDPINFO("addr 0x%08x, value 0x%08x\n", addr, value);

				addr = addr & 0xfff;
				if (addr >= 0x80 && addr <= 0xd7c) {
					idx = (addr - 0x80) / 4;
					spr_ip_params[idx] = value;
					DDPINFO("set spr ip cfg %d to 0x%08x\n",
						idx, spr_ip_params[idx]);
				} else {
					DDPINFO("spr_ip_params addr is wrong\n");
				}
				j = 0;
			}
		}
		DDPINFO("set spr ip done\n");
	} else if (strncmp(opt, "get_panels_info", 15) == 0) {
		mtk_get_panels_info();
	} else if (strncmp(opt, "clear_errdump", 13) == 0) {
		memset(err_buffer[0], 0, ERROR_BUFFER_COUNT * LOGGER_BUFFER_SIZE);
		memset(dump_buffer[0], 0, DUMP_BUFFER_COUNT * LOGGER_BUFFER_SIZE);
	} else if (strncmp(opt, "conn_obj_id", 11) == 0) {
		unsigned int value;
		int ret;

		ret = sscanf(opt, "conn_obj_id:%u\n", &value);
		if (ret != 1) {
			DDPPR_ERR("conn_obj_id scan fail, ret=%d\n", ret);
			return;
		}

		ret = mtk_drm_get_conn_obj_id_from_idx(value, 0);
		DDPINFO("disp_idx %u, conn_obj_id %d\n", value, ret);
	} else if (strncmp(opt, "dsi0_panel_id", 13) == 0) {
		DDPINFO("dsi0_panel_id is %d\n", dsi0_panel_id);
	} else if (strncmp(opt, "lcd:", 4) == 0) {
		struct mtk_drm_private *priv = drm_dev->dev_private;

		if (strncmp(opt + 4, "on", 2) == 0)
			noti_uevent_user(&priv->uevent_data, 1);
		else if (strncmp(opt + 4, "off", 3) == 0)
			noti_uevent_user(&priv->uevent_data, 0);
	} else if (strncmp(opt, "wait_frame_test:", strlen("wait_frame_test:")) == 0) {
		if (strncmp(opt + strlen("wait_frame_test:"), "1", 1) == 0)
			wait_frame_condition(DISP_FRAME_START, HZ * 3);
		else if (strncmp(opt + strlen("wait_frame_test:"), "2", 1) == 0)
			wait_frame_condition(DISP_FRAME_DONE, HZ * 3);
	}
}

static void process_dbg_cmd(char *cmd)
{
	char *tok;

	DDPINFO("[mtkfb_dbg] %s\n", cmd);

	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}
static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count,
			  loff_t *ppos)
{
	int debug_bufmax;
	static int n;

	if (*ppos != 0 || !is_buffer_init)
		goto out;

	if (!debug_buffer) {
		debug_buffer = vmalloc(sizeof(char) * DEBUG_BUFFER_SIZE);
		if (!debug_buffer)
			return -ENOMEM;

		memset(debug_buffer, 0, sizeof(char) * DEBUG_BUFFER_SIZE);
	}

	debug_bufmax = DEBUG_BUFFER_SIZE - 1;
	n = debug_get_info(debug_buffer, debug_bufmax);

out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

#if IS_ENABLED(CONFIG_DRM_PANEL_MCD_COMMON)
int mtkfb_debug_show(struct seq_file *m, void *unused)
{
	int debug_bufmax;
	static int n;

	if (!is_buffer_init)
		goto out;

	if (!debug_buffer) {
		debug_buffer = vmalloc(sizeof(char) * DEBUG_BUFFER_SIZE);
		if (!debug_buffer)
			return -ENOMEM;

		memset(debug_buffer, 0, sizeof(char) * DEBUG_BUFFER_SIZE);
	}

	debug_bufmax = DEBUG_BUFFER_SIZE - 1;
	n = debug_get_info(debug_buffer, debug_bufmax);

out:
	if (n < 0)
		return -EINVAL;

	//return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
	seq_puts(m, "------ DISPLAY DEBUG INFO (/proc/mtkfb) ------\n");
	if (debug_buffer)
		seq_puts(m, debug_buffer);

	return 0;
}
EXPORT_SYMBOL(mtkfb_debug_show);

int mtk_drm_uevent_trigger(int state)
{
	return noti_uevent_user_by_drm(drm_dev, state);
}
EXPORT_SYMBOL(mtk_drm_uevent_trigger);
#endif

static ssize_t debug_write(struct file *file, const char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	const int debug_bufmax = 512 - 1;
	size_t ret;
	char cmd_buffer[512];

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buffer, ubuf, count))
		return -EFAULT;

	cmd_buffer[count] = 0;

	process_dbg_cmd(cmd_buffer);

	return ret;
}

static const struct file_operations debug_fops = {
	.read = debug_read, .write = debug_write, .open = debug_open,
};

static const struct proc_ops debug_proc_fops = {
	.proc_read = debug_read,
	.proc_write = debug_write,
	.proc_open = debug_open,
};

static int idletime_set(void *data, u64 val)
{
	struct drm_crtc *crtc;
	u64 ret = 0;

	if (val < 33)
		val = 33;
	if (val > 1000000)
		val = 1000000;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return -ENODEV;
	}
	ret = mtk_drm_set_idle_check_interval(crtc, val);
	if (ret == 0)
		return -ENODEV;

	return 0;
}

static int idletime_get(void *data, u64 *val)
{
	struct drm_crtc *crtc;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return -ENODEV;
	}
	*val = mtk_drm_get_idle_check_interval(crtc);
	if (*val == 0)
		return -ENODEV;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(idletime_fops, idletime_get, idletime_set, "%llu\n");

static int idletime_proc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t idletime_proc_set(struct file *file, const char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	struct drm_crtc *crtc;
	int ret;
	unsigned long long val;

	ret = kstrtoull_from_user(ubuf, count, 0, &val);
	if (ret)
		return ret;

	if (val < 33)
		val = 33;
	if (val > 1000000)
		val = 1000000;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return -ENODEV;
	}
	ret = mtk_drm_set_idle_check_interval(crtc, val);
	if (ret == 0)
		return -ENODEV;

	return count;
}

static ssize_t idletime_proc_get(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct drm_crtc *crtc;
	unsigned long long val;
	int n = 0;
	char buffer[512];

	if (*ppos != 0)
		goto out;

	if (IS_ERR_OR_NULL(drm_dev)) {
		DDPPR_ERR("%s, invalid drm dev\n", __func__);
		return -EINVAL;
	}

	crtc = list_first_entry(&(drm_dev)->mode_config.crtc_list,
				typeof(*crtc), head);
	if (IS_ERR_OR_NULL(crtc)) {
		DDPPR_ERR("find crtc fail\n");
		return -ENODEV;
	}
	val = mtk_drm_get_idle_check_interval(crtc);
	if (val == 0)
		return -ENODEV;

	n = scnprintf(buffer, 512, "%llu", val);
out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, buffer, n);
}

static const struct proc_ops idletime_proc_fops = {
	.proc_read = idletime_proc_get,
	.proc_write = idletime_proc_set,
	.proc_open = idletime_proc_open,
};

int disp_met_set(void *data, u64 val)
{
	/*1 enable  ; 0 disable*/
	disp_met_en = val;
	return 0;
}

static int disp_met_get(void *data, u64 *val)
{
	*val = disp_met_en;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(disp_met_fops, disp_met_get, disp_met_set, "%llu\n");

int disp_met_stop_set(void *data, u64 val)
{
	/*1 enable  ; 0 disable*/
	DDPMSG("MET Stop Condition list:\n");
	DDPMSG("    1: underrun\n");
	DDPMSG("    2: oddmr err\n");
	DDPMSG("    3: others\n");
	DDPMSG("%s: update met stop condition from:%u to %lu\n",
		__func__, disp_met_condition, val);

	disp_met_condition = val;

	switch (disp_met_condition) {
	case 1: //underrun
		clear_dsi_underrun_event();
		break;
	case 2: //oddmr err
		clear_oddmr_err_event();
		break;
	case 3: //others
		break;
	default:
		break;
	}

	return 0;
}

static int disp_met_stop_get(void *data, u64 *val)
{
	switch (disp_met_condition) {
	case 1: //underrun
		*val = check_dsi_underrun_event();
		break;
	case 2: //oddmr err
		*val = check_oddmr_err_event();
		break;
	case 3: //others
		*val = 0;
		break;
	default:
		*val = 0;
		break;
	}

	DDPMSG("%s: met stop at condition:%u:%lu\n",
		__func__, disp_met_condition, *val);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(disp_met_stop_fops, disp_met_stop_get, disp_met_stop_set, "%llu\n");

static int disp_met_proc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t disp_met_proc_set(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int ret;

	ret = kstrtouint_from_user(ubuf, count, 0, &disp_met_en);
	if (ret)
		return ret;

	return count;
}

static ssize_t disp_met_proc_get(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int n = 0;
	char buffer[512];

	if (*ppos != 0)
		goto out;

	n = scnprintf(buffer, 512, "%u", disp_met_en);
out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, buffer, n);
}

static int disp_lfr_dbg_proc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t disp_lfr_dbg_proc_set(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int ret;

	ret = kstrtouint_from_user(ubuf, count, 0, &lfr_dbg);
	if (ret)
		return ret;

	return count;
}

static ssize_t disp_lfr_dbg_proc_get(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int n = 0;
	char buffer[512];

	if (*ppos != 0)
		goto out;

	n = scnprintf(buffer, 512, "%u", lfr_dbg);
out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, buffer, n);
}

static int disp_lfr_params_proc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t disp_lfr_params_proc_set(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int ret;

	ret = kstrtouint_from_user(ubuf, count, 0, &lfr_params);
	if (ret)
		return ret;

	return count;
}

static ssize_t disp_lfr_params_proc_get(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int n = 0;
	char buffer[512];

	if (*ppos != 0)
		goto out;

	n = scnprintf(buffer, 512, "%u", lfr_params);
out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, buffer, n);
}

static const struct proc_ops disp_lfr_dbg_proc_fops = {
	.proc_read = disp_lfr_dbg_proc_get,
	.proc_write = disp_lfr_dbg_proc_set,
	.proc_open = disp_lfr_dbg_proc_open,
};

static const struct proc_ops disp_lfr_params_proc_fops = {
	.proc_read = disp_lfr_params_proc_get,
	.proc_write = disp_lfr_params_proc_set,
	.proc_open = disp_lfr_params_proc_open,
};

static const struct proc_ops disp_met_proc_fops = {
	.proc_read = disp_met_proc_get,
	.proc_write = disp_met_proc_set,
	.proc_open = disp_met_proc_open,
};

int disp_lfr_dbg_set(void *data, u64 val)
{
	lfr_dbg = val;
	return 0;
}

static int disp_lfr_dbg_get(void *data, u64 *val)
{
	*val = lfr_dbg;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(disp_lfr_dbg_fops, disp_lfr_dbg_get,
	disp_lfr_dbg_set, "%llu\n");

int disp_lfr_params_set(void *data, u64 val)
{

	lfr_params = val;
	return 0;
}

static int disp_lfr_params_get(void *data, u64 *val)
{
	*val = lfr_params;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(disp_lfr_params_fops, disp_lfr_params_get,
	disp_lfr_params_set, "%llu\n");

unsigned int mtk_dbg_get_lfr_mode_value(void)
{
	unsigned int lfr_mode = (lfr_params & 0x03);
	return lfr_mode;
}
unsigned int mtk_dbg_get_lfr_type_value(void)
{
	unsigned int lfr_type = (lfr_params & 0x0C) >> 2;
	return lfr_type;
}
unsigned int mtk_dbg_get_lfr_enable_value(void)
{
	unsigned int lfr_enable = (lfr_params & 0x10) >> 4;
	return lfr_enable;
}
unsigned int mtk_dbg_get_lfr_update_value(void)
{
	unsigned int lfr_update = (lfr_params & 0x20) >> 5;
	return lfr_update;
}
unsigned int mtk_dbg_get_lfr_vse_dis_value(void)
{
	unsigned int lfr_vse_dis = (lfr_params & 0x40) >> 6;
	return lfr_vse_dis;
}
unsigned int mtk_dbg_get_lfr_skip_num_value(void)
{
	unsigned int lfr_skip_num = (lfr_params & 0x3F00) >> 8;
	return lfr_skip_num;
}

unsigned int mtk_dbg_get_lfr_dbg_value(void)
{
	return lfr_dbg;
}

static void backup_vfp_for_lp_cust(u64 vfp)
{
		vfp_backup = vfp;
}

static u64 get_backup_vfp(void)
{
	return vfp_backup;
}

static int idlevfp_set(void *data, u64 val)
{
	if (val > 4095)
		val = 4095;

	backup_vfp_for_lp_cust((unsigned int)val);
	return 0;
}

static int idlevfp_get(void *data, u64 *val)
{
	*val = (u64)get_backup_vfp();
	return 0;
}

int hrt_lp_switch_get(void)
{
	return hrt_lp_switch;
}

DEFINE_SIMPLE_ATTRIBUTE(idlevfp_fops, idlevfp_get, idlevfp_set, "%llu\n");

static int idlevfp_proc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t idlevfp_proc_set(struct file *file, const char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	int ret;
	u64 val;

	ret = kstrtou64_from_user(ubuf, count, 0, &val);
	if (ret)
		return ret;

	if (val > 4095)
		val = 4095;

	backup_vfp_for_lp_cust(val);

	return count;
}

static ssize_t idlevfp_proc_get(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int n = 0;
	u64 val;
	char buffer[512];

	if (*ppos != 0)
		goto out;

	val = get_backup_vfp();

	n = scnprintf(buffer, 512, "%llu", val);
out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, buffer, n);
}

static const struct proc_ops idlevfp_proc_fops = {
	.proc_read = idlevfp_proc_get,
	.proc_write = idlevfp_proc_set,
	.proc_open = idlevfp_proc_open,
};

static int hrt_lp_proc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t hrt_lp_proc_set(struct file *file, const char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	int ret;
	u64 val;

	ret = kstrtou64_from_user(ubuf, count, 0, &val);
	if (ret)
		return ret;

	hrt_lp_switch = val;

	return count;
}

static ssize_t hrt_lp_proc_get(struct file *file, char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int n = 0;
	u64 val;
	char buffer[512];

	if (*ppos != 0)
		goto out;

	val = hrt_lp_switch;

	n = scnprintf(buffer, 512, "%llu", val);
out:
	if (n < 0)
		return -EINVAL;

	return simple_read_from_buffer(ubuf, count, ppos, buffer, n);
}

#if defined (CONFIG_DSI_PRE_OFF)
static int cust_ipanic(struct notifier_block *this, unsigned long event, void *ptr)
{
	ipanic_dsi_off();

	return 0;
}

static int cust_ipanic_die(struct notifier_block *self, unsigned long cmd, void *ptr)
{
	ipanic_dsi_off();

	return 0;
}

static struct notifier_block cust_panic_blk = {
	.notifier_call = cust_ipanic,
	.priority = INT_MAX,
};

static struct notifier_block cust_die_blk = {
	.notifier_call = cust_ipanic_die,
	.priority = INT_MAX,
};

#else
static int disp_ipanic(struct notifier_block *this, unsigned long event, void *ptr)
{
	ipanic_lcm_reset();

	return 0;
}

static int disp_ipanic_die(struct notifier_block *self, unsigned long cmd, void *ptr)
{
	ipanic_lcm_reset();

	return 0;
}

static struct notifier_block panic_blk = {
	.notifier_call = disp_ipanic,
	.priority = 1,
};

static struct notifier_block die_blk = {
	.notifier_call = disp_ipanic_die,
	.priority = 1,
};
#endif

static const struct proc_ops hrt_lp_proc_fops = {
	.proc_read = hrt_lp_proc_get,
	.proc_write = hrt_lp_proc_set,
	.proc_open = hrt_lp_proc_open,
};

void disp_dbg_probe(void)
{
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *d_folder;
	struct dentry *d_file;

	mtkfb_dbgfs = debugfs_create_file("mtkfb", S_IFREG | 0440, NULL,
					  NULL, &debug_fops);

	d_folder = debugfs_create_dir("displowpower", NULL);
	if (d_folder) {
		d_file = debugfs_create_file("idletime", S_IFREG | 0644,
					     d_folder, NULL, &idletime_fops);
	}

	d_folder = debugfs_create_dir("mtkfb_debug", NULL);
	if (d_folder) {
		d_file = debugfs_create_file("disp_met", S_IFREG | 0644,
					     d_folder, NULL, &disp_met_fops);
		d_file = debugfs_create_file("disp_met_stop", S_IFREG | 0644,
					     d_folder, NULL, &disp_met_stop_fops);
	}
	if (d_folder) {
		d_file = debugfs_create_file("disp_lfr_dbg",
			S_IFREG | 0644,	d_folder, NULL, &disp_lfr_dbg_fops);
		d_file = debugfs_create_file("disp_lfr_params",
			S_IFREG | 0644,	d_folder, NULL, &disp_lfr_params_fops);
	}
	if (logger_enable)
		init_log_buffer();

	drm_mmp_init();
#endif

#if IS_ENABLED(CONFIG_PROC_FS)
	mtkfb_procfs = proc_create("mtkfb", S_IFREG | 0440,
				   NULL,
				   &debug_proc_fops);
	if (!mtkfb_procfs) {
		DDPPR_ERR("[%s %d]failed to create mtkfb in /proc/disp_ddp\n",
			__func__, __LINE__);
		goto out;
	}

	disp_lowpower_proc = proc_mkdir("displowpower", NULL);
	if (!disp_lowpower_proc) {
		DDPPR_ERR("[%s %d]failed to create dir: /proc/displowpower\n",
			__func__, __LINE__);
		goto out;
	}

	if (!proc_create("idletime", S_IFREG | 0440,
			 disp_lowpower_proc, &idletime_proc_fops)) {
		DDPPR_ERR("[%s %d]failed to create idletime in /proc/displowpower\n",
			__func__, __LINE__);
		goto out;
	}

	if (!proc_create("idlevfp", S_IFREG | 0440,
		disp_lowpower_proc, &idlevfp_proc_fops)) {
		DDPPR_ERR("[%s %d]failed to create idlevfp in /proc/displowpower\n",
			__func__, __LINE__);
		goto out;
	}

	if (!proc_create("hrt_lp", S_IFREG | 0440,
		disp_lowpower_proc, &hrt_lp_proc_fops)) {
		DDPPR_ERR("[%s %d]failed to create hrt_lp in /proc/displowpower\n",
			__func__, __LINE__);
		goto out;
	}

	mtkfb_debug_procfs = proc_mkdir("mtkfb_debug", NULL);
	if (!mtkfb_debug_procfs) {
		DDPPR_ERR("[%s %d]failed to create dir: /proc/mtkfb_debug\n",
			__func__, __LINE__);
		goto out;
	}
	if (!proc_create("disp_met", S_IFREG | 0440,
		mtkfb_debug_procfs, &disp_met_proc_fops)) {
		DDPPR_ERR("[%s %d]failed to create idlevfp in /proc/mtkfb_debug/disp_met\n",
			__func__, __LINE__);
		goto out;
	}

	if (!proc_create("disp_lfr_dbg", S_IFREG | 0440,
		mtkfb_debug_procfs, &disp_lfr_dbg_proc_fops)) {
		DDPPR_ERR("[%s %d]failed to create idlevfp in /proc/mtkfb_debug/disp_lfr_dbg\n",
			__func__, __LINE__);
		goto out;
	}
	if (!proc_create("disp_lfr_params", S_IFREG | 0440,
		mtkfb_debug_procfs, &disp_lfr_params_proc_fops)) {
		DDPPR_ERR("[%s %d]failed to create idlevfp in /proc/mtkfb_debug/disp_lfr_params\n",
			__func__, __LINE__);
		goto out;
	}
#endif

	mtk_dp_debugfs_init();
#if defined (CONFIG_DSI_PRE_OFF)
	atomic_notifier_chain_register(&panic_notifier_list, &cust_panic_blk);
	register_die_notifier(&cust_die_blk);
#else
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	register_die_notifier(&die_blk);
#endif

out:
	return;
}

void disp_dbg_init(struct drm_device *dev)
{
	int i;

	if (IS_ERR_OR_NULL(dev))
		DDPMSG("%s, disp debug init with invalid dev\n", __func__);
	else
		DDPMSG("%s, disp debug init\n", __func__);

	drm_dev = dev;
	init_completion(&cwb_cmp);

	for (i = 0; i < MAX_CRTC; ++i)
		INIT_LIST_HEAD(&cb_data_list[i]);
}

void disp_dbg_deinit(void)
{
	if (debug_buffer)
		vfree(debug_buffer);
#if IS_ENABLED(CONFIG_DEBUG_FS)
	debugfs_remove(mtkfb_dbgfs);
#endif

#if IS_ENABLED(CONFIG_PROC_FS)
	if (mtkfb_procfs) {
		proc_remove(mtkfb_procfs);
		mtkfb_procfs = NULL;
	}
	if (disp_lowpower_proc) {
		proc_remove(disp_lowpower_proc);
		disp_lowpower_proc = NULL;
	}
#endif
	mtk_dp_debugfs_deinit();
}

void get_disp_dbg_buffer(unsigned long *addr, unsigned long *size,
	unsigned long *start)
{
	if (logger_enable)
		init_log_buffer();
	if (is_buffer_init) {
		*addr = (unsigned long)err_buffer[0];
		*size = (DEBUG_BUFFER_SIZE - 4096);
		*start = 0;
	} else {
		*addr = 0;
		*size = 0;
		*start = 0;
	}
}
