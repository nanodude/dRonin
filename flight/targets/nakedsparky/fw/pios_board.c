/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup NakedSparky support files
 * @{
 *
 * @file       pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      The board specific initialization routines
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */

#include "board_hw_defs.c"

#include <pios.h>
#include <pios_hal.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include "hwnakedsparky.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"
#include "flightbatterysettings.h"

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS5611)
#include "pios_ms5611_priv.h"
static const struct pios_ms5611_cfg pios_ms5611_cfg = {
	.oversampling = MS5611_OSR_1024,
	.temperature_interleaving = 1,
};
#endif /* PIOS_INCLUDE_MS5611 */

/**
 * Configuration for the MPU6050 chip
 */
#if defined(PIOS_INCLUDE_MPU6050)
#include "pios_mpu6050.h"
static const struct pios_exti_cfg pios_exti_mpu6050_cfg __exti_config = {
	.vector = PIOS_MPU6050_IRQHandler,
	.line = EXTI_Line15,
	.pin = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin = GPIO_Pin_15,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = EXTI15_10_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line = EXTI_Line15, // matches above GPIO pin
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_mpu60x0_cfg pios_mpu6050_cfg = {
	.exti_cfg = &pios_exti_mpu6050_cfg,
	.default_samplerate = 500,
	.interrupt_cfg = PIOS_MPU60X0_INT_CLR_ANYRD,
	.interrupt_en = PIOS_MPU60X0_INTEN_DATA_RDY,
	.User_ctl = 0,
	.Pwr_mgmt_clk = PIOS_MPU60X0_PWRMGMT_PLL_Z_CLK,
	.default_filter = PIOS_MPU60X0_LOWPASS_188_HZ,
	.orientation = PIOS_MPU60X0_TOP_180DEG
};
#endif /* PIOS_INCLUDE_MPU6050 */

/**
 * Configuration for the MPU9150 chip
 */
#if defined(PIOS_INCLUDE_MPU9150)
#include "pios_mpu9150.h"
static const struct pios_exti_cfg pios_exti_mpu9150_cfg __exti_config = {
	.vector = PIOS_MPU9150_IRQHandler,
	.line = EXTI_Line15,
	.pin = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin = GPIO_Pin_15,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = EXTI15_10_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line = EXTI_Line15, // matches above GPIO pin
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_mpu60x0_cfg pios_mpu9150_cfg = {
	.exti_cfg = &pios_exti_mpu9150_cfg,
	.default_samplerate = 500,
	.interrupt_cfg = PIOS_MPU60X0_INT_CLR_ANYRD,
	.interrupt_en = PIOS_MPU60X0_INTEN_DATA_RDY,
	.User_ctl = 0,
	.Pwr_mgmt_clk = PIOS_MPU60X0_PWRMGMT_PLL_Z_CLK,
	.default_filter = PIOS_MPU60X0_LOWPASS_188_HZ,
	.orientation = PIOS_MPU60X0_TOP_180DEG,
};
#endif /* PIOS_INCLUDE_MPU9150 */

#define PIOS_COM_CAN_RX_BUF_LEN 256
#define PIOS_COM_CAN_TX_BUF_LEN 256

uintptr_t pios_com_aux_id;
uintptr_t pios_com_can_id;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_internal_adc_id;
uintptr_t pios_can_id;
uintptr_t pios_com_openlog_logging_id;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{

	/* Delay system */
	PIOS_DELAY_Init();

	const struct pios_board_info *bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg *led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_CAN)
	if (PIOS_CAN_Init(&pios_can_id, &pios_can_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN);

	if (PIOS_COM_Init(&pios_com_can_id, &pios_can_com_driver, pios_can_id,
	                  PIOS_COM_CAN_RX_BUF_LEN,
	                  PIOS_COM_CAN_TX_BUF_LEN))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN);

	pios_com_bridge_id = pios_com_can_id;
#endif

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);

	/* Register the partition table */
	const struct pios_flash_partition *flash_partition_table;
	uint32_t num_partitions;
	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(bdinfo->board_rev, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);
#if defined(ERASE_FLASH)
	PIOS_FLASHFS_Format(pios_uavo_settings_fs_id);
#endif

#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the hardware UAVOs */
	HwNakedSparkyInitialize();
	ModuleSettingsInitialize();

#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

	/* Initialize watchdog as early as possible to catch faults during init
	 * but do it only if there is no debugger connected
	 */
	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
		PIOS_WDG_Init();
	}

	/* Set up pulse timers */
	//inputs

	//outputs
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_2_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_15_cfg);
	PIOS_TIM_InitClock(&tim_16_cfg);
	PIOS_TIM_InitClock(&tim_17_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwNakedSparkySetDefaults(HwNakedSparkyHandle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(), 0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}

#if defined(PIOS_INCLUDE_USB)
	/* Initialize board specific USB data */
	PIOS_USB_BOARD_DATA_Init();

	/* Flags to determine if various USB interfaces are advertised */
	bool usb_hid_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
	bool usb_cdc_present = false;
	if (PIOS_USB_DESC_HID_CDC_Init()) {
		PIOS_Assert(0);
	}
	usb_hid_present = true;
	usb_cdc_present = true;
#else
	if (PIOS_USB_DESC_HID_ONLY_Init()) {
		PIOS_Assert(0);
	}
	usb_hid_present = true;
#endif

	uintptr_t pios_usb_id;
	PIOS_USB_Init(&pios_usb_id, PIOS_BOARD_HW_DEFS_GetUsbCfg(bdinfo->board_rev));

#if defined(PIOS_INCLUDE_USB_CDC)

	uint8_t hw_usb_vcpport;
	/* Configure the USB VCP port */
	HwNakedSparkyUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWNAKEDSPARKY_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwNakedSparkyUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWNAKEDSPARKY_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg))
		PIOS_DEBUG_Assert(0);

	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_INT);
	else
		if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
