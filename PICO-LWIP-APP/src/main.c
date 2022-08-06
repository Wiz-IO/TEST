#include <stdio.h>
#include "pico/stdlib.h"

void foo_test(void);

#include "lwip/apps/tftp_client.h"
#include "lwip/apps/tftp_server.h"
static const struct tftp_context tftp = {};

int main(void)
{
    stdio_init_all();
    printf("\n\nHello World\n");
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    //foo_test();
    
    tftp_init_client(&tftp);

    while (true)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(100);
    }
}
