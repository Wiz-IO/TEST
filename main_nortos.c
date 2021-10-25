#ifdef USE_NORTOS

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "NoRTOS.h"
#include "ti_drivers_config.h"

int print_memory(void);
UART_Handle uartHandle;

void *mainThread(void *arg0)
{
    GPIO_init();
    GPIO_setConfig(CONFIG_GPIO_LED_GREEN, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    UART_init();
    uartHandle = UART_open(CONFIG_UART_0, NULL);
    retarget_uart(uartHandle);
    UART_write(uartHandle, "\n[APP] NORTOS Hello World\n", 19);
    printf("[APP] PRINTF TEST\n");
    print_memory();
    while (1)
    {
        UART_write(uartHandle, "LOOP ", 5);
        GPIO_write(LED_GREEN, 1);
        CPUdelay(1000000);
        GPIO_write(LED_GREEN, 0);
        CPUdelay(9000000);
    }
}

int main(void)
{
    Power_init();
    PIN_init(BoardGpioInitTable);
    NoRTOS_start();
    mainThread(NULL);
    return -1;
}

#endif // USE_NORTOS