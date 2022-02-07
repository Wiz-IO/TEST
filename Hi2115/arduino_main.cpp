#ifdef __cplusplus
extern "C"
{
#endif

    //#include <stddef.h>
    //#include <stdint.h>
    //#include "preserve.h"
    //#include "cmsis_rpc.h"
    //#include "cmsis_os2.h"
    //#include "dma.h"
    //#include "uart.h"
    //#include "eflash.h"
    //#include "aio_manager.h"
    //#include "neul_socket.h"
    //#include "neul_kv_storage.h"
    //#include "neul_io_bank.h"
    //#include "irmalloc.h"
    //#include "gpio.h"
    //#include "rtc.h"

#include "platform.h"
#include "hal.h"
#include "neulfw.h"
#include "cmsis_rpc.h"
#include "cmsis_os2.h"
#include "hal_reboot.h"
#include "clocks.h"
#include "watchdog.h"
#include "uart.h"
#include "ipc.h"
#include "irmalloc.h"
#include "string.h"
#include "rtc.h"
#include "neul_socket.h"
#include "system_status.h"
#include "log_common.h"
#include "gpio.h"
#include "log_uart.h"
#include "aio_manager.h"
#include "neul_kv_storage.h"
#include "preserve.h"
#include "dma.h"

#include "eflash.h"
#include "neul_io_bank.h"

    void __libc_init_array(void);

    //extern void dtls_main(void *param);
    //extern void dns_main(void *param);
    //extern void dns_create_queue(void);
    //extern void iccid_register(void *param);

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus

extern void setup();
extern void loop();

void app_main_task(void *param);
void arduino_task(void *param);

static osThreadAttr_t app_task_attr = {"app", 0, NULL, 0, NULL, (190), osPriorityLow7, 0, 0};
static osThreadAttr_t arduino_task_attr = {"arduino_task", 0, NULL, 0, NULL, (1024), osPriorityLow, 0, 0};

typedef struct
{
    osThreadAttr_t attr;
    osThreadFunc_t func;
    uint32_t *task_handle;
    void (*create_queue_fn_p)(void);
} app_task_definition_t;

#define APP_TASKS 2

app_task_definition_t app_tasks[APP_TASKS] = {
    {{"rpc", 0, NULL, 0, NULL, 430, osPriorityLow6, 0, 0}, rpc_main, NULL, rpc_interface_create_queue},
    {{"log", 0, NULL, 0, NULL, 200, osPriorityLow2, 0, 0}, log_main, NULL, NULL},
    //{{"iccid", 0, NULL, 0, NULL, 0x12C, osPriorityLow3, 0, 0}, iccid_register, NULL, NULL},
    //{{"dns", 0, NULL, 0, NULL, 0x12C, osPriorityLow3, 0, 0}, dns_main, NULL, dns_create_queue},
    //{{"dtls", 0, NULL, 0, NULL, 0x118, osPriorityLow5, 0, 0}, dtls_main, NULL, NULL},
    //{{"atuart", 0, NULL, 0, NULL, 0x4B, osPriorityNormal, 0, 0}, at_uart_recv_process, NULL, at_uart_create_queue},
};

void app_os_init(void)
{
    int i;
    for (i = 0; i < APP_TASKS; i++)
    {
        if (app_tasks[i].create_queue_fn_p)
            app_tasks[i].create_queue_fn_p();
    }
    for (i = 0; i < APP_TASKS; i++)
    {
        app_tasks[i].task_handle = (uint32_t *)osThreadNew(app_tasks[i].func, 0, &app_tasks[i].attr);
        if (NULL == app_tasks[i].task_handle)
            panic(PANIC_TASK_CREATE_FAILED, i);
    }

    osThreadNew(app_main_task, 0, &app_task_attr);

    // ? ... need info
}

void arduino_task(void *param)
{
    __libc_init_array();
    setup();
    while (1)
    {
        loop();
        osThreadYield();
    }
}

void app_main_task(void *param)
{
    io_bank_init();           // PMU
    //eflash_init();          // exist ??? Initialise external flash
    neul_kv_cache_register(); // register cache to allow flusing on reboot
    log_uart_enable();

    // ? ... need info

    osThreadNew(arduino_task, 0, &arduino_task_attr);
    while (1)
    {
        osDelay(-1); //osThreadYield();
    }
}

void hw_init(void)
{
    hal_cpu_init();
    hal_reboot_init();
    irmalloc_init_default(); // Initialise malloc

    // Initialise the drivers used.
    gpio_init();
    rtc_init();
    system_status_init();
    ipc_init();

#ifndef DISABLE_WDT
#define WDT_TIMEOUT_S (30)
    watchdog_init(clocks_get_core_clock() * WDT_TIMEOUT_S, true, true);
    watchdog_enable();
#endif
}

int main(void)
{
    preserve_init();
    hw_init();
    log_init();
    osKernelInitialize();

    dma_init();
    uart_init();
    log_init_after_rtos();
    log_uart_init_after_rtos(0);
    socket_init();
    rpc_init();
    neul_kv_init();
    aio_manager_init(); // analog

    app_os_init();
    osThreadNew(app_main_task, 0, &app_task_attr);
    return osKernelStart();
}

////// NOTES //////////////////////////////////////////////////

/* MEMORY */
//void *irmalloc(size_t size);
//void *irzalloc(size_t size);
//void *irrealloc(void *buf, size_t size);
//void irfree(void * buf);

/* TIME */
//osStatus_t osDelay (uint32_t ticks);
//uint64_t osKernelGetTick2ms(void);