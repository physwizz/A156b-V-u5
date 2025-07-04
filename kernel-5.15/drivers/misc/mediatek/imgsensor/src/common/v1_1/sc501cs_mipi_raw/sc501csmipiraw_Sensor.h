/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 Samsung Electronics Co., Ltd.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     SC501CSmipi_Sensor.h
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     CMOS sensor header file
 *
 ****************************************************************************/
#ifndef __SC501CSMIPI_SENSOR_H__
#define __SC501CSMIPI_SENSOR_H__

/* mirror/flip */
#if IS_ENABLED(CONFIG_CAMERA_AAX_V15X) || IS_ENABLED(CONFIG_CAMERA_AAX_V15)
#define ENABLE_MIRROR_HV
#endif

/* OTP */
#define SC501CS_OTP_CHECK_BANK       0x80EC
#define SC501CS_OTP_BANK1_MARK       0x01
#define SC501CS_OTP_BANK2_MARK       0x03
#define SC501CS_OTP_BANK1_START_ADDR 0x80F0
#define SC501CS_OTP_BANK2_START_ADDR 0x8800

struct sc501cs_otp {
	kal_uint8  dd_flag;
	kal_uint16 dd_num;
	kal_uint8  reg_flag;
	kal_uint8  reg_num;
	kal_uint8  regs[10][3];
#if defined(SC501CS_OTP_CUSTOMER)
	kal_uint8  wb_flag;
	kal_uint16 rg_gain;
	kal_uint16 bg_gain;
	kal_uint8  golden_flag;
	kal_uint16 golden_rg;
	kal_uint16 golden_bg;
#endif
};// sc501cs_otp_struct;

enum IMGSENSOR_MODE {
	IMGSENSOR_MODE_INIT,
	IMGSENSOR_MODE_PREVIEW,
	IMGSENSOR_MODE_CAPTURE,
	IMGSENSOR_MODE_VIDEO,
	IMGSENSOR_MODE_HIGH_SPEED_VIDEO,
	IMGSENSOR_MODE_SLIM_VIDEO,
	IMGSENSOR_MODE_CUSTOM1,
	IMGSENSOR_MODE_CUSTOM2,
	IMGSENSOR_MODE_CUSTOM3,
	IMGSENSOR_MODE_CUSTOM4,
	IMGSENSOR_MODE_CUSTOM5,
	IMGSENSOR_MODE_MAX
};

struct setfile_mode_info {
	kal_uint16 *setfile;
	kal_uint32 size;
	char *name;
};

struct imgsensor_mode_struct {
	kal_uint32 pclk;                /* record different mode's pclk */
	kal_uint32 linelength;          /* record different mode's linelength */
	kal_uint32 framelength;         /* record different mode's framelength */
	kal_uint8  startx;              /* record different mode's startx of grabwindow */
	kal_uint8  starty;              /* record different mode's startx of grabwindow */
	kal_uint16 grabwindow_width;    /* record different mode's width of grabwindow */
	kal_uint16 grabwindow_height;   /* record different mode's height of grabwindow */
	kal_uint32 mipi_pixel_rate;
	/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario */
	kal_uint8  mipi_data_lp2hs_settle_dc;
	/* following for GetDefaultFramerateByScenario() */
	kal_uint16 max_framerate;
};

/* SENSOR PRIVATE STRUCT FOR VARIABLES */
struct imgsensor_struct {
	kal_uint8  mirror;                /* mirrorflip information */
	kal_uint8  sensor_mode;           /* record IMGSENSOR_MODE enum value */
	kal_uint32 shutter;               /* current shutter */
	kal_uint16 gain;                  /* current gain */
	kal_uint32 pclk;                  /* current pclk */
	kal_uint32 frame_length;          /* current framelength */
	kal_uint32 line_length;           /* current linelength */
	kal_uint32 min_frame_length;      /* current min  framelength to max framerate */
	kal_uint16 dummy_pixel;           /* current dummypixel */
	kal_uint16 dummy_line;            /* current dummline */
	kal_uint16 current_fps;           /* current max fps */
	kal_bool   autoflicker_en;        /* record autoflicker enable or disable */
	kal_bool   test_pattern;          /* record test pattern mode or not */
	enum MSDK_SCENARIO_ID_ENUM current_scenario_id; /* current scenario id */
	kal_uint8  ihdr_en;               /* ihdr enable or disable */
	kal_uint8  i2c_write_id;          /* record current sensor's i2c write id */
};