#endif  // PIOS_INCLUDE_I2C

	HwNakedSparkyDSMxModeOptions hw_DSMxMode;
	HwNakedSparkyDSMxModeGet(&hw_DSMxMode);

	/* Configure main USART port */
	uint8_t hw_mainport;
	HwNakedSparkyMainPortGet(&hw_mainport);

	PIOS_HAL_ConfigurePort(hw_mainport,          // port type protocol
	        &pios_main_usart_cfg,                // usart_port_cfg
	        &pios_usart_com_driver,              // com_driver
	        NULL,                                // i2c_id
	        NULL,                                // i2c_cfg
	        NULL,                                // ppm_cfg
	        NULL,                                // pwm_cfg
	        PIOS_LED_ALARM,                      // led_id
	        &pios_main_dsm_aux_cfg,              // dsm_cfg
	        hw_DSMxMode,                         // dsm_mode
	        NULL);                               // sbus_cfg

	/* Configure FlexiPort */
	uint8_t hw_flexiport;
	HwNakedSparkyFlexiPortGet(&hw_flexiport);

	PIOS_HAL_ConfigurePort(hw_flexiport,         // port type protocol
	        &pios_flexi_usart_cfg,               // usart_port_cfg
	        &pios_usart_com_driver,              // com_driver
	        &pios_i2c_flexi_id,                  // i2c_id
	        &pios_i2c_flexi_cfg,                 // i2c_cfg
	        NULL,                                // ppm_cfg
	        NULL,                                // pwm_cfg
	        PIOS_LED_ALARM,                      // led_id
	        &pios_flexi_dsm_aux_cfg,             // dsm_cfg
	        hw_DSMxMode,                         // dsm_mode
	        NULL);                               // sbus_cfg

	/* Configure the rcvr port */
	uint8_t hw_rcvrport;
	HwNakedSparkyRcvrPortGet(&hw_rcvrport);

	PIOS_HAL_ConfigurePort(hw_rcvrport,          // port type protocol
	        &pios_rcvr_usart_cfg,                // usart_port_cfg
	        &pios_usart_com_driver,              // com_driver
	        NULL,                                // i2c_id
	        NULL,                                // i2c_cfg
	        &pios_ppm_cfg,                       // ppm_cfg
	        NULL,                                // pwm_cfg
	        PIOS_LED_ALARM,                      // led_id
	        &pios_rcvr_dsm_aux_cfg,              // dsm_cfg
	        hw_DSMxMode,                         // dsm_mode
	        NULL);                               // sbus_cfg

#if defined(PIOS_INCLUDE_GCSRCVR)
	GCSReceiverInitialize();
	uintptr_t pios_gcsrcvr_id;
	PIOS_GCSRCVR_Init(&pios_gcsrcvr_id);
	uintptr_t pios_gcsrcvr_rcvr_id;
	if (PIOS_RCVR_Init(&pios_gcsrcvr_rcvr_id, &pios_gcsrcvr_rcvr_driver, pios_gcsrcvr_id)) {
		PIOS_Assert(0);
	}
	pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_GCS] = pios_gcsrcvr_rcvr_id;
