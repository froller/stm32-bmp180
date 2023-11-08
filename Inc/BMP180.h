/*
 * BMP180_hal.h
 *
 *  Created on: Jan 16, 2023
 *      Author: froller
 */

#ifndef INC_BMP180_H_
#define INC_BMP180_H_

#include "stm32f1xx_hal.h"

#define I2C_TIMEOUT 1000U

/**
 * @enum BMP180_SensorTypeDef
 * Exact sensor chip type.
 */
typedef enum {
  BMP085 = 85,
  BMP180 = 180,
  BMP183 = 183
} BMP180_SensorTypeDef;

/**
 * @enum BMP180_MeasurementTypeDef
 * Measurement type. Either Air temperature or atmospheric pressure.
 */
typedef enum {
  BMP180_MEASUREMENT_TEMPERATURE = 0x0E, /*!< Air temperature */
  BMP180_MEASUREMENT_PRESSURE    = 0x14  /*!< Atmospheric pressure */
} BMP180_MeasurementTypeDef;

/**
 * @enum BMP180_ConversionStatusTypeDef
 * Conversion status.
 */
typedef enum {
  BMP180_CONVERSION_IDLE    = 0, /*!< Conversion is complete. Data registers can be read. */
  BMP180_CONVERSION_RUNNING = 1, /*!< Conversion is in progress. Data registers contain inconsistent data. */
} BMP180_ConversionStatusTypeDef;

/**
 * @enum BMP180_OversamplingTypeDef
 * Oversampling settings.
 * Different setting require different conversion time and average current consumption.
 */
typedef enum {
  BMP180_OVERSAMPLING_0 = 0, /*!< 1 sample, 4.5ms, 3uA */
  BMP180_OVERSAMPLING_1 = 1, /*!< 2 samples, 7.5ms, 5uA */
  BMP180_OVERSAMPLING_2 = 2, /*!< 4 samples, 13.5ms, 7uA */
  BMP180_OVERSAMPLING_3 = 3, /*!< 8 samples, 22.5ms, 12uA */
  BMP180_PRECISION_LOW       = BMP180_OVERSAMPLING_0, /*!< Low precision. @see BMP180_OVERSAMPLING_0 */
  BMP180_PRECISION_STANDARD  = BMP180_OVERSAMPLING_1, /*!< Standard precision. @see BMP180_OVERSAMPLING_1 */
  BMP180_PRECISION_HIGH      = BMP180_OVERSAMPLING_2, /*!< High precision. @see BMP180_OVERSAMPLING_2 */
  BMP180_PRECISION_ULTRAHIGH = BMP180_OVERSAMPLING_3  /*!   < Ultra high precision. @see BMP180_OVERSAMPLING_3 */
} BMP180_OversamplingTypeDef;

#pragma pack(push, 1)

/**
 * @struct BMP180_CalibrationDataTypeDef
 * BMP180 calibration values.
 */
typedef struct {
  short AC1;
  short AC2;
  short AC3;
  unsigned short AC4;
  unsigned short AC5;
  unsigned short AC6;
  short B1;
  short B2;
  short MB;
  short MC;
  short MD;
} BMP180_CalibrationDataTypeDef;

/**
 * @struct BMP180_ControlRegisterTypeDef
 * BMP180 control register bit-fields
 */
typedef struct {
  BMP180_MeasurementTypeDef measurement:5; /**< Type of measurement. BMP180_MEASUREMENT_TEMPERATURE or BMP180_MEASUREMENT_PRESSURE */
  BMP180_ConversionStatusTypeDef sco:1; /**< Conversion status */
  BMP180_OversamplingTypeDef oss:2; /**< Oversampling settings */
} BMP180_ControlRegisterTypeDef;

#pragma pack(pop)

/**
 * @struct BMP180_HandleTypeDef
 * BMP180 handle
 */
typedef struct {
  I2C_HandleTypeDef *I2C_Handle; /**< I2C handle pointer. Set by user. */
  BMP180_SensorTypeDef Sensor;  /**< Sensor type. Set by user.*/
  uint8_t DeviceAddress; /**< I2C device address. Set by [BMP180_Init()](@ref BMP180_Init) */
  uint8_t ChipId; /**< Chip id. Set by @ref BMP180_Init() */
  BMP180_CalibrationDataTypeDef CalibrationData;  /**< BMP180 calibration data. Read from chip by @ref BMP180_Init() */
} BMP180_HandleTypeDef;

HAL_StatusTypeDef __BMP180_ReadRegister(BMP180_HandleTypeDef *hbmp, uint8_t reg, uint8_t *buffer, const size_t size);
HAL_StatusTypeDef __BMP180_WriteRegister(BMP180_HandleTypeDef *hbmp, uint8_t reg, uint8_t *buffer, const size_t size);
HAL_StatusTypeDef __BMP180_ReadChipId(BMP180_HandleTypeDef *hbmp, uint8_t *chipId);
HAL_StatusTypeDef __BMP180_ReadCalibrationData(BMP180_HandleTypeDef *hbmp, BMP180_CalibrationDataTypeDef *sCalibration);
HAL_StatusTypeDef __BMP180_ReadSensor(BMP180_HandleTypeDef *hbmp, BMP180_OversamplingTypeDef precision, uint16_t *UT, uint32_t *UP);
long __BMP180_CompensateTemp(BMP180_HandleTypeDef *hbmp, long UT);
long __BMP180_CompensatePressure(BMP180_HandleTypeDef *hbmp, BMP180_OversamplingTypeDef precision, long UT, long UP);

HAL_StatusTypeDef BMP180_Init(BMP180_HandleTypeDef *hbmp);
HAL_StatusTypeDef BMP180_Reset(BMP180_HandleTypeDef *hbmp);
HAL_StatusTypeDef BMP180_ReadSensor(BMP180_HandleTypeDef *hbmp, BMP180_OversamplingTypeDef precision, long *temp, long *pressure);

#endif /* INC_BMP180_H_ */
