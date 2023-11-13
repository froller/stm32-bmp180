# stm32-bmp180

STM32 HAL-based library for Bosch BMP180 combined temperature,
and atmospheric pressure sensor.

## Synopsys

```C
#include "stm32f1xx_hal.h"
#include "../../BMP180/Inc/BMP180.h"

I2C_HandleTypeDef hi2c1;
BMP180_HandleTypeDef hbmp;

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_I2C1_Init();

  hbmp.I2C_Handle = &hi2c1;
  hbmp.Sensor = BMP180;
  BMP180_Init(&hbmp);

  long temp;
  long pressure;
  BMP180_ReadSensor(&hbmp, BMP180_PRECISION_STANDARD, &temp, &pressure);
}
```

## See Also

- https://www.bosch-sensortec.com/products/environmental-sensors/
- https://datasheetspdf.com/pdf-file/770150/Bosch/BMP180/1

