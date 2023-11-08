# stm32-bme280

STM32 HAL-based library for Bosch BMP180 combined temperature,
and atmospheric pressure sensor.

## Synopsys

```C
#include "stm32f1xx_hal.h"
#include "../../BMP180/Inc/BMP180.h"

I2C_HandleTypeDef hi2c1;
BME280_HandleTypeDef hbmp180;

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_I2C1_Init();

  hbmp180.I2C_Handle = &hi2c1;
  hbmp180.Sensor = BMP180;
  BMP180_Init(&hbmp180);

  long temp;
  long pressure;
  BMP180_ReadSensor(&hbmp180, BMP180_PRECISION_STANDARD, &temp, &pressure);
}
```

## See Also

- https://www.bosch-sensortec.com/products/environmental-sensors/
- https://www.alldatasheet.com/datasheet-pdf/pdf/1132068/BOSCH/BMP180.html
