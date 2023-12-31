/*
 * BMP180_hal.c
 *
 *  Created on: Jan 16, 2023
 *      Author: Alexander Frolov <alex.froller@gmail.com>
 */

#include <string.h>
#include "../Inc/BMP180.h"

/*******************************************************************************
 *
 * Public functions
 *
 ******************************************************************************/

/**
 * @brief Initialize BMP180 driver
 * @param[in,out] hbmp: BMP180 handle pointer
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_Init(BMP180_HandleTypeDef *hbmp) {
  switch (hbmp->Sensor) {
  case BMP085:
  case BMP180:
  case BMP183:
    hbmp->DeviceAddress = 0xEE;
    hbmp->ChipId = 0x55;
    break;
  default:
    return HAL_ERROR;
  }
  uint8_t chipId;
  HAL_StatusTypeDef result;
  if ((result = BMP180_ReadChipId(hbmp, &chipId)))
    return result;
  if (hbmp->ChipId != chipId)
    return HAL_ERROR;
  return BMP180_ReadCalibrationData(hbmp, &hbmp->CalibrationData);
}

/**
 * @brief Software reset chip
 * @param[in] hbmp: BMP180 handle pointer
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_Reset(BMP180_HandleTypeDef *hbmp) {
  uint8_t resetCommand = 0xB6;
  return BMP180_WriteRegister(hbmp, 0xE0, &resetCommand, sizeof(resetCommand));
}

/**
 * @brief Read compensated sensor values
 * @param[in] hbmp: BMP180 handle pointer
 * @param[in] precision: precision level
 * @param[out] temp: compensated temperature value
 * @param[out] pressure: compensated atmospheric pressure value
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_ReadSensor(BMP180_HandleTypeDef *hbmp, BMP180_OversamplingTypeDef precision, long *temp, long *pressure) {
  uint16_t UT;
  uint32_t UP;
  HAL_StatusTypeDef result;
  if ((result = BMP180_ReadSensorRaw(hbmp, precision, &UT, &UP)))
    return result;
  *temp = BMP180_CompensateTemp(hbmp, UT);
  *pressure = BMP180_CompensatePressure(hbmp, precision, UT, UP);
  return result;
}

/*******************************************************************************
 *
 * Private functions
 *
 ******************************************************************************/

/**
 * @brief Read BMP180 data register(s) into buffer
 * @param[in] hbmp: BMP180 handle pointer
 * @param[in] reg: Register starting address
 * @param[out] buffer: Pointer to buffer
 * @param[in] size: Number of bytes to read sequentially
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_ReadRegister(BMP180_HandleTypeDef *hbmp, uint8_t reg, uint8_t *buffer, const size_t size) {
  HAL_StatusTypeDef result;
  if ((result = HAL_I2C_Master_Transmit(hbmp->I2C_Handle, hbmp->DeviceAddress, &reg, sizeof(reg), I2C_TIMEOUT)))
    return result;
  return HAL_I2C_Master_Receive(hbmp->I2C_Handle, hbmp->DeviceAddress + 1, buffer, size, I2C_TIMEOUT);
}

/**
 * @brief Write BMP180 data register(s) from buffer
 * @param[in] hbmp: BMP180 handle pointer
 * @param[in] reg: Register starting address
 * @param[in] buffer: Pointer to buffer
 * @param[in] size: Number of bytes to write sequentially
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_WriteRegister(BMP180_HandleTypeDef *hbmp, uint8_t reg, uint8_t *buffer, const size_t size) {
  uint8_t tx_buffer[size + 1];
  tx_buffer[0] = reg;
  memcpy(tx_buffer + 1, buffer, size);
  return HAL_I2C_Master_Transmit(hbmp->I2C_Handle, hbmp->DeviceAddress, tx_buffer, size + 1, I2C_TIMEOUT);
}

/**
 * @brief Read chip Id
 * @param[in] hbmp: BMP180 handle pointer
 * @param[out] chipId: chip Id
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_ReadChipId(BMP180_HandleTypeDef *hbmp, uint8_t *chipId) {
  return BMP180_ReadRegister(hbmp, 0xD0, chipId, sizeof(*chipId));
}

/**
 * @brief Read calibration data from the chip
 * @param[in] hbmp: BMP180 handle pointer
 * @param[out] sCalibration: pointer to calibration data structure
 * @return HAL status
 * @see BMP180_CalibrationDataTypeDef
 */
HAL_StatusTypeDef BMP180_ReadCalibrationData(BMP180_HandleTypeDef *hbmp, BMP180_CalibrationDataTypeDef *sCalibration) {
  uint8_t buffer[22];
  HAL_StatusTypeDef result;
  if ((result = BMP180_ReadRegister(hbmp, 0xAA, buffer, 22)))
    return result;
  for (int i = 0; i < 11; ++i)
    ((uint16_t *)sCalibration)[i] = __builtin_bswap16(((uint16_t *)buffer)[i]);
  return HAL_OK;
}

/**
 * @brief Read raw uncompensated values from sensors
 * @param[in] hbmp: BMP180 handle pointer
 * @param[in] precision: precision level
 * @param[out] UT: uncompensated temperature value
 * @param[out] UP: uncompensated atmospheric pressure value
 * @return HAL status
 */
