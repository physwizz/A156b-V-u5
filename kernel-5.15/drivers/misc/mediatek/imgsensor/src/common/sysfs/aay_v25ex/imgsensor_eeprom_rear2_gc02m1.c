// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Inc.
 */

#include "imgsensor_eeprom_rear2_gc02m1.h"

#define REAR2_GC02M1_MAX_CAL_SIZE                (0x07CF + 0x1) //actual size of cal

#define REAR2_GC02M1_HEADER_CHECKSUM_LEN        (0x00FB + 0x1)
#define REAR2_GC02M1_AWB_CHECKSUM_LEN           (0x011B - 0x0110 + 0x1)
#define REAR2_GC02M1_SHADING_CHECKSUM_LEN       (0x087B - 0x0120 + 0x1)

#define REAR2_GC02M1_CONVERTED_MAX_CAL_SIZE     (0x0B2F + 0x1 + 0x10) //margin(0x10) of cal conversion

const struct imgsensor_vendor_rom_addr rear2_gc02m1_cal_addr = {
	/* Set '-1' if not used */
	.camera_module_es_version               = 'A',
	.cal_map_es_version                     = '1',
	.rom_max_cal_size                       = REAR2_GC02M1_MAX_CAL_SIZE,
	.rom_header_cal_data_start_addr         = 0x00,
	.rom_header_main_module_info_start_addr = 0x5E,
	.rom_header_cal_map_ver_start_addr      = 0x88,
	.rom_header_project_name_start_addr     = 0x8C,
	.rom_header_module_id_addr              = 0xA8 + 0x06,
	.rom_header_main_sensor_id_addr         = 0xB8,

	.rom_header_sub_module_info_start_addr  = -1,
	.rom_header_sub_sensor_id_addr          = -1,

	.rom_header_main_header_start_addr      = 0x00,
	.rom_header_main_header_end_addr        = 0x04,
	.rom_header_main_oem_start_addr         = 0x08,
	.rom_header_main_oem_end_addr           = 0x0C,
	.rom_header_main_awb_start_addr         = 0x10,
	.rom_header_main_awb_end_addr           = 0x14,
	.rom_header_main_shading_start_addr     = 0x18,
	.rom_header_main_shading_end_addr       = 0x1C,
	.rom_header_main_sensor_cal_start_addr  = -1,
	.rom_header_main_sensor_cal_end_addr    = -1,
	.rom_header_dual_cal_start_addr         = -1,
	.rom_header_dual_cal_end_addr           = -1,
	.rom_header_pdaf_cal_start_addr         = -1,
	.rom_header_pdaf_cal_end_addr           = -1,
	.rom_header_ois_cal_start_addr          = -1,
	.rom_header_ois_cal_end_addr            = -1,

	.rom_header_sub_oem_start_addr          = -1,
	.rom_header_sub_oem_end_addr            = -1,
	.rom_header_sub_awb_start_addr          = -1,
	.rom_header_sub_awb_end_addr            = -1,
	.rom_header_sub_shading_start_addr      = -1,
	.rom_header_sub_shading_end_addr        = -1,

	.rom_header_main_mtf_data_addr          = -1,// It must be in af area, not in factory area in cal data
	.rom_header_sub_mtf_data_addr           = -1,// It must be in af area, not in factory area in cal data

	.rom_header_checksum_addr               = 0x00FC,
	.rom_header_checksum_len                = REAR2_GC02M1_HEADER_CHECKSUM_LEN,

	.rom_oem_af_inf_position_addr           = -1,
	.rom_oem_af_macro_position_addr         = -1,
	.rom_oem_module_info_start_addr         = -1,
	.rom_oem_checksum_addr                  = -1,
	.rom_oem_checksum_len                   = -1,

	.rom_module_cal_data_start_addr         = -1,
	.rom_module_module_info_start_addr      = -1,
	.rom_module_checksum_addr               = -1,
	.rom_module_checksum_len                = -1,

	.rom_awb_cal_data_start_addr            = 0x0110,
	.rom_awb_module_info_start_addr         = -1,
	.rom_awb_checksum_addr                  = 0x011C,
	.rom_awb_checksum_len                   = REAR2_GC02M1_AWB_CHECKSUM_LEN,

	.rom_shading_cal_data_start_addr        = 0x0120,
	.rom_shading_module_info_start_addr     = -1,
	.rom_shading_checksum_addr              = 0x087C,
	.rom_shading_checksum_len               = REAR2_GC02M1_SHADING_CHECKSUM_LEN,

	.rom_sensor_cal_module_info_start_addr  = -1,
	.rom_sensor_cal_checksum_addr           = -1,
	.rom_sensor_cal_checksum_len            = -1,

	.rom_dual_module_info_start_addr        = -1,
	.rom_dual_checksum_addr                 = -1,
	.rom_dual_checksum_len                  = -1,

	.rom_pdaf_module_info_start_addr        = -1,
	.rom_pdaf_checksum_addr                 = -1,
	.rom_pdaf_checksum_len                  = -1,

	.rom_ois_checksum_addr                  = -1,
	.rom_ois_checksum_len                   = -1,

	.rom_sub_oem_af_inf_position_addr       = -1,
	.rom_sub_oem_af_macro_position_addr     = -1,
	.rom_sub_oem_module_info_start_addr     = -1,
	.rom_sub_oem_checksum_addr              = -1,
	.rom_sub_oem_checksum_len               = -1,

	.rom_sub_awb_module_info_start_addr     = -1,
	.rom_sub_awb_checksum_addr              = -1,
	.rom_sub_awb_checksum_len               = -1,

	.rom_sub_shading_module_info_start_addr = -1,
	.rom_sub_shading_checksum_addr          = -1,
	.rom_sub_shading_checksum_len           = -1,

	.rom_dual_cal_data2_start_addr          = -1,
	.rom_dual_cal_data2_size                = -1,
	.rom_dual_tilt_x_addr                   = -1,
	.rom_dual_tilt_y_addr                   = -1,
	.rom_dual_tilt_z_addr                   = -1,
	.rom_dual_tilt_sx_addr                  = -1,
	.rom_dual_tilt_sy_addr                  = -1,
	.rom_dual_tilt_range_addr               = -1,
	.rom_dual_tilt_max_err_addr             = -1,
	.rom_dual_tilt_avg_err_addr             = -1,
	.rom_dual_tilt_dll_version_addr         = -1,
	.rom_dual_tilt_dll_modelID_addr         = -1,
	.rom_dual_tilt_dll_modelID_size         = -1,
	.rom_dual_shift_x_addr                  = -1,
	.rom_dual_shift_y_addr                  = -1,

	.extend_cal_addr                        = NULL,

	.converted_cal_addr                     = NULL,
	.rom_converted_max_cal_size             = REAR2_GC02M1_CONVERTED_MAX_CAL_SIZE,

	.sensor_maker                           = "GALAXYCORE",
	.sensor_name                            = "GC02M1",
	.sub_sensor_maker                       = NULL,
	.sub_sensor_name                        = NULL,

	.bayerformat                            = -1,
};
