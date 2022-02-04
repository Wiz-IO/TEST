////////////////////////////////////////////////////////////////////////////////////////
//
//      2022 Georgi Angelov
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

#include <Arduino.h>
#include <variant.h>
#include <hal_gpio.h>
#include <hal_pio.h>
#include <hal_pio_ex.h>

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

PinDescription *getArduinoPin(uint8_t arduinoPin)
{
    for (int i = 0; i < ARRAYLEN(pinsMap); i++)
        if (pinsMap[i].arduino == arduinoPin)
            return &pinsMap[i];
    return NULL;
}

void pinMode(uint8_t pin, uint8_t mode)
{
    PinDescription *n = getArduinoPin(pin);
    if (n)
    {
        PIN Pin = (PIN)n->hisilicon;
        HAL_PIO_GPIO_DIR Dir = HAL_PIO_GPIO_DIR_IN;
        if (mode == -1)
        {
            hal_pio_set_function(Pin, FUNC_UNCLAIMED);
        }
        else
        {
            hal_pio_set_function(Pin, FUNC_GPIO); // must be first
            if (mode & INPUT_PULLUP)
            {
                hal_pio_config_pull(Pin, HAL_PIO_PULL_UP);
            }
            else if (mode & INPUT_PULLDOWN)
            {
                hal_pio_config_pull(Pin, HAL_PIO_PULL_DOWN);
            }
            if ((mode & OUTPUT) || (mode & OUTPUT_LO) || (mode & OUTPUT_HI))
            {
                Dir = HAL_PIO_GPIO_DIR_OUT;
                bool level = mode & OUTPUT_HI;
                hal_pio_set_gpio_output(Pin, (HAL_PIO_GPIO_LVL)level);
            }
            hal_pio_set_gpio_direction(Pin, Dir);
        }
    }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    PinDescription *n = getArduinoPin(pin);
    if (n)
        hal_pio_set_gpio_output((PIN)n->hisilicon, (HAL_PIO_GPIO_LVL)val);
}

int digitalRead(uint8_t pin)
{
    PinDescription *n = getArduinoPin(pin);
    if (n)
        return hal_pio_get_gpio_input((PIN)n->hisilicon);
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////

void led_blink(int led, int delay_ms)
{
    digitalWrite(led, 1);
    delay(delay_ms);
    digitalWrite(led, 0);
    delay(delay_ms);
}