HAL_StatusTypeDef BMP180_ReadSensorRaw(BMP180_HandleTypeDef *hbmp, BMP180_OversamplingTypeDef precision, uint16_t *UT, uint32_t *UP) {
  BMP180_ControlRegisterTypeDef controlRegister;
  HAL_StatusTypeDef result;

  HAL_StatusTypeDef waitForConversion() {
    do {
      if ((result = BMP180_ReadRegister(hbmp, 0xF4, (uint8_t *)&controlRegister, sizeof(controlRegister))))
        return result;
      else if (controlRegister.sco == BMP180_CONVERSION_RUNNING)
        HAL_Delay(2);
    }
    while (controlRegister.sco == BMP180_CONVERSION_RUNNING);
    return result;
  }

  controlRegister.measurement = BMP180_MEASUREMENT_TEMPERATURE;
  controlRegister.oss = BMP180_OVERSAMPLING_0;
  controlRegister.sco = BMP180_CONVERSION_RUNNING;
  if ((result = BMP180_WriteRegister(hbmp, 0xF4, (uint8_t *)&controlRegister, sizeof(controlRegister))))
    goto HAL_BMP180_ReadSensor_Exit;

  if ((result = waitForConversion()))
    goto HAL_BMP180_ReadSensor_Exit;

  uint16_t UTbuffer = 0;
  if ((result = BMP180_ReadRegister(hbmp, 0xF6, (uint8_t *)&UTbuffer, sizeof(short))))
    goto HAL_BMP180_ReadSensor_Exit;
  *UT = __builtin_bswap16(UTbuffer);

  controlRegister.measurement = BMP180_MEASUREMENT_PRESSURE;
  controlRegister.oss = precision;
  controlRegister.sco = BMP180_CONVERSION_RUNNING;
  if ((result = BMP180_WriteRegister(hbmp, 0xF4, (uint8_t *)&controlRegister, sizeof(controlRegister))))
    goto HAL_BMP180_ReadSensor_Exit;

  if (waitForConversion() != HAL_OK)
    goto HAL_BMP180_ReadSensor_Exit;

  uint32_t UPbuffer = 0;
  if ((result = BMP180_ReadRegister(hbmp, 0xF6, (uint8_t *)&UPbuffer, 3)))
    goto HAL_BMP180_ReadSensor_Exit;
  *UP = (__builtin_bswap32(UPbuffer) >> (16 - controlRegister.oss));
HAL_BMP180_ReadSensor_Exit:
  return result;
}

/**
 * @brief Compensate raw temperature value
 * @param[in] hbmp: BMP180 handle pointer
 * @param[in] UT: raw uncompensated temperature value
 * @return Compensated temperature
 * @see https://datasheetspdf.com/pdf-file/770150/Bosch/BMP180/1 page 15
 */
long BMP180_CompensateTemp(BMP180_HandleTypeDef *hbmp, long UT) {
  long X1, X2, B5;

  X1 = (((long)UT - (long)hbmp->CalibrationData.AC6) * (long)hbmp->CalibrationData.AC5) >> 15;
  X2 = ((long)hbmp->CalibrationData.MC << 11)/(X1 + hbmp->CalibrationData.MD);
  B5 = X1 + X2;
  return ((B5 + 8) >> 4);
}

/**
 * @brief Compensate raw atmospheric pressure value
 * @param[in] hbmp: BMP180 handle pointer
 * @param[in] precision: precision level
 * @param[in] UT: uncompensated raw temperature value
 * @param[in] UP: uncompensated raw atmospheric pressure value
 * @return Compensated athmospheric pressure
 * @see https://datasheetspdf.com/pdf-file/770150/Bosch/BMP180/1 page 15
 */
long BMP180_CompensatePressure(BMP180_HandleTypeDef *hbmp, BMP180_OversamplingTypeDef precision, long UT, long UP) {
  long X1, X2, X3;
  long B3, B5, B6;
  unsigned long B4, B7;
  long p;

  X1 = (((long)UT - (long)hbmp->CalibrationData.AC6) * (long)hbmp->CalibrationData.AC5) >> 15;
  X2 = ((long)hbmp->CalibrationData.MC << 11)/(X1 + hbmp->CalibrationData.MD);
  B5 = X1 + X2;

  B6 = B5 - 4000;

  X1 = (hbmp->CalibrationData.B2 * (B6 * B6) >> 12) >> 11;
  X2 = (hbmp->CalibrationData.AC2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = (((((long)hbmp->CalibrationData.AC1) * 4 + X3) << precision) + 2) >> 2;

  X1 = (hbmp->CalibrationData.AC3 * B6) >> 13;
  X2 = (hbmp->CalibrationData.B1 * ((B6 * B6) >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = (hbmp->CalibrationData.AC4 * (unsigned long)(X3 + 32768)) >> 15;

  B7 = ((unsigned long)(UP - B3) * (50000 >> precision));

  if (B7 < 0x80000000)
    p = (B7 << 1) / B4;
  else
    p = (B7 / B4) << 1;

  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;
  p += (X1 + X2 + 3791) >> 4;

  return p;
}
