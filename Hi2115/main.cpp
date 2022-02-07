// BIN FILE SIZE: 53 kB

#include <Arduino.h>

#include <Modem.h>
Modem dev;
char imei[16];
char imsi[16];

void setup()
{
    dev.waitPinReady();

    Serial.begin(115200);
    Serial.println("\n[APP] HiSilicon Hi2115 2022 Georgi Angelov\n");
    pinMode(LED, OUTPUT);

    dev.getIMEI(imei);
    Serial.printf("[APP] IMEI: %s\n", imei);
    dev.getIMSI(imsi);
    Serial.printf("[APP] IMSI: %s\n", imsi);

    // if(false == dev.waitRegistration()) reboot_system( REBOOT_CAUSE_APPLICATION_WATCHDOG );

    // TCP/IP is ready
    Serial.printf("[APP] Active Time: %d\n", (int)dev.active_time);
    Serial.printf("[APP] Periodic TAU: %d\n", (int)dev.periodic_tau);
}

void loop()
{
    Serial.print("LOOP ");
    digitalWrite(LED, 1);
    delay(500);
    digitalWrite(LED, 0);
    delay(500);
}
