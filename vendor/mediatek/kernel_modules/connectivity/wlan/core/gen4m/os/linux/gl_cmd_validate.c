/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*! \file gl_cmd_validate.c
 *   \brief This file validates input arguments of Mediatek ioctl private
 *          command and NL80211 vendor string command, and contains the
 *          declaration of command corresponding handler and policy.
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_cmd_validate.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct CMD_VALIDATE_POLICY u8_policy[COMMON_CMD_SET_ARG_NUM(7)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(4)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(5)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(6)] = {.type = NLA_U8, .min = 0, .max = U8_MAX}
};

struct CMD_VALIDATE_POLICY u16_policy[COMMON_CMD_SET_ARG_NUM(7)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(4)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(5)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(6)] = {.type = NLA_U16, .min = 0, .max = U16_MAX}
};

struct CMD_VALIDATE_POLICY u32_policy[COMMON_CMD_SET_ARG_NUM(7)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(4)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(5)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(6)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY set_flag_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 1}
};

struct CMD_VALIDATE_POLICY ap_start_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = RUNNING_P2P_MODE,
				 .max = RUNNING_P2P_DEV_MODE}
};

struct CMD_VALIDATE_POLICY set_band_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = CMD_BAND_TYPE_AUTO,
				 .max = CMD_BAND_TYPE_ALL}
};

struct CMD_VALIDATE_POLICY set_country_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .min = 2, .max = 4}
};

#if (CFG_SUPPORT_IDC_CH_SWITCH == 1)
struct CMD_VALIDATE_POLICY set_cas_ex_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
#if (CFG_SUPPORT_WIFI_6G == 1)
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = BAND_2G4,
				 .max = BAND_6G},
#else
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = BAND_2G4,
				 .max = BAND_5G},
#endif
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U8, .min = 0, .max = U8_MAX}
};

struct CMD_VALIDATE_POLICY set_cas_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = U8_MAX}
};

#endif

struct CMD_VALIDATE_POLICY get_chnls_policy[COMMON_CMD_GET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .min = 2, .max = 3}
};

#if (CFG_SUPPORT_WFD == 1)
struct CMD_VALIDATE_POLICY set_miracast_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = MIRACAST_MODE_OFF,
				 .max = MIRACAST_MODE_SINK}
};
#endif

struct CMD_VALIDATE_POLICY set_mcr_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY get_mcr_policy[COMMON_CMD_GET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY set_test_mdoe_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U16,
				 .min = 0,
				 .max = 2011}
};

struct CMD_VALIDATE_POLICY set_test_cmd_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY get_test_result_policy[COMMON_CMD_GET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY set_acl_policy[COMMON_CMD_GET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = PARAM_CUSTOM_ACL_POLICY_DISABLE,
				 .max = PARAM_CUSTOM_ACL_POLICY_REMOVE}
};

struct CMD_VALIDATE_POLICY add_acl_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .len = 17}
};
#if CFG_SUPPORT_NAN
struct CMD_VALIDATE_POLICY set_faw_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
struct CMD_VALIDATE_POLICY rddreport_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 4}
};
#endif

#if CFG_WOW_SUPPORT
struct CMD_VALIDATE_POLICY get_wow_port_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 1},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U8, .min = 0, .max = 1}
};
#endif

#if CFG_SUPPORT_QA_TOOL
#if (CFG_SUPPORT_CONNAC3X == 0)
struct CMD_VALIDATE_POLICY get_rx_stats_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};
#else
struct CMD_VALIDATE_POLICY get_rx_stats_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U16, .min = 0, .max = U16_MAX}
};
#endif
#endif

struct CMD_VALIDATE_POLICY get_sta_idx_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .len = 17}
};

struct CMD_VALIDATE_POLICY get_wtbl_info_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 32}
};

struct CMD_VALIDATE_POLICY get_mib_info_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 2}
};

struct CMD_VALIDATE_POLICY get_cfg_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .min = 0, .max = 127}
};

struct CMD_VALIDATE_POLICY set_noise_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 3}
};

struct CMD_VALIDATE_POLICY set_pop_policy[COMMON_CMD_SET_ARG_NUM(4)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 3},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_U16, .min = 0, .max = U16_MAX}
};

struct CMD_VALIDATE_POLICY set_ed_policy[COMMON_CMD_SET_ARG_NUM(4)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 3},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_S8, .min = -44, .max = -77},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_S8, .min = -44, .max = -77}
};

struct CMD_VALIDATE_POLICY set_amsdu_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY set_p2p_policy[COMMON_CMD_SET_ARG_NUM(5)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 1},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(4)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY set_stanss_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 8}
};

#if CFG_WLAN_ASSISTANT_NVRAM
struct CMD_VALIDATE_POLICY set_nvram_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U16, .min = 0, .max = U16_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U16, .min = 0, .max = U16_MAX}
};

struct CMD_VALIDATE_POLICY get_nvram_policy[COMMON_CMD_GET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U16, .min = 0, .max = U16_MAX}
};
#endif

struct CMD_VALIDATE_POLICY thermal_protect_enable_policy[
		COMMON_CMD_SET_ARG_NUM(7)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(3)] = {.type = NLA_U8, .min = 0, .max = U8_MAX},
	[COMMON_CMD_ATTR_IDX(4)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(5)] = {.type = NLA_U32, .min = 0, .max = U32_MAX},
	[COMMON_CMD_ATTR_IDX(6)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY thermal_protect_info_policy[
		COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 2}
};

struct CMD_VALIDATE_POLICY set_dual_sta_usecase_policy[
		COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8, .min = 0, .max = 15}
};

struct CMD_VALIDATE_POLICY get_tsf_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = 0, .max = MAX_BSS_INDEX - 1}
};

struct CMD_VALIDATE_POLICY set_ml_probereq_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .len = 17},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct CMD_VALIDATE_POLICY set_trx_ba_size_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .min = 2, .max = 6},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U16, .min = 0, .max = U16_MAX}
};

#if (CFG_SUPPORT_802_11AX == 1)
struct CMD_VALIDATE_POLICY set_om_ch_bw_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = CH_BW_20,
				 .max = CH_BW_160}
};
#endif

