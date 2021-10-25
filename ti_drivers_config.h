////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2021 Georgi Angelov ver 1.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef ti_drivers_config_h
#define ti_drivers_config_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include <ti/devices/cc13x2_cc26x2/inc/hw_memmap.h>
#include <ti/devices/cc13x2_cc26x2/inc/hw_ints.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
    extern const PIN_Config BoardGpioInitTable[];
#define CONFIG_PIN_0 0x00000006
#define CONFIG_PIN_1 0x00000007

#define CONFIG_PIN_2 0x00000002
#define CONFIG_PIN_3 0x00000003

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC26XX.h>

#define CONFIG_GPIO_LED_RED 0
#define LED_RED CONFIG_GPIO_LED_RED

#define CONFIG_GPIO_LED_GREEN 1
#define LED_GREEN CONFIG_GPIO_LED_GREEN

#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>

#define CONFIG_UART_0 0

void retarget_uart(UART_Handle handle);

#ifdef __cplusplus
}
#endif
#endif // ti_drivers_config_h