/* SENSOR PRIVATE STRUCT FOR CONSTANT */
struct imgsensor_info_struct {
	kal_uint32 sensor_id;                     /* record sensor id defined in Kd_imgsensor.h */
	kal_uint32 checksum_value;                /* checksum value for Camera Auto Test */
	struct imgsensor_mode_struct pre;         /* preview scenario relative information */
	struct imgsensor_mode_struct cap;         /* capture scenario relative information */
	struct imgsensor_mode_struct cap1;        /* capture for PIP 24fps relative information */
	/* capture1 mode must use same framelength, linelength with Capture mode for shutter calculate */
	struct imgsensor_mode_struct normal_video;/* normal video  scenario relative information */
	struct imgsensor_mode_struct hs_video;    /* high speed video scenario relative information */
	struct imgsensor_mode_struct slim_video;  /* slim video for VT scenario relative information */
	struct imgsensor_mode_struct custom1;     /* custom1 for stereo scenario relative information */
	struct imgsensor_mode_struct custom2;     /* custom2 for stereo scenario relative information */
	struct imgsensor_mode_struct custom3;     /* custom3 for stereo scenario relative information */
	struct imgsensor_mode_struct custom4;     /* custom4 for stereo scenario relative information */
	struct imgsensor_mode_struct custom5;     /* custom5 for stereo scenario relative information */

	struct imgsensor_mode_struct *mode_info[IMGSENSOR_MODE_MAX];
	bool is_invalid_mode;

	kal_uint8 ae_shut_delay_frame;            /* shutter delay frame for AE cycle */
	kal_uint8 ae_sensor_gain_delay_frame;     /* sensor gain delay frame for AE cycle */
	kal_uint8 ae_ispGain_delay_frame;         /* isp gain delay frame for AE cycle */
	kal_uint8 ihdr_support;                   /* 1, support; 0,not support */
	kal_uint8 temperature_support;            /* 1, support; 0,not support */
	kal_uint8 ihdr_le_firstline;              /* 1,le first ; 0, se first */
	kal_uint8 sensor_mode_num;                /* support sensor mode num */
	kal_uint8 cap_delay_frame;                /* enter capture delay frame num */
	kal_uint8 pre_delay_frame;                /* enter preview delay frame num */
	kal_uint8 video_delay_frame;              /* enter video delay frame num */
	kal_uint8 hs_video_delay_frame;           /* enter high speed video  delay frame num */
	kal_uint8 slim_video_delay_frame;         /* enter slim video delay frame num */
	kal_uint8 custom1_delay_frame;            /* enter custom1 delay frame num */
	kal_uint8 custom2_delay_frame;            /* enter custom2 delay frame num */
	kal_uint8 custom3_delay_frame;            /* enter custom3 delay frame num */
	kal_uint8 custom4_delay_frame;            /* enter custom4 delay frame num */
	kal_uint8 custom5_delay_frame;            /* enter custom5 delay frame num */
	kal_uint8 frame_time_delay_frame;         /* enter frame_time_delay_frame num */
	kal_uint8 margin;                         /* sensor framelength & shutter margin */
	kal_uint32 min_shutter;                   /* min shutter */
	kal_uint32 min_gain;
	kal_uint32 max_gain;
	kal_uint32 min_gain_iso;
	kal_uint32 exp_step;
	kal_uint32 gain_step;
	kal_uint32 gain_type;
	kal_uint32 max_frame_length;              /* max framelength by sensor register's limitation */
	kal_uint8 isp_driving_current;            /* mclk driving current */
	kal_uint8 sensor_interface_type;          /* sensor_interface_type */
	kal_uint8 mipi_sensor_type;
	/* 0, MIPI_OPHY_NCSI2; 1, MIPI_OPHY_CSI2, default is NCSI2, don't modify this para */
	kal_uint8 mipi_settle_delay_mode;
	/* 0, high speed signal auto detect; 1, use settle delay, unit is ns*/
	/* default is auto detect, don't modify this para */
	kal_uint8 sensor_output_dataformat;       /* sensor output first pixel color */
	kal_uint8 mclk;             /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	kal_uint8 mipi_lane_num;    /* mipi lane num */
	kal_uint8 i2c_addr_table[5];
	kal_uint32 i2c_speed;
	/* record sensor support all write id addr, only supprt 4must end with 0xff */
};

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int iMultiReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId, u8 number);
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId);
extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing);

#endif


