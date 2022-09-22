#include "Arduino.h"
#include "logo.h"
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <ILI9341.h>
SPIClass SPI(3, SPI_PIN_NC, SPI_USE_HARD_PIN, SPI_USE_HARD_PIN, SPI_PIN_NC); // miso used for DC, cs used for RST
#define TFT_DC 0  
#define TFT_RST 3 
ILI9341 ILI = ILI9341(TFT_DC, TFT_RST);

void oled_setup()
{
    ILI.begin();
    ILI.setRotation(1);
    ILI.clearScreen();
    delay(1000);
    ILI.drawImage(0, 0, 320, 240, (uint16_t*)OpenAPI);
    delay(4000);
    ILI.fillScreen(RED);
    delay(2000);
    ILI.fillScreen(GREEN);
    delay(2000);
    ILI.fillScreen(BLUE);
    delay(2000);
}

void oled_loop()
{
    static int frames = 0;
    static uint32_t begin = millis();
    ILI.fillScreen(random(0xFFFF));
    frames += 1;
    if (millis() - begin > 1000)
    {
        Serial.printf("[ARDUINO] Frames per sec: %d\n", frames); // 18
        frames = 0;
        begin = millis();
    }
}

void led_setup()
{
    pinMode(LED, OUTPUT);
}

void led_loop()
{
    static uint32_t begin = millis();
    static int T = 0;
    if (millis() - begin > 250)
    {
        digitalWrite(LED, ++T & 1);
        begin = millis();
    }
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("\n[ARDUINO] Mediatek MT2625 OpenAPI 2022 Georgi Angelov");
    led_setup();
    oled_setup();
}

void loop(void)
{
    led_loop();
    oled_loop();
}