#if CFG_CHIP_RESET_HANG
struct CMD_VALIDATE_POLICY set_rst_hang_policy[COMMON_CMD_SET_ARG_NUM(2)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_U8,
				 .min = SER_L0_HANG_RST_NONE,
				 .max = SER_L0_HANG_RST_CMD_TRG}
};
#endif

struct CMD_VALIDATE_POLICY reassoc_policy[COMMON_CMD_SET_ARG_NUM(3)] = {
	[COMMON_CMD_ATTR_IDX(1)] = {.type = NLA_STRING, .len = 17},
	[COMMON_CMD_ATTR_IDX(2)] = {.type = NLA_U32, .min = 0, .max = U32_MAX}
};

struct PRIV_CMD_HANDLER priv_cmd_handlers[] = {
	{
		.pcCmdStr  = CMD_AP_START,
		.pfHandler = priv_driver_set_ap_start,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = ap_start_policy,
		.u4PolicySize = ARRAY_SIZE(ap_start_policy)
	},
	{
		.pcCmdStr  = CMD_LINKSPEED,
		.pfHandler = priv_driver_get_linkspeed,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SETSUSPENDMODE,
		.pfHandler = priv_driver_set_suspend_mode,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SETBAND,
		.pfHandler = priv_driver_set_band,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_band_policy,
		.u4PolicySize = ARRAY_SIZE(set_band_policy)
	},
	{
		.pcCmdStr  = CMD_COUNTRY,
		.pfHandler = priv_driver_set_country,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_country_policy,
		.u4PolicySize = ARRAY_SIZE(set_country_policy)
	},
#if (CFG_SUPPORT_IDC_CH_SWITCH == 1)
	{
		.pcCmdStr  = CMD_CSA_EX_EVENT,
		.pfHandler = priv_driver_set_csa_ex_event,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_cas_ex_policy,
		.u4PolicySize = ARRAY_SIZE(set_cas_ex_policy)
	},
	{
		.pcCmdStr  = CMD_CSA_EX,
		.pfHandler = priv_driver_set_csa_ex,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_cas_ex_policy,
		.u4PolicySize = ARRAY_SIZE(set_cas_ex_policy)
	},
	{
		.pcCmdStr  = CMD_CSA,
		.pfHandler = priv_driver_set_csa,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_cas_policy,
		.u4PolicySize = ARRAY_SIZE(set_cas_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_GET_COUNTRY,
		.pfHandler = priv_driver_get_country,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CHANNELS,
		.pfHandler = priv_driver_get_channels,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_chnls_policy,
		.u4PolicySize = ARRAY_SIZE(get_chnls_policy)
	},
#if (CFG_SUPPORT_WFD == 1)
	{
		.pcCmdStr  = CMD_MIRACAST,
		.pfHandler = priv_driver_set_miracast,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_miracast_policy,
		.u4PolicySize = ARRAY_SIZE(set_miracast_policy)
	},
	{
		.pcCmdStr  = CMD_SETCASTMODE,
		.pfHandler = priv_driver_set_miracast,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_miracast_policy,
		.u4PolicySize = ARRAY_SIZE(set_miracast_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_SET_SW_CTRL,
		.pfHandler = priv_driver_set_sw_ctrl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(set_mcr_policy)
	},
#if (CFG_SUPPORT_RA_GEN == 1)
	{
		.pcCmdStr  = CMD_SET_FIXED_FALLBACK,
		.pfHandler = priv_driver_set_fixed_fallback,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_RA_DBG,
		.pfHandler = priv_driver_set_ra_debug_proc,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if (CFG_SUPPORT_TXPOWER_INFO == 1)
	{
		.pcCmdStr  = CMD_GET_TX_POWER_INFO,
		.pfHandler = priv_driver_get_txpower_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_TX_POWER_MANUAL_SET,
		.pfHandler = priv_driver_txpower_man_set,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	{
		.pcCmdStr  = CMD_SET_FIXED_RATE,
		.pfHandler = priv_driver_set_unified_fixed_rate,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_AUTO_RATE,
		.pfHandler = priv_driver_set_unified_auto_rate,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	{
		.pcCmdStr  = CMD_SET_MLO_AGC_TX,
		.pfHandler = priv_driver_set_unified_mlo_agc_tx,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_MLD_REC,
		.pfHandler = priv_driver_get_unified_mld_rec,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#else
	{
		.pcCmdStr  = CMD_SET_FIXED_RATE,
		.pfHandler = priv_driver_set_fixed_rate,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	{
		.pcCmdStr  = CMD_SET_PP_CAP_CTRL,
		.pfHandler = priv_driver_set_pp_cap_ctrl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_PP_ALG_CTRL,
		.pfHandler = priv_driver_set_pp_alg_ctrl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_SET_BOOSTCPU,
		.pfHandler = priv_driver_boostcpu,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	{
		.pcCmdStr  = CMD_SET_SNIFFER,
		.pfHandler = priv_driver_sniffer,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	{
		.pcCmdStr  = CMD_SET_MONITOR,
		.pfHandler = priv_driver_set_monitor,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_GET_SW_CTRL,
		.pfHandler = priv_driver_get_sw_ctrl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(get_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_SET_MCR,
		.pfHandler = priv_driver_set_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(set_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_GET_MCR,
		.pfHandler = priv_driver_get_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(get_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_SET_DRV_MCR,
		.pfHandler = priv_driver_set_drv_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(set_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_GET_DRV_MCR,
		.pfHandler = priv_driver_get_drv_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(get_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_GET_EMI_MCR,
		.pfHandler = priv_driver_get_emi_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(get_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_SET_UHW_MCR,
		.pfHandler = priv_driver_set_uhw_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(set_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_GET_UHW_MCR,
		.pfHandler = priv_driver_get_uhw_mcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_mcr_policy,
		.u4PolicySize = ARRAY_SIZE(get_mcr_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TEST_MODE,
		.pfHandler = priv_driver_set_test_mode,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_test_mdoe_policy,
		.u4PolicySize = ARRAY_SIZE(set_test_mdoe_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TEST_CMD,
		.pfHandler = priv_driver_set_test_cmd,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_test_cmd_policy,
		.u4PolicySize = ARRAY_SIZE(set_test_cmd_policy)
	},
	{
		.pcCmdStr  = CMD_GET_TEST_RESULT,
		.pfHandler = priv_driver_get_test_result,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(3),
		.policy    = get_test_result_policy,
		.u4PolicySize = ARRAY_SIZE(get_test_result_policy)
	},
	{
		.pcCmdStr  = CMD_GET_STA_STAT2,
		.pfHandler = priv_driver_get_sta_stat2,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_STA_STAT,
		.pfHandler = priv_driver_get_sta_stat,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_STA_RX_STAT,
		.pfHandler = priv_driver_show_rx_stat,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_POLICY_ACL,
		.pfHandler = priv_driver_set_acl_policy,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_acl_policy,
		.u4PolicySize = ARRAY_SIZE(set_acl_policy)
	},
	{
		.pcCmdStr  = CMD_ADD_ACL_ENTRY,
		.pfHandler = priv_driver_add_acl_entry,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = add_acl_policy,
		.u4PolicySize = ARRAY_SIZE(add_acl_policy)
	},
	{
		.pcCmdStr  = CMD_DEL_ACL_ENTRY,
		.pfHandler = priv_driver_del_acl_entry,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = add_acl_policy,
		.u4PolicySize = ARRAY_SIZE(add_acl_policy)
	},
	{
		.pcCmdStr  = CMD_SHOW_ACL_ENTRY,
		.pfHandler = priv_driver_show_acl_entry,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_CLEAR_ACL_ENTRY,
		.pfHandler = priv_driver_clear_acl_entry,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},

#if CFG_SUPPORT_NAN
	{
		.pcCmdStr  = CMD_NAN_START,
		.pfHandler = priv_driver_set_nan_start,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_NAN_GET_MASTER_IND,
		.pfHandler = priv_driver_get_master_ind,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_NAN_GET_RANGE,
		.pfHandler = priv_driver_get_range,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_FAW_RESET,
		.pfHandler = priv_driver_set_faw_reset,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_FAW_CONFIG,
		.pfHandler = priv_driver_set_faw_config,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_faw_policy,
		.u4PolicySize = ARRAY_SIZE(set_faw_policy)
	},
	{
		.pcCmdStr  = CMD_FAW_APPLY,
		.pfHandler = priv_driver_set_faw_apply,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if (CFG_SUPPORT_DFS_MASTER == 1)
	{
		.pcCmdStr  = CMD_SET_DFS_CHN_AVAILABLE,
		.pfHandler = priv_driver_set_dfs_channel_available,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SHOW_DFS_STATE,
		.pfHandler = priv_driver_show_dfs_state,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SHOW_DFS_HELP,
		.pfHandler = priv_driver_show_dfs_help,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SHOW_DFS_CAC_TIME,
		.pfHandler = priv_driver_show_dfs_cac_time,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_DFS_RDDREPORT,
		.pfHandler = priv_driver_rddreport,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = rddreport_policy,
		.u4PolicySize = ARRAY_SIZE(rddreport_policy)
	},
	{
		.pcCmdStr  = CMD_SET_DFS_RADARMODE,
		.pfHandler = priv_driver_radarmode,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_DFS_RADAREVENT,
		.pfHandler = priv_driver_radarevent,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_DFS_RDDOPCHNG,
		.pfHandler = priv_driver_set_rdd_op_mode,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(6),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif
#if CFG_SUPPORT_IDC_CH_SWITCH
	{
		.pcCmdStr  = CMD_SET_IDC_BMP,
		.pfHandler = priv_driver_set_idc_bmp,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(5),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif
#if CFG_WOW_SUPPORT
	{
		.pcCmdStr  = CMD_WOW_START,
		.pfHandler = priv_driver_set_wow,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_WOW_ENABLE,
		.pfHandler = priv_driver_set_wow_enable,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_WOW_PAR,
		.pfHandler = priv_driver_set_wow_par,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(7),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_WOW_UDP,
		.pfHandler = priv_driver_set_wow_udpport,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_WOW_TCP,
		.pfHandler = priv_driver_set_wow_tcpport,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_WOW_PORT,
		.pfHandler = priv_driver_get_wow_port,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = get_wow_port_policy,
		.u4PolicySize = ARRAY_SIZE(get_wow_port_policy)
	},
	{
		.pcCmdStr  = CMD_GET_WOW_REASON,
		.pfHandler = priv_driver_get_wow_reason,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if CFG_SUPPORT_MDNS_OFFLOAD
	{
		.pcCmdStr  = CMD_SHOW_MDNS_RECORD,
		.pfHandler = priv_driver_show_mdns_record,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_ENABLE_MDNS,
		.pfHandler = priv_driver_enable_mdns_offload,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_DISABLE_MDNS,
		.pfHandler = priv_driver_disable_mdns_offload,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_MDNS_SET_WAKE_FLAG,
		.pfHandler = priv_driver_set_mdns_wake_flag,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if TEST_CODE_FOR_MDNS
	{
		.pcCmdStr  = CMD_SEND_MDNS_RECORD,
		.pfHandler = priv_driver_send_mdns_record,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_ADD_MDNS_RECORD,
		.pfHandler = priv_driver_add_mdns_record,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = TEST_ADD_MDNS_RECORD,
		.pfHandler = priv_driver_test_add_mdns_record,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif
#endif
#endif
	{
		.pcCmdStr  = CMD_SET_ADV_PWS,
		.pfHandler = priv_driver_set_adv_pws,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_MDTIM,
		.pfHandler = priv_driver_set_mdtim,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#if CFG_SUPPORT_QA_TOOL
	{
		.pcCmdStr  = CMD_GET_RX_STATISTICS,
		.pfHandler = priv_driver_get_rx_statistics,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = get_rx_stats_policy,
		.u4PolicySize = ARRAY_SIZE(get_rx_stats_policy)
	},
#endif
#if CFG_SUPPORT_MSP
	{
		.pcCmdStr  = CMD_GET_STA_STATS,
		.pfHandler = priv_driver_get_sta_statistics,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_BSS_STATS,
		.pfHandler = priv_driver_get_bss_statistics,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_STA_IDX,
		.pfHandler = priv_driver_get_sta_index,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = get_sta_idx_policy,
		.u4PolicySize = ARRAY_SIZE(get_sta_idx_policy)
	},
	{
		.pcCmdStr  = CMD_GET_STA_INFO,
		.pfHandler = priv_driver_get_sta_info,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_WTBL_INFO,
		.pfHandler = priv_driver_get_wtbl_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = get_wtbl_info_policy,
		.u4PolicySize = ARRAY_SIZE(get_wtbl_info_policy)
	},
	{
		.pcCmdStr  = CMD_GET_MIB_INFO,
		.pfHandler = priv_driver_get_mib_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = get_mib_info_policy,
		.u4PolicySize = ARRAY_SIZE(get_mib_info_policy)
	},
	{
		.pcCmdStr  = CMD_SET_FW_LOG,
		.pfHandler = priv_driver_set_fw_log,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif

	{
		.pcCmdStr  = CMD_SET_CFG,
		.pfHandler = priv_driver_set_cfg,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CFG,
		.pfHandler = priv_driver_get_cfg,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_cfg_policy,
		.u4PolicySize = ARRAY_SIZE(get_cfg_policy)
	},
	{
		.pcCmdStr  = CMD_SET_EM_CFG,
		.pfHandler = priv_driver_set_em_cfg,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(3),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_EM_CFG,
		.pfHandler = priv_driver_get_em_cfg,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_CHIP,
		.pfHandler = priv_driver_set_chip_config,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CHIP,
		.pfHandler = priv_driver_get_chip_config,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_VERSION,
		.pfHandler = priv_driver_get_version,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CNM,
		.pfHandler = priv_driver_get_cnm,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CAPAB_RSDB,
		.pfHandler = priv_driver_get_capab_rsdb,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},

#if CFG_SUPPORT_DBDC
	{
		.pcCmdStr  = CMD_SET_DBDC,
		.pfHandler = priv_driver_set_dbdc,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_GET_QUE_INFO,
		.pfHandler = priv_driver_get_que_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_MEM_INFO,
		.pfHandler = priv_driver_get_mem_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_HIF_INFO,
		.pfHandler = priv_driver_get_hif_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_TP_INFO,
		.pfHandler = priv_driver_get_tp_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CH_RANK_LIST,
		.pfHandler = priv_driver_get_ch_rank_list,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_CH_DIRTINESS,
		.pfHandler = priv_driver_get_ch_dirtiness,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_EFUSE,
		.pfHandler = priv_driver_efuse_ops,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if defined(_HIF_SDIO) && (MTK_WCN_HIF_SDIO == 0)
	{
		.pcCmdStr  = CMD_CCCR,
		.pfHandler = priv_driver_cccr_ops,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_SUPPORT_ADVANCE_CONTROL
	{
		.pcCmdStr  = CMD_SET_NOISE,
		.pfHandler = priv_driver_set_noise,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_noise_policy,
		.u4PolicySize = ARRAY_SIZE(set_noise_policy)
	},
	{
		.pcCmdStr  = CMD_GET_NOISE,
		.pfHandler = priv_driver_get_noise,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_POP,
		.pfHandler = priv_driver_set_pop,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(4),
		.policy    = set_pop_policy,
		.u4PolicySize = ARRAY_SIZE(set_pop_policy)
	},
#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	{
		.pcCmdStr  = CMD_SET_ED,
		.pfHandler = priv_driver_set_ed,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(4),
		.policy    = set_ed_policy,
		.u4PolicySize = ARRAY_SIZE(set_ed_policy)
	},
	{
		.pcCmdStr  = CMD_GET_ED,
		.pfHandler = priv_driver_get_ed,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_SET_PD,
		.pfHandler = priv_driver_set_pd,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_MAX_RFGAIN,
		.pfHandler = priv_driver_set_maxrfgain,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_SET_DRV_SER,
		.pfHandler = priv_driver_set_drv_ser,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_SW_AMSDU_NUM,
		.pfHandler = priv_driver_set_amsdu_num,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_amsdu_policy,
		.u4PolicySize = ARRAY_SIZE(set_amsdu_policy)
	},
	{
		.pcCmdStr  = CMD_SET_SW_AMSDU_SIZE,
		.pfHandler = priv_driver_set_amsdu_size,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_amsdu_policy,
		.u4PolicySize = ARRAY_SIZE(set_amsdu_policy)
	},
#if CFG_ENABLE_WIFI_DIRECT
	{
		.pcCmdStr  = CMD_P2P_SET_PS,
		.pfHandler = priv_driver_set_p2p_ps,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_p2p_policy,
		.u4PolicySize = ARRAY_SIZE(set_p2p_policy)
	},
	{
		.pcCmdStr  = CMD_P2P_SET_NOA,
		.pfHandler = priv_driver_set_p2p_noa,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(5),
		.policy    = set_p2p_policy,
		.u4PolicySize = ARRAY_SIZE(set_p2p_policy)
	},
#endif
#ifdef UT_TEST_MODE
	{
		.pcCmdStr  = CMD_RUN_UT,
		.pfHandler = priv_driver_run_ut,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_GET_WIFI_TYPE,
		.pfHandler = priv_driver_get_wifi_type,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	{
		.pcCmdStr  = CMD_SET_PWR_CTRL,
		.pfHandler = priv_driver_set_power_control,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_WMT_RESET_API_SUPPORT
	{
		.pcCmdStr  = CMD_SET_WHOLE_CHIP_RESET,
		.pfHandler = priv_driver_trigger_whole_chip_reset,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_WFSYS_RESET,
		.pfHandler = priv_driver_trigger_wfsys_reset,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if (CFG_SUPPORT_CONNAC2X == 1)
	{
		.pcCmdStr  = CMD_GET_FWTBL_UMAC,
		.pfHandler = priv_driver_get_uwtbl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif
#if CFG_SUPPORT_CONNAC2X == 1 || CFG_SUPPORT_CONNAC3X == 1
	{
		.pcCmdStr  = CMD_GET_UWTBL,
		.pfHandler = priv_driver_get_uwtbl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_SHOW_TXD_INFO,
		.pfHandler = priv_driver_show_txd_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_GET_MU_RX_PKTCNT,
		.pfHandler = priv_driver_show_rx_stat,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_RUN_HQA,
		.pfHandler = priv_driver_run_hqa,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_CALIBRATION,
		.pfHandler = priv_driver_calibration,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = NULL,
		.u4PolicySize = 0
	},

#if CFG_SUPPORT_DBDC
	{
		.pcCmdStr  = CMD_SET_STA1NSS,
		.pfHandler = priv_driver_set_sta1ss,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_stanss_policy,
		.u4PolicySize = ARRAY_SIZE(set_stanss_policy)
	},
#endif
#if CFG_WLAN_ASSISTANT_NVRAM
	{
		.pcCmdStr  = CMD_SET_NVRAM,
		.pfHandler = priv_driver_set_nvram,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_nvram_policy,
		.u4PolicySize = ARRAY_SIZE(set_nvram_policy)
	},
	{
		.pcCmdStr  = CMD_GET_NVRAM,
		.pfHandler = priv_driver_get_nvram,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_nvram_policy,
		.u4PolicySize = ARRAY_SIZE(get_nvram_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_SUPPORT_NVRAM,
		.pfHandler = priv_driver_support_nvram,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if CFG_MTK_WIFI_SW_WFDMA
	{
		.pcCmdStr  = CMD_SET_SW_WFDMA,
		.pfHandler = priv_driver_set_sw_wfdma,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_GET_HAPD_CHANNEL,
		.pfHandler = priv_driver_get_hapd_channel,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	{
		.pcCmdStr  = CMD_SET_PWR_LEVEL,
		.pfHandler = priv_driver_set_pwr_level,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
	{
		.pcCmdStr  = CMD_SET_PWR_TEMP,
		.pfHandler = priv_driver_set_pwr_temp,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_THERMAL_PROTECT_ENABLE,
		.pfHandler = priv_driver_thermal_protect_enable,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(7),
		.policy    = thermal_protect_enable_policy,
		.u4PolicySize = ARRAY_SIZE(thermal_protect_enable_policy)
	},
	{
		.pcCmdStr  = CMD_THERMAL_PROTECT_DISABLE,
		.pfHandler = priv_driver_thermal_protect_disable,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(4),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_THERMAL_PROTECT_INFO,
		.pfHandler = priv_driver_thermal_protect_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = thermal_protect_info_policy,
		.u4PolicySize = ARRAY_SIZE(thermal_protect_info_policy)
	},
	{
		.pcCmdStr  = CMD_THERMAL_PROTECT_DUTY_INFO,
		.pfHandler = priv_driver_thermal_protect_duty_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = thermal_protect_info_policy,
		.u4PolicySize = ARRAY_SIZE(thermal_protect_info_policy)
	},
	{
		.pcCmdStr  = CMD_THERMAL_PROTECT_DUTY_CFG,
		.pfHandler = priv_driver_thermal_protect_duty_cfg,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(4),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_THERMAL_PROTECT_STATE_ACT,
		.pfHandler = priv_driver_thermal_protect_state_act,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(5),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_MDVT,
		.pfHandler = priv_driver_set_mdvt,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	{
		.pcCmdStr  = CMD_DBG_SHOW_MLD,
		.pfHandler = priv_driver_dump_mld,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_DBG_SHOW_MLD_BSS,
		.pfHandler = priv_driver_dump_mld_bss,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_DBG_SHOW_MLD_STA,
		.pfHandler = priv_driver_dump_mld_sta,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_SET_USE_CASE,
		.pfHandler = priv_driver_set_multista_use_case,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_dual_sta_usecase_policy,
		.u4PolicySize = ARRAY_SIZE(set_dual_sta_usecase_policy)
	},
	{
		.pcCmdStr  = CMD_GET_BAINFO,
		.pfHandler = priv_driver_get_bainfo,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if (CFG_SUPPORT_TSF_SYNC == 1)
	{
		.pcCmdStr  = CMD_GET_TSF_VALUE,
		.pfHandler = priv_driver_get_tsf_value,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = get_tsf_policy,
		.u4PolicySize = ARRAY_SIZE(get_tsf_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_GET_MCU_INFO,
		.pfHandler = priv_driver_get_mcu_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if (CFG_SUPPORT_DEBUG_SOP == 1)
	{
		.pcCmdStr  = CMD_GET_SLEEP_INFO,
		.pfHandler = priv_driver_get_sleep_dbg_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	{
		.pcCmdStr  = CMD_PRESET_LINKID,
		.pfHandler = priv_driver_preset_linkid,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_ML_PROBEREQ,
		.pfHandler = priv_driver_set_ml_probereq,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_ml_probereq_policy,
		.u4PolicySize = ARRAY_SIZE(set_ml_probereq_policy)
	},
	{
		.pcCmdStr  = CMD_GET_ML_CAPA,
		.pfHandler = priv_driver_get_ml_capa,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_GET_ML_PREFER_FREQ_LIST,
		.pfHandler = priv_driver_get_ml_prefer_freqlist,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_ML_2ND_FREQ,
		.pfHandler = priv_driver_get_ml_2nd_freq,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(3),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
	{
		.pcCmdStr  = CMD_GET_DPD_CACHE,
		.pfHandler = priv_driver_get_dpd_cache,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_COEX_CONTROL,
		.pfHandler = priv_driver_coex_ctrl,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#if (CFG_WIFI_GET_MCS_INFO == 1)
	{,
		.pcCmdStr  = CMD_GET_MCS_INFO,
		.pfHandler = priv_driver_get_mcs_info,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_GET_SER,
		.pfHandler = priv_driver_get_ser_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_GET_EMI,
		.pfHandler = priv_driver_get_emi_info,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(3),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
	{
		.pcCmdStr  = CMD_QUERY_THERMAL_TEMP,
		.pfHandler = priv_driver_query_thermal_temp,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if CFG_SUPPORT_WIFI_SYSDVT
	{
		.pcCmdStr  = CMD_SET_TXS_TEST,
		.pfHandler = priv_driver_txs_test,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_TXS_TEST_RESULT,
		.pfHandler = priv_driver_txs_test_result,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_RXV_TEST,
		.pfHandler = priv_driver_rxv_test,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_RXV_TEST_RESULT,
		.pfHandler = priv_driver_rxv_test_result,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	{
		.pcCmdStr  = CMD_SET_CSO_TEST,
		.pfHandler = priv_driver_cso_test,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_SET_TX_AC_TEST,
		.pfHandler = priv_driver_set_tx_test_ac,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TX_TEST,
		.pfHandler = priv_driver_set_tx_test,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u16_policy,
		.u4PolicySize = ARRAY_SIZE(u16_policy)
	},
	{
		.pcCmdStr  = CMD_SET_SKIP_CH_CHECK,
		.pfHandler = priv_driver_skip_legal_ch_check,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#if (CFG_SUPPORT_DMASHDL_SYSDVT)
	{
		.pcCmdStr  = CMD_SET_DMASHDL_DUMP,
		.pfHandler = priv_driver_show_dmashdl_allcr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_DMASHDL_DVT_ITEM,
		.pfHandler = priv_driver_dmashdl_dvt_item,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
#endif
#endif
#if CFG_AP_80211KVR_INTERFACE
	{
		.pcCmdStr  = CMD_BSS_STATUS_REPORT,
		.pfHandler = priv_driver_MulAPAgent_bss_status_report,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_BSS_REPORT_INFO,
		.pfHandler = priv_driver_MulAPAgent_bss_report_info,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_STA_REPORT_INFO,
		.pfHandler = priv_driver_MulAPAgent_sta_report_info,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_STA_MEASUREMENT_ENABLE,
		.pfHandler = priv_driver_MulAPAgent_sta_measurement_control,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_STA_MEASUREMENT_INFO,
		.pfHandler = priv_driver_MulAPAgent_sta_measurement_info,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_WHITELIST_STA,
		.pfHandler = priv_driver_MulAPAgent_set_white_sta,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_BLACKLIST_STA,
		.pfHandler = priv_driver_MulAPAgent_set_Black_sta,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_AP_80211K_SUPPORT
	{
		.pcCmdStr  = CMD_STA_BEACON_REQUEST,
		.pfHandler = priv_driver_MulAPAgent_beacon_report_request,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_AP_80211V_SUPPORT
	{
		.pcCmdStr  = CMD_STA_BTM_REQUEST,
		.pfHandler = priv_driver_MulAPAgent_BTM_request,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
	{
		.pcCmdStr  = CMD_SET_BF,
		.pfHandler = priv_driver_set_bf,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_NSS,
		.pfHandler = priv_driver_set_nss,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_stanss_policy,
		.u4PolicySize = ARRAY_SIZE(set_stanss_policy)
	},
	{
		.pcCmdStr  = CMD_SET_AMSDU_TX,
		.pfHandler = priv_driver_set_amsdu_tx,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_AMSDU_RX,
		.pfHandler = priv_driver_set_amsdu_rx,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_AMPDU_TX,
		.pfHandler = priv_driver_set_ampdu_tx,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_AMPDU_RX,
		.pfHandler = priv_driver_set_ampdu_rx,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_QOS,
		.pfHandler = priv_driver_set_qos,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
#if CFG_SUPPORT_CSI
	{
		.pcCmdStr  = CMD_SET_CSI,
		.pfHandler = priv_driver_set_csi,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	{
		.pcCmdStr  = CMD_SET_MUEDCA_OVERRIDE,
		.pfHandler = priv_driver_muedca_override,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_BA_SIZE,
		.pfHandler = priv_driver_set_ba_size,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u16_policy,
		.u4PolicySize = ARRAY_SIZE(u16_policy)
	},
	{
		.pcCmdStr  = CMD_SET_RX_BA_SIZE,
		.pfHandler = priv_driver_set_trx_ba_size,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_trx_ba_size_policy,
		.u4PolicySize = ARRAY_SIZE(set_trx_ba_size_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TX_BA_SIZE,
		.pfHandler = priv_driver_set_trx_ba_size,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = set_trx_ba_size_policy,
		.u4PolicySize = ARRAY_SIZE(set_trx_ba_size_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TP_TEST_MODE,
		.pfHandler = priv_driver_set_tp_test_mode,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TX_MCSMAP,
		.pfHandler = priv_driver_set_mcsmap,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TX_PPDU,
		.pfHandler = priv_driver_set_tx_ppdu,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_LDPC,
		.pfHandler = priv_driver_set_ldpc,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_FORCE_AMSDU_TX,
		.pfHandler = priv_driver_set_tx_force_amsdu,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_OM_CH_BW,
		.pfHandler = priv_driver_set_om_ch_bw,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_om_ch_bw_policy,
		.u4PolicySize = ARRAY_SIZE(set_om_ch_bw_policy)
	},
	{
		.pcCmdStr  = CMD_SET_OM_RX_NSS,
		.pfHandler = priv_driver_set_om_rx_nss,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_stanss_policy,
		.u4PolicySize = ARRAY_SIZE(set_stanss_policy)
	},
	{
		.pcCmdStr  = CMD_SET_OM_TX_NSS,
		.pfHandler = priv_driver_set_om_tx_nss,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_stanss_policy,
		.u4PolicySize = ARRAY_SIZE(set_stanss_policy)
	},
	{
		.pcCmdStr  = CMD_SET_OM_MU_DISABLE,
		.pfHandler = priv_driver_set_om_mu_dis,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_OM_MU_DATA_DISABLE,
		.pfHandler = priv_driver_set_om_mu_data_dis,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
	{
		.pcCmdStr  = CMD_SET_RxCtrlToMutiBss,
		.pfHandler = priv_driver_set_rx_ctrl_to_muti_bss,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#if (CFG_SUPPORT_802_11BE == 1)
	{
		.pcCmdStr  = CMD_SET_EHT_OM_MODE,
		.pfHandler = priv_driver_set_eht_om,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_SET_EHT_OM_RX_NSS_EXT,
		.pfHandler = priv_driver_set_eht_om_rx_nss_ext,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_EHT_OM_CH_BW_EXT,
		.pfHandler = priv_driver_set_eht_om_ch_bw_ext,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_EHT_OM_TX_NSTS_EXT,
		.pfHandler = priv_driver_set_eht_om_tx_nsts_ext,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TX_EHTMCSMAP,
		.pfHandler = priv_driver_set_ehtmcsmap,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = u32_policy,
		.u4PolicySize = ARRAY_SIZE(u32_policy)
	},
#endif
	{
		.pcCmdStr  = CMD_SET_TX_OM_PACKET,
		.pfHandler = priv_driver_set_tx_om_packet,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_TX_CCK_1M_PWR,
		.pfHandler = priv_driver_set_tx_cck_1m_pwr,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_SET_PAD_DUR,
		.pfHandler = priv_driver_set_pad_dur,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_SR_ENABLE,
		.pfHandler = priv_driver_set_sr_enable,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
	{
		.pcCmdStr  = CMD_GET_SR_CAP,
		.pfHandler = priv_driver_get_sr_cap,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
#else
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
#endif
	},
	{
		.pcCmdStr  = CMD_GET_SR_IND,
		.pfHandler = priv_driver_get_sr_ind,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
#else
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
#endif
	},
	{
		.pcCmdStr  = CMD_SET_PP_RX,
		.pfHandler = priv_driver_set_pp_rx,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_flag_policy,
		.u4PolicySize = ARRAY_SIZE(set_flag_policy)
	},
#endif
#if CFG_CHIP_RESET_HANG
	{
		.pcCmdStr  = CMD_SET_RST_HANG,
		.pfHandler = priv_driver_set_rst_hang,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = set_rst_hang_policy,
		.u4PolicySize = ARRAY_SIZE(set_rst_hang_policy)
	},
#endif
#if (CFG_SUPPORT_TWT == 1)
	{
		.pcCmdStr  = CMD_SET_TWT_PARAMS,
		.pfHandler = priv_driver_set_twtparams,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_SUPPORT_802_11K
	{
		.pcCmdStr  = CMD_NEIGHBOR_REQ,
		.pfHandler = priv_driver_neighbor_request,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	{
		.pcCmdStr  = CMD_BTM_QUERY,
		.pfHandler = priv_driver_bss_transition_query,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
#if CFG_SUPPORT_PCIE_GEN_SWITCH
	{
		.pcCmdStr  = CMD_SET_PCIE_SPEED,
		.pfHandler = priv_driver_set_pcie_speed,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
#endif
/*
 *	{
 *		.pcCmdStr  = <command string>,
 *		.pfHandler = <command handler>,
 *		.argPolicy = <argument count validation policy>,
 *			      VERIFY_EXACT_ARG_NUM : Verify the ucArgNum must
 *					equal to the argument count.
 *			      VERIFY_MIN_ARG_NUM: Verify the ucArgNum must less
 *					than or equal to the argument count.
 *		.ucArgNum  = <number for argument count check, include CMD>,
 *		.policy    = <argument type and range validation policy,
 *			      one entry for one argument>,
 *		.u4PolicySize = <array size of policy, use ARRAY_SIZE MARCO>
 *	},
 */
};

struct STR_CMD_HANDLER str_cmd_handlers[] = {
	{
		.pcCmdStr  = CMD_TDLS_PS,
		.pfHandler = testmode_disable_tdls_ps,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_NEIGHBOR_REQUEST,
		.pfHandler = testmode_neighbor_request,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_BSS_TRAN_QUERY,
		.pfHandler = testmode_bss_tran_query,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_OSHARE,
		.pfHandler = testmode_osharemod,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_EXAMPLE,
		.pfHandler = testmode_cmd_example,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(1),
		.policy    = NULL,
		.u4PolicySize = 0
	},
	{
		.pcCmdStr  = CMD_REASSOC,
		.pfHandler = testmode_reassoc,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(3),
		.policy    = reassoc_policy,
		.u4PolicySize = ARRAY_SIZE(reassoc_policy)
	},
	{
		.pcCmdStr  = CMD_SET_AX_BLACKLIST,
		.pfHandler = testmode_set_ax_blacklist,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_RTT_GET_CAP,
		.pfHandler = testmode_rtt_test,
		.argPolicy = VERIFY_MIN_ARG_NUM,
		.ucArgNum  = COMMON_CMD_GET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_REPORT_VENDOR_SPECIFIED,
		.pfHandler = testmode_set_report_vendor_specified,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(2),
		.policy    = u8_policy,
		.u4PolicySize = ARRAY_SIZE(u8_policy)
	},
	{
		.pcCmdStr  = CMD_SET_CUSTOM_TX_POWER_CALLING,
		.pfHandler = testmode_set_custom_tx_power_calling,
		.argPolicy = VERIFY_EXACT_ARG_NUM,
		.ucArgNum  = COMMON_CMD_SET_ARG_NUM(13),
		.policy    = NULL,
		.u4PolicySize = 0
	},
/*
 *	{
 *		.pcCmdStr  = <command string>,
 *		.pfHandler = <command handler>,
 *		.argPolicy = <argument count validation policy>,
 *			      VERIFY_EXACT_ARG_NUM : Verify the ucArgNum must
 *					equal to the argument count.
 *			      VERIFY_MIN_ARG_NUM: Verify the ucArgNum must less
 *					than or equal to the argument count.
 *		.ucArgNum  = <number for argument count check, include CMD>,
 *		.policy    = <argument type and range validation policy,
 *			      one entry for one argument>,
 *		.u4PolicySize = <array size of policy, use ARRAY_SIZE MARCO>
 *	},
 */
};

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
uint32_t cmd_validate(int8_t *pcCmd, enum ARG_NUM_POLICY argPolicy,
	uint8_t ucArgNum, struct CMD_VALIDATE_POLICY *policy,
	uint32_t u4PolicySize)
{
	uint8_t ucIdx = 0;
	uint32_t ret = WLAN_STATUS_FAILURE;
	int8_t *pcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Argc = 0;

	wlanCfgParseArgument(pcCmd, &i4Argc, pcArgv);

	/* 1. validate argument count */
	if (argPolicy == VERIFY_EXACT_ARG_NUM &&
		ucArgNum != i4Argc)
		return ret;
	else if (argPolicy == VERIFY_MIN_ARG_NUM &&
		ucArgNum > i4Argc)
		return ret;

	/* 2. validate arguments */
	if (policy == NULL) {
		ret = WLAN_STATUS_SUCCESS;
		return ret;
	}

	for (ucIdx = 1; ucIdx < ucArgNum && ucIdx < u4PolicySize; ucIdx++) {
		struct CMD_VALIDATE_POLICY *prAttr = &policy[ucIdx];

		if (!prAttr) {
			DBGLOG(REQ, INFO, "invalid attr(%d)\n", ucIdx);
			return ret;
		}
		DBGLOG(REQ, LOUD, "(%d) type[%d] len[%u] min[%u] max[%u]\n",
			ucIdx, prAttr->type, prAttr->len, prAttr->min,
			prAttr->max);
		switch (prAttr->type) {
		case NLA_U8:
		case NLA_U16:
		case NLA_U32:
		{
			uint32_t tmp;

			if (kalkStrtou32(pcArgv[ucIdx], 0, &tmp) != 0)
				return ret;
			DBGLOG(REQ, LOUD, ">> value[%u]\n", tmp);

			if (tmp >= prAttr->min && tmp <= prAttr->max)
				continue;
			else
				return ret;
			break;
		}
		case NLA_S8:
		case NLA_S16:
		case NLA_S32:
		{
			int tmp;

			if (kalStrtoint(pcArgv[ucIdx], 0, &tmp) != 0)
				return ret;
			DBGLOG(REQ, LOUD, ">> value[%d]\n", tmp);

			if (tmp >= prAttr->min && tmp <= prAttr->max)
				continue;
			else
				return ret;
			break;
		}
		case NLA_STRING:
		{
			uint8_t len = kalStrLen(pcArgv[ucIdx]);

			DBGLOG(REQ, LOUD, ">> len[%d]\n", len);

			if (prAttr->len != 0) {
				if (prAttr->len != len)
					return ret;
				continue;
			}

			if (len >= prAttr->min && len <= prAttr->max)
				continue;
			else
				return ret;
			break;
		}
		default: {
			DBGLOG(REQ, ERROR, "unknown type[%d]\n", prAttr->type);
			return ret;
		}
		}
	}
	ret = WLAN_STATUS_SUCCESS;
	DBGLOG(REQ, LOUD, "command validate pass\n");

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is used to find Mediatek ioctl private command handler,
 *        and check argument count and validate policy.
 *
 * @param[in] cmd	Mediatek ioctl private command
 * @param[in] len	Fixed buffer size containing private command
 *
 * @return Mediatek ioctl private command handler if found and validation pass
 * @return NULL if no matched command string or validation fail
 */
/*----------------------------------------------------------------------------*/
PRIV_CMD_FUNCTION get_priv_cmd_handler(uint8_t *cmd, int32_t len)
{
	uint8_t ucIdx = 0;
	uint32_t ret = WLAN_STATUS_FAILURE;
	int8_t *pcCmd;
	int32_t i4CmdSize;

	for (ucIdx = 0; ucIdx < sizeof(priv_cmd_handlers) / sizeof(struct
				PRIV_CMD_HANDLER); ucIdx++) {
		if (strnicmp(cmd, priv_cmd_handlers[ucIdx].pcCmdStr,
			     strlen(priv_cmd_handlers[ucIdx].pcCmdStr)) == 0) {

			i4CmdSize = len + 1;
			pcCmd = (int8_t *) kalMemAlloc(i4CmdSize, VIR_MEM_TYPE);
			if (!pcCmd) {
				DBGLOG(REQ, WARN,
					"%s, alloc mem failed\n", __func__);
				return 0;
			}
			kalMemZero(pcCmd, i4CmdSize);
			kalMemCopy(pcCmd, cmd, len);
			pcCmd[i4CmdSize - 1] = '\0';

			DBGLOG(REQ, LOUD,
				"ioctl priv command is [%s], argPolicy[%d] argNum[%d] u4PolicySize[%d]\n",
				pcCmd,
				priv_cmd_handlers[ucIdx].argPolicy,
				priv_cmd_handlers[ucIdx].ucArgNum,
				priv_cmd_handlers[ucIdx].u4PolicySize);

			ret = cmd_validate(pcCmd,
				priv_cmd_handlers[ucIdx].argPolicy,
				priv_cmd_handlers[ucIdx].ucArgNum,
				priv_cmd_handlers[ucIdx].policy,
				priv_cmd_handlers[ucIdx].u4PolicySize);

			if (pcCmd)
				kalMemFree(pcCmd, VIR_MEM_TYPE, i4CmdSize);

			if (ret != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN, "Command validate fail\n");
				return NULL;
			}
			return priv_cmd_handlers[ucIdx].pfHandler;
		}
	}
	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is used to find nl80211 vendor string command handler,
 *        and check argument count and validate policy.
 *
 * @param[in] cmd	Nl80211 vendor string command
 * @param[in] len	Exact length of vendor string command
 *
 * @return Nl80211 vendor string command handler if found and validation pass
 * @return NULL if no matched command string or validation fail
 */
/*----------------------------------------------------------------------------*/
STR_CMD_FUNCTION get_str_cmd_handler(uint8_t *cmd, int32_t len)
{
	uint8_t ucIdx = 0;
	uint32_t ret = WLAN_STATUS_FAILURE;
	int8_t *pcCmd;
	int32_t i4CmdSize;

	for (ucIdx = 0; ucIdx < sizeof(str_cmd_handlers) / sizeof(struct
				STR_CMD_HANDLER); ucIdx++) {
		if (len >= strlen(str_cmd_handlers[ucIdx].pcCmdStr) &&
			strnicmp(cmd, str_cmd_handlers[ucIdx].pcCmdStr,
			    strlen(str_cmd_handlers[ucIdx].pcCmdStr)) == 0) {

			i4CmdSize = len + 1;
			pcCmd = (int8_t *) kalMemAlloc(i4CmdSize, VIR_MEM_TYPE);
			if (!pcCmd) {
				DBGLOG(REQ, WARN,
					"%s, alloc mem failed\n", __func__);
				return 0;
			}
			kalMemZero(pcCmd, i4CmdSize);
			kalMemCopy(pcCmd, cmd, len);
			pcCmd[i4CmdSize - 1] = '\0';

			DBGLOG(REQ, LOUD,
				"vendor str command is [%s], argPolicy[%d] argNum[%d] u4PolicySize[%d]\n",
				pcCmd,
				str_cmd_handlers[ucIdx].argPolicy,
				str_cmd_handlers[ucIdx].ucArgNum,
				str_cmd_handlers[ucIdx].u4PolicySize);

			ret = cmd_validate(pcCmd,
				str_cmd_handlers[ucIdx].argPolicy,
				str_cmd_handlers[ucIdx].ucArgNum,
				str_cmd_handlers[ucIdx].policy,
				str_cmd_handlers[ucIdx].u4PolicySize);

			if (pcCmd)
				kalMemFree(pcCmd, VIR_MEM_TYPE, i4CmdSize);

			if (ret != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN, "Command validate fail\n");
				return NULL;
			}
			return str_cmd_handlers[ucIdx].pfHandler;
		}
	}
	return NULL;
}
