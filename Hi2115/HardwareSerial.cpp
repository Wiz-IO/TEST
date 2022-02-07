////////////////////////////////////////////////////////////////////////////////////////
//
//      2021 Georgi Angelov
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

#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include <HardwareSerial.h>

static void ISR_UART0(void) { Serial.save(); }
static void ISR_UART1(void) { Serial1.save(); }
static void ISR_UART2(void) { Serial2.save(); }

////////////////////////////////////////////////////////////////////////////////////////

HardwareSerial::HardwareSerial(uint32_t id)
{
    if (id > 2)
        abort();
    uart = (UART_BUS)id;
    hal_uart_init();

    switch (id)
    {
    case 0:
    {
        hal_pio_set_function((PIN)0, FUNC_UART0_TXD); // PIN ?
        hal_pio_set_function((PIN)0, FUNC_UART0_RXD); // PIN ?
        rx_isr = ISR_UART0;
    }
    break;
    case 1:
    {
        hal_pio_set_function((PIN)0, FUNC_UART0_TXD); // PIN ?
        hal_pio_set_function((PIN)0, FUNC_UART0_RXD); // PIN ?
        rx_isr = ISR_UART1;
    }
    break;
    case 2:
    {
        hal_pio_set_function((PIN)0, FUNC_UART0_TXD); // PIN ?
        hal_pio_set_function((PIN)0, FUNC_UART0_RXD); // PIN ?
        rx_isr = ISR_UART2;
    }
    break;
    }

    data_register = hal_uart_get_data_register(uart);
}

void HardwareSerial::begin(unsigned long baud, bool retarget)
{
    _rx_buffer_head = _rx_buffer_tail = 0;
    memset(_rx_buffer, 0, SERIAL_RX_BUFFER_SIZE);

    hal_uart_set_baud_rate(uart, baud);
    hal_uart_set_data_bits(uart, UART_DATA_BITS_8);
    hal_uart_set_parity(uart, UART_PARITY_NONE);
    hal_uart_set_stop_bits(uart, UART_STOP_BITS_1);
    hal_uart_set_enable_uart(uart, HAL_UART_ENABLE_FLAG_RX | HAL_UART_ENABLE_FLAG_TX);

    hal_uart_configure_interrupt(uart, HAL_UART_INTERRUPT_RX, rx_isr, INTERRUPT_STATE_ENABLED);
    hal_uart_enable_interrupt(uart, HAL_UART_INTERRUPT_RX);

    // TODO RETARGET PRINTF
}

void HardwareSerial::end()
{
    hal_uart_disable_interrupt(uart, HAL_UART_INTERRUPT_RX);
    hal_uart_disable_uart(uart);
    // PINs ?
}

size_t HardwareSerial::write(uint8_t c)
{
    while (hal_uart_is_tx_fifo_full(uart))
        ;
    *data_register = c;
    return 1;
}

size_t HardwareSerial::write(const uint8_t *buf, size_t size)
{
    int res = 0;
    if (buf && size)
    {
        while (size--)
            res += write(*buf++);
    }
    return res;
}

int HardwareSerial::available(void) // ISR PROTECT
{
    return ((unsigned int)(SERIAL_RX_BUFFER_SIZE + _rx_buffer_head - _rx_buffer_tail)) % SERIAL_RX_BUFFER_SIZE;
}

int HardwareSerial::peek(void) // ISR PROTECT
{
    return (_rx_buffer_head == _rx_buffer_tail) ? -1 : _rx_buffer[_rx_buffer_tail];
}

int HardwareSerial::read(void) // ISR PROTECT
{
    int c = -1;
    if (_rx_buffer_head != _rx_buffer_tail)
    {
        c = _rx_buffer[_rx_buffer_tail];
        _rx_buffer_tail = (buffer_index_t)(_rx_buffer_tail + 1) % SERIAL_RX_BUFFER_SIZE;
    }
    return c;
}

// PRIVATE, DONT USE !
void HardwareSerial::save(void) // HI PRIO
{
    while (false == hal_uart_is_rx_fifo_empty(uart))
    {
        uint32_t i = (uint32_t)(_rx_buffer_head + 1) % SERIAL_RX_BUFFER_SIZE;
        if (i == _rx_buffer_tail)
            break; // ring is full
        _rx_buffer[_rx_buffer_head] = *data_register;
        _rx_buffer_head = i;
    }
}

HardwareSerial Serial(0);
HardwareSerial Serial1(1);
HardwareSerial Serial2(2);
