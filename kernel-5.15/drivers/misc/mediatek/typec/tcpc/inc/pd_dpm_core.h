/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef PD_DPM_CORE_H
#define PD_DPM_CORE_H

#include "tcpci.h"
#include "pd_core.h"
#include "pd_process_evt.h"

/* ---- MISC ---- */
int pd_dpm_core_init(struct pd_port *pd_port);
int pd_dpm_enable_vconn(struct pd_port *pd_port, bool en);
int pd_dpm_send_sink_caps(struct pd_port *pd_port);
int pd_dpm_send_source_caps(struct pd_port *pd_port);
void pd_dpm_inform_cable_id(struct pd_port *pd_port, bool ack,
			    bool src_startup);
void pd_dpm_inform_cable_svids(struct pd_port *pd_port, bool ack);
void pd_dpm_inform_cable_modes(struct pd_port *pd_port, bool ack);
void pd_dpm_dynamic_enable_vconn(struct pd_port *pd_port);
void pd_dpm_dynamic_disable_vconn(struct pd_port *pd_port);

/* ---- SNK ---- */

#if CONFIG_USB_PD_REV30_PPS_SINK
void pd_dpm_start_pps_request(struct pd_port *pd_port, bool en);
#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */

int pd_dpm_update_tcp_request(struct pd_port *pd_port,
	struct tcp_dpm_pd_request *pd_req);

int pd_dpm_update_tcp_request_ex(struct pd_port *pd_port,
	struct tcp_dpm_pd_request_ex *pd_req);

int pd_dpm_update_tcp_request_again(struct pd_port *pd_port);

void pd_dpm_snk_evaluate_caps(struct pd_port *pd_port);
void pd_dpm_snk_standby_power(struct pd_port *pd_port);
void pd_dpm_snk_transition_power(struct pd_port *pd_port);
void pd_dpm_snk_hard_reset(struct pd_port *pd_port);

/* ---- SRC ---- */

void pd_dpm_src_evaluate_request(struct pd_port *pd_port);
void pd_dpm_src_transition_power(struct pd_port *pd_port);
void pd_dpm_src_hard_reset(struct pd_port *pd_port);

/* ---- UFP : Handle VDM Request ---- */

void pd_dpm_ufp_request_id_info(struct pd_port *pd_port);
void pd_dpm_ufp_request_svid_info(struct pd_port *pd_port);
void pd_dpm_ufp_request_mode_info(struct pd_port *pd_port);
void pd_dpm_ufp_request_enter_mode(struct pd_port *pd_port);
void pd_dpm_ufp_request_exit_mode(struct pd_port *pd_port);

/* ---- UFP : DP Only ---- */

int pd_dpm_ufp_request_dp_status(struct pd_port *pd_port);
int pd_dpm_ufp_request_dp_config(struct pd_port *pd_port);
void pd_dpm_ufp_send_dp_attention(struct pd_port *pd_port);

/* ---- DFP : Inform VDM Result ---- */

void pd_dpm_dfp_inform_id(struct pd_port *pd_port, bool ack);
void pd_dpm_dfp_inform_svids(struct pd_port *pd_port, bool ack);
void pd_dpm_dfp_inform_modes(struct pd_port *pd_port, bool ack);
void pd_dpm_dfp_inform_enter_mode(struct pd_port *pd_port, bool ack);
void pd_dpm_dfp_inform_exit_mode(struct pd_port *pd_port);
void pd_dpm_dfp_inform_attention(struct pd_port *pd_port);

/* ---- DFP : DP Only  ---- */

void pd_dpm_dfp_send_dp_status_update(struct pd_port *pd_port);
void pd_dpm_dfp_inform_dp_status_update(struct pd_port *pd_port, bool ack);

void pd_dpm_dfp_send_dp_configuration(struct pd_port *pd_port);
void pd_dpm_dfp_inform_dp_configuration(struct pd_port *pd_port, bool ack);

/* ---- SVDM/UVDM  ---- */

void pd_dpm_ufp_recv_uvdm(struct pd_port *pd_port);
void pd_dpm_dfp_send_uvdm(struct pd_port *pd_port);
void pd_dpm_dfp_inform_uvdm(struct pd_port *pd_port, bool ack);

