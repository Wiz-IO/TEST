#include <string.h>
#include <stdio.h>
#include <driverlib/sys_ctrl.h>
#include <driverlib/prcm.h>
#include <driverlib/ioc.h>
#include <driverlib/uart.h>
#include <FreeRTOS.h>
#include <task.h>

#include <LZSTACK.h>

#define LED 7
#define APN 0x1A62

void powerOn(uint32_t domain, uint32_t periph, bool sleep, bool deep_sleep)
{
    if (domain)
    {
        PRCMPowerDomainOn(domain);
        while (PRCMPowerDomainsAllOn(domain) != PRCM_DOMAIN_POWER_ON)
            ;
    }
    if (periph)
    {
        PRCMPeripheralRunEnable(periph);
        if (sleep)
            PRCMPeripheralSleepEnable(periph);
        if (deep_sleep)
            PRCMPeripheralDeepSleepEnable(periph);
        PRCMLoadSet();
        while (!PRCMLoadGet())
            ;
    }
}

void setup()
{
    // Enable HF XOSC
    OSCHF_TurnOnXosc();
    while (!OSCHF_AttemptToSwitchToXosc())
        ;
    // Use 32768 Hz XOSC
    OSCClockSourceSet(OSC_SRC_CLK_LF, OSC_XOSC_LF);
    while (OSCClockSourceGet(OSC_SRC_CLK_LF) != OSC_XOSC_LF)
        ;
    SysCtrlAonSync();            // Make sure writes take effect
    PRCMCacheRetentionDisable(); // now disable retention

    powerOn(PRCM_DOMAIN_SERIAL, PRCM_PERIPH_UART0, 0, 0);                                                                         // power on uart0
    IOCPinTypeUart(UART0_BASE, 2, 3, IOID_UNUSED, IOID_UNUSED);                                                                   // pin as rx, tx
    UARTConfigSetExpClk(UART0_BASE, SysCtrlClockGet(), 115200, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE); //
    UARTEnable(UART0_BASE);
    void retarget(uint32_t base);
    retarget(UART0_BASE);

    powerOn(PRCM_DOMAIN_PERIPH, PRCM_PERIPH_GPIO, 0, 0);   // power on gpio
    IOCPortConfigureSet(7, IOC_PORT_GPIO, IOC_STD_OUTPUT); // pin as gpio
    GPIO_setOutputEnableDio(7, GPIO_OUTPUT_ENABLE);        // pin dir output

    powerOn(PRCM_DOMAIN_PERIPH, PRCM_PERIPH_GPIO, 0, 0);   // power on gpio
    IOCPortConfigureSet(6, IOC_PORT_GPIO, IOC_STD_OUTPUT); // pin as gpio
    GPIO_setOutputEnableDio(6, GPIO_OUTPUT_ENABLE);        // pin dir output

    extern void hard_trap_init(uint32_t UartBase);
    hard_trap_init(UART0_BASE); // hard_trap_test();
    printf("\n[APP] Z-Stack FreeRTOS 2021 Georgi Angelov\n");
}

void z_init(void);
void z_process(bool blocked);
void z_test_beacon(void);
void z_test_join(void);
void zclAppInit(void);
void zclAppProcess(bool blocked);

void BLINK(void *arg)
{
    printf("[TASK] APP BLINK BEGIN\n");
    while (1)
    {
        GPIO_toggleDio(LED);
        vTaskDelay(100);
    }
}

void APP(void *arg)
{
    printf("[TASK] APP ZCL BEGIN\n");
    zclAppInit();
    while (1)
    {
        zclAppProcess(1);
    }
}

void ZSTACK(void *arg)
{
    printf("[TASK] ZSTACK BEGIN\n");
    z_init();
    xTaskCreate(APP, "APP", 1024, NULL, 2, NULL);
    while (1)
    {
        z_process(1);
    }
}

int main(void)
{
    setup();
    xTaskCreate(BLINK, "BLINK", 256, NULL, 2, NULL);
    xTaskCreate(ZSTACK, "ZSTACK", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    return -1;
}