#endif	/* PIOS_INCLUDE_GCSRCVR */

	uint8_t hw_auxport;
	uint8_t number_of_pwm_outputs;
	uint8_t number_of_adc_ports;
	bool use_pwm_in;
	HwNakedSparkyAuxPortGet(&hw_auxport);
	switch (hw_auxport) {
	case HWNAKEDSPARKY_AUXPORT_PWM10:
		number_of_pwm_outputs = 10;
		number_of_adc_ports = 0;
		use_pwm_in = false;
		break;
	case HWNAKEDSPARKY_AUXPORT_PWM82ADC:
		number_of_pwm_outputs = 8;
		number_of_adc_ports = 2;
		use_pwm_in = false;
		break;
	case HWNAKEDSPARKY_AUXPORT_PWM73ADC:
		number_of_pwm_outputs = 7;
		number_of_adc_ports = 3;
		use_pwm_in = false;
		break;
	case HWNAKEDSPARKY_AUXPORT_PWM9PWM_IN:
		number_of_pwm_outputs = 9;
		use_pwm_in = true;
		number_of_adc_ports = 0;
		break;
	case HWNAKEDSPARKY_AUXPORT_PWM72ADCPWM_IN:
		number_of_pwm_outputs = 7;
		use_pwm_in = true;
		number_of_adc_ports = 2;
		break;
	default:
		PIOS_Assert(0);
		break;
	}
	number_of_adc_ports += 1; // First always on for internal battery monitoring

#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
#ifdef PIOS_INCLUDE_SERVO
	pios_servo_cfg.num_channels = number_of_pwm_outputs;

	if (hw_rcvrport != HWSHARED_PORTTYPES_PPM) {
		PIOS_Servo_Init(&pios_servo_cfg);
	} else {
		PIOS_Servo_Init(&pios_servo_slow_cfg);
	}
#endif
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif

#if defined(PIOS_INCLUDE_ADC)
	if (number_of_adc_ports < 3)
		internal_adc_cfg.adc_dev_slave = NULL;
	internal_adc_cfg.adc_pin_count = number_of_adc_ports;
	uint32_t internal_adc_id;
	if (PIOS_INTERNAL_ADC_Init(&internal_adc_id, &internal_adc_cfg) < 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_ADC);
	if (PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id) < 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_ADC);

	// Brutalization follows
	// Set voltage calibration value but only if at UAVO default
	// Value = 1000 * 4.53 / (4.53 + 68) converted to float32
	FlightBatterySettingsInitialize();
	FlightBatterySettingsData bs;
	FlightBatterySettingsGet(&bs);
	if(bs.SensorCalibrationFactor[FLIGHTBATTERYSETTINGS_SENSORCALIBRATIONFACTOR_VOLTAGE] == 63.69f) {
		bs.SensorCalibrationFactor[FLIGHTBATTERYSETTINGS_SENSORCALIBRATIONFACTOR_VOLTAGE] = 62.456913f;
		bs.VoltagePin = FLIGHTBATTERYSETTINGS_VOLTAGEPIN_ADC0;
		bs.CurrentPin = FLIGHTBATTERYSETTINGS_CURRENTPIN_NONE;
	}
	FlightBatterySettingsSet(&bs);
#endif /* PIOS_INCLUDE_ADC */

#if defined(PIOS_INCLUDE_PWM)
	if (use_pwm_in) {
		if (number_of_adc_ports > 1)
			pios_pwm_cfg.channels = &pios_tim_rcvrport_pwm[1];
		uintptr_t pios_pwm_id;
		PIOS_PWM_Init(&pios_pwm_id, &pios_pwm_cfg);

		uintptr_t pios_pwm_rcvr_id;
		if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
			PIOS_Assert(0);
		}
		pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
	}
#endif	/* PIOS_INCLUDE_PWM */
	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MPU9150)
#if defined(PIOS_INCLUDE_MPU6050)
	// Enable autoprobing when both 6050 and 9050 compiled in
	bool mpu9150_found = false;
	if (PIOS_MPU9150_Probe(pios_i2c_internal_id, PIOS_MPU9150_I2C_ADD_A0_LOW) == 0) {
		mpu9150_found = true;
#else
	{
#endif /* PIOS_INCLUDE_MPU6050 */

		uint8_t magnetometer;
		HwNakedSparkyMagnetometerGet(&magnetometer);

		if (magnetometer == HWNAKEDSPARKY_MAGNETOMETER_INTERNAL)
			use_mpu_mag = true;
		else
			use_mpu_mag = false;


		int retval;
		retval = PIOS_MPU9150_Init(pios_i2c_internal_id, PIOS_MPU9150_I2C_ADD_A0_LOW, &pios_mpu9150_cfg, use_mpu_mag);
		if (retval == -10)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
		if (retval != 0)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

		// To be safe map from UAVO enum to driver enum
		uint8_t hw_gyro_range;
		HwNakedSparkyGyroRangeGet(&hw_gyro_range);
		switch (hw_gyro_range) {
		case HWNAKEDSPARKY_GYRORANGE_250:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
			break;
		case HWNAKEDSPARKY_GYRORANGE_500:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
			break;
		case HWNAKEDSPARKY_GYRORANGE_1000:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
			break;
		case HWNAKEDSPARKY_GYRORANGE_2000:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
			break;
		}

		uint8_t hw_accel_range;
		HwNakedSparkyAccelRangeGet(&hw_accel_range);
		switch (hw_accel_range) {
		case HWNAKEDSPARKY_ACCELRANGE_2G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
			break;
		case HWNAKEDSPARKY_ACCELRANGE_4G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
			break;
		case HWNAKEDSPARKY_ACCELRANGE_8G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
			break;
		case HWNAKEDSPARKY_ACCELRANGE_16G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
			break;
		}

		uint8_t hw_mpu9150_dlpf;
		HwNakedSparkyMPU9150DLPFGet(&hw_mpu9150_dlpf);
		enum pios_mpu60x0_filter mpu9150_dlpf = \
		                                        (hw_mpu9150_dlpf == HWNAKEDSPARKY_MPU9150DLPF_188) ? PIOS_MPU60X0_LOWPASS_188_HZ : \
		                                        (hw_mpu9150_dlpf == HWNAKEDSPARKY_MPU9150DLPF_98) ? PIOS_MPU60X0_LOWPASS_98_HZ : \
		                                        (hw_mpu9150_dlpf == HWNAKEDSPARKY_MPU9150DLPF_42) ? PIOS_MPU60X0_LOWPASS_42_HZ : \
		                                        (hw_mpu9150_dlpf == HWNAKEDSPARKY_MPU9150DLPF_20) ? PIOS_MPU60X0_LOWPASS_20_HZ : \
		                                        (hw_mpu9150_dlpf == HWNAKEDSPARKY_MPU9150DLPF_10) ? PIOS_MPU60X0_LOWPASS_10_HZ : \
		                                        (hw_mpu9150_dlpf == HWNAKEDSPARKY_MPU9150DLPF_5) ? PIOS_MPU60X0_LOWPASS_5_HZ : \
		                                        pios_mpu9150_cfg.default_filter;
		PIOS_MPU9150_SetLPF(mpu9150_dlpf);
	}

#endif /* PIOS_INCLUDE_MPU9150 */

#if defined(PIOS_INCLUDE_MPU6050)
#if defined(PIOS_INCLUDE_MPU9150)
	// MPU9150 looks like an MPU6050 _plus_ additional hardware.  So we cannot try and
	// probe if MPU9150 is found or we will find a duplicate
	if (mpu9150_found == false)
#endif /* PIOS_INCLUDE_MPU9150 */
	{
		if (PIOS_MPU6050_Init(pios_i2c_internal_id, PIOS_MPU6050_I2C_ADD_A0_LOW, &pios_mpu6050_cfg) != 0)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
		if (PIOS_MPU6050_Test() != 0)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

		// To be safe map from UAVO enum to driver enum
		uint8_t hw_gyro_range;
		HwNakedSparkyGyroRangeGet(&hw_gyro_range);
		switch (hw_gyro_range) {
		case HWNAKEDSPARKY_GYRORANGE_250:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
			break;
		case HWNAKEDSPARKY_GYRORANGE_500:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
			break;
		case HWNAKEDSPARKY_GYRORANGE_1000:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
			break;
		case HWNAKEDSPARKY_GYRORANGE_2000:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
			break;
		}

		uint8_t hw_accel_range;
		HwNakedSparkyAccelRangeGet(&hw_accel_range);
		switch (hw_accel_range) {
		case HWNAKEDSPARKY_ACCELRANGE_2G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
			break;
		case HWNAKEDSPARKY_ACCELRANGE_4G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
			break;
		case HWNAKEDSPARKY_ACCELRANGE_8G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
			break;
		case HWNAKEDSPARKY_ACCELRANGE_16G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
			break;
		}
	}

#endif /* PIOS_INCLUDE_MPU6050 */

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

	uint8_t hw_magnetometer;
	HwNakedSparkyMagnetometerGet(&hw_magnetometer);

	switch (hw_magnetometer) {
		case HWNAKEDSPARKY_MAGNETOMETER_NONE:
		case HWNAKEDSPARKY_MAGNETOMETER_INTERNAL:
			/* internal handled by MPU code above */
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
			if (hw_flexiport == HWSHARED_PORTTYPES_I2C) {
				uint8_t hw_orientation;
				HwNakedSparkyExtMagOrientationGet(&hw_orientation);

				PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
					&pios_i2c_flexi_id, &pios_i2c_flexi_cfg);
			} else {
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			}
			break;
	}

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
	if ((PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id) != 0)
			|| (PIOS_MS5611_Test() != 0))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_serial_id || pios_com_telem_usb_id);
}

/**
 * @}
 */