void pd_dpm_ufp_send_svdm_nak(struct pd_port *pd_port);

/* ---- DRP : Inform PowerCap ---- */

void pd_dpm_dr_inform_sink_cap(struct pd_port *pd_port);
void pd_dpm_dr_inform_source_cap(struct pd_port *pd_port);

/* ---- DRP : Data Role Swap ---- */

void pd_dpm_drs_evaluate_swap(struct pd_port *pd_port, uint8_t role);
void pd_dpm_drs_change_role(struct pd_port *pd_port, uint8_t role);

/* ---- DRP : Power Role Swap ---- */

void pd_dpm_prs_evaluate_swap(struct pd_port *pd_port, uint8_t role);
void pd_dpm_prs_turn_off_power_sink(struct pd_port *pd_port);
void pd_dpm_prs_enable_power_source(struct pd_port *pd_port, bool en);
void pd_dpm_prs_change_role(struct pd_port *pd_port, uint8_t role);

/* ---- DRP : Vconn Swap ---- */

void pd_dpm_vcs_evaluate_swap(struct pd_port *pd_port);
void pd_dpm_vcs_enable_vconn(struct pd_port *pd_port, uint8_t role);


/*
 * PE : PD3.0
 */

#if CONFIG_USB_PD_REV30

#if CONFIG_USB_PD_REV30_SRC_CAP_EXT_LOCAL
int pd_dpm_send_source_cap_ext(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_SRC_CAP_EXT_LOCAL */

#if CONFIG_USB_PD_REV30_SRC_CAP_EXT_REMOTE
void pd_dpm_inform_source_cap_ext(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_SRC_CAP_EXT_REMOTE */

#if CONFIG_USB_PD_REV30_BAT_CAP_LOCAL
int pd_dpm_send_battery_cap(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_BAT_CAP_LOCAL */

#if CONFIG_USB_PD_REV30_BAT_CAP_REMOTE
void pd_dpm_inform_battery_cap(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_BAT_CAP_REMOTE */

#if CONFIG_USB_PD_REV30_BAT_STATUS_LOCAL
int pd_dpm_send_battery_status(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_BAT_STATUS_LOCAL */

#if CONFIG_USB_PD_REV30_BAT_STATUS_REMOTE
void pd_dpm_inform_battery_status(struct pd_port *pd_port);
#endif	/* #if CONFIG_USB_PD_REV30_BAT_STATUS_REMOTE */

#if CONFIG_USB_PD_REV30_MFRS_INFO_LOCAL
int pd_dpm_send_mfrs_info(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_MFRS_INFO_LOCAL */

#if CONFIG_USB_PD_REV30_MFRS_INFO_REMOTE
void pd_dpm_inform_mfrs_info(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_MFRS_INFO_REMOTE */

#if CONFIG_USB_PD_REV30_COUNTRY_CODE_LOCAL
int pd_dpm_send_country_codes(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_COUNTRY_CODE_LOCAL */

#if CONFIG_USB_PD_REV30_COUNTRY_CODE_REMOTE
void pd_dpm_inform_country_codes(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_COUNTRY_CODE_REMOTE */

#if CONFIG_USB_PD_REV30_COUNTRY_INFO_LOCAL
int pd_dpm_send_country_info(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_COUNTRY_INFO_LOCAL */

#if CONFIG_USB_PD_REV30_COUNTRY_INFO_REMOTE
void pd_dpm_inform_country_info(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_COUNTRY_INFO_REMOTE */

#if CONFIG_USB_PD_REV30_ALERT_LOCAL
int pd_dpm_send_alert(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_ALERT_LOCAL */

#if CONFIG_USB_PD_REV30_ALERT_REMOTE
void pd_dpm_inform_alert(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_ALERT_REMOTE */

#if CONFIG_USB_PD_REV30_STATUS_LOCAL
int pd_dpm_send_status(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_STATUS_LOCAL */

#if CONFIG_USB_PD_REV30_STATUS_REMOTE
void pd_dpm_inform_status(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_STATUS_REMOTE */

#if CONFIG_USB_PD_REV30_PPS_SINK
void pd_dpm_inform_pps_status(struct pd_port *pd_port);
#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */

void pd_dpm_inform_sink_cap_ext(struct pd_port *pd_port);
int pd_dpm_send_sink_cap_ext(struct pd_port *pd_port);

void pd_dpm_inform_not_support(struct pd_port *pd_port);

void pd_dpm_inform_revision(struct pd_port *pd_port);
int pd_dpm_send_revision(struct pd_port *pd_port);

#endif	/* CONFIG_USB_PD_REV30 */

/* PE : Notify DPM */

int pd_dpm_notify_pe_startup(struct pd_port *pd_port);
int pd_dpm_notify_pe_hardreset(struct pd_port *pd_port);

/* TCPCI - VBUS Control */

static inline int pd_dpm_check_vbus_valid(struct pd_port *pd_port)
{
	return tcpci_check_vbus_valid(pd_port->tcpc);
}

static inline int pd_dpm_sink_vbus(struct pd_port *pd_port, bool en)
{
	int mv = en ? TCPC_VBUS_SINK_5V : TCPC_VBUS_SINK_0V;

	return tcpci_sink_vbus(pd_port->tcpc,
				TCP_VBUS_CTRL_REQUEST, mv, -1);
}

static inline int pd_dpm_source_vbus(struct pd_port *pd_port, bool en)
{
	int mv = en ? TCPC_VBUS_SOURCE_5V : TCPC_VBUS_SOURCE_0V;

	return tcpci_source_vbus(pd_port->tcpc,
				TCP_VBUS_CTRL_REQUEST, mv, -1);
}

/* Mode Operations */

extern bool dp_dfp_u_notify_discover_id(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data, bool ack);

extern bool dp_dfp_u_notify_discover_svids(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data, bool ack);

extern bool dp_dfp_u_notify_discover_modes(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data, bool ack);

extern bool dp_dfp_u_notify_enter_mode(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data, uint8_t ops, bool ack);

extern bool dp_dfp_u_notify_exit_mode(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data, uint8_t ops);

extern bool dp_dfp_u_notify_attention(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data);

extern bool dp_dfp_u_notify_discover_cable_id(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data, bool ack, uint32_t *payload,
	uint8_t cnt);

extern bool dp_dfp_u_notify_discover_cable_svids(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data, bool ack, uint32_t *payload,
	uint8_t cnt);

extern bool dp_dfp_u_notify_discover_cable_modes(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data, bool ack, uint32_t *payload,
	uint8_t cnt);

extern bool dp_ufp_u_request_enter_mode(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data, uint8_t ops);

extern bool dp_ufp_u_request_exit_mode(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data, uint8_t ops);

extern bool dp_dfp_u_notify_pe_startup(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data);

extern bool dp_dfp_u_notify_pe_ready(struct pd_port *pd_port,
	struct svdm_svid_data *svid_data);

extern bool dp_reset_state(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data);

extern bool dp_parse_svid_data(
	struct pd_port *pd_port, struct svdm_svid_data *svid_data);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
enum sec_dfp_state {
	SEC_DFP_NONE = 0,
	SEC_DFP_DISCOVER_ID,
	SEC_DFP_DISCOVER_SVIDS,
	SEC_DFP_DISCOVER_MODES,
	SEC_DFP_ENTER_MODE,
	SEC_DFP_ENTER_MODE_DONE,
	SEC_DFP_ERR,
};
extern void sec_dfp_accessory_detach_handler(struct pd_port *pd_port);
extern bool sec_dfp_notify_discover_id(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data, bool ack);
extern bool sec_dfp_notify_discover_svid(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data, bool ack);
extern bool sec_dfp_notify_discover_modes(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data, bool ack);
extern bool sec_dfp_notify_enter_mode(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data, uint8_t ops, bool ack);
extern bool sec_dfp_notify_pe_startup(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data);
extern bool sec_dfp_notify_pe_ready(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data);
extern bool sec_dfp_notify_uvdm(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data, bool ack);
extern bool sec_dfp_parse_svid_data(struct pd_port *pd_port,
		struct svdm_svid_data *svid_data);
extern int sec_dfp_uvdm_ready(void);
extern void sec_dfp_uvdm_close(void);
extern int sec_dfp_uvdm_out_request_message(void *data, int size);
extern int sec_dfp_uvdm_in_request_message(void *data);
#endif	/* CONFIG_PDIC_NOTIFIER */

/**
 * pd_dpm_get_ready_reaction
 *
 * Get a pending event from the reaction's table definied by DPM.
 *
 * Returns TCP_DPM_EVT_ID if succeeded;
 * Returns Zero to indicate there is no pending event.
 * Returns DPM_READY_REACTION_BUSY to indicate
 *	waiting for previous reaction is finished.
 */

#define DPM_READY_REACTION_BUSY		(0xff)
extern uint8_t pd_dpm_get_ready_reaction(struct pd_port *pd_port);

/* ---- DPM reactions ---- */



/* If receive reject/wait, cancel reaction */
#define DPM_REACTION_REQUEST_PR_SWAP		BIT(0)
#define DPM_REACTION_REQUEST_DR_SWAP		BIT(1)
#define DPM_REACTION_GET_SINK_CAP		BIT(2)
#define DPM_REACTION_GET_SOURCE_CAP		BIT(3)
#define DPM_REACTION_ATTEMPT_GET_FLAG		BIT(4)
#define DPM_REACTION_REQUEST_VCONN_SRC		BIT(5)
#define DPM_REACTION_RETURN_VCONN_SRC		BIT(6)
#define DPM_REACTION_REJECT_CANCEL		BIT(7)	/* FLAG ONLY */

#define DPM_REACTION_DFP_FLOW_DELAY		BIT(8)
#define DPM_REACTION_UFP_FLOW_DELAY		BIT(9)
#define DPM_REACTION_VCONN_STABLE_DELAY		BIT(10)

#define DPM_REACTION_DISCOVER_ID		BIT(11)
#define DPM_REACTION_DISCOVER_SVIDS		BIT(12)
#define DPM_REACTION_DISCOVER_CABLE		BIT(13)
#define DPM_REACTION_DYNAMIC_VCONN		BIT(14)

#define DPM_REACTION_DISCOVER_CABLE_FLOW	(\
	DPM_REACTION_DISCOVER_CABLE | \
	DPM_REACTION_REQUEST_VCONN_SRC |\
	DPM_REACTION_VCONN_STABLE_DELAY |\
	DPM_REACTION_DYNAMIC_VCONN)

#define DPM_REACTION_GET_SOURCE_CAP_EXT		BIT(15)

#define DPM_REACTION_CAP_RESET_CABLE		BIT(28)
#define DPM_REACTION_CAP_READY_ONCE		BIT(29)
#define DPM_REACTION_CAP_ALWAYS			BIT(31)

static inline void dpm_reaction_clear(struct pd_port *pd_port, uint32_t mask)
{
	pd_port->pe_data.dpm_ready_reactions &= ~mask;
}

static inline void dpm_reaction_set(struct pd_port *pd_port, uint32_t mask)
{
	struct tcpc_device *tcpc = pd_port->tcpc;

	pd_port->pe_data.dpm_ready_reactions |= mask;
	tcpc_event_thread_wake_up(tcpc);
}

static inline void dpm_reaction_set_ready_once(struct pd_port *pd_port)
{
	if (pd_check_pe_state_ready(pd_port))
		dpm_reaction_set(pd_port, DPM_REACTION_CAP_READY_ONCE);
}

static inline void dpm_reaction_set_clear(
	struct pd_port *pd_port, uint32_t set, uint32_t clear)
{
	struct tcpc_device *tcpc = pd_port->tcpc;
	uint32_t val = pd_port->pe_data.dpm_ready_reactions | set;

	pd_port->pe_data.dpm_ready_reactions = val & (~clear);
	tcpc_event_thread_wake_up(tcpc);
}

static inline uint32_t dpm_reaction_check(
		struct pd_port *pd_port, uint32_t mask)
{
	return pd_port->pe_data.dpm_ready_reactions & mask;
}
#endif /* PD_DPM_CORE_H */
