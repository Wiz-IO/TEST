/// DEMO: https://www.youtube.com/watch?v=2sYov6i-YAU

#include <Arduino.h>

#include <Modem.h>
Modem dev;
char imei[16];
char imsi[16];

void setup()
{
    Serial.begin(115200);
    Serial.println("\n[APP] HiSilicon Hi2115 2022 Georgi Angelov\n");
    pinMode(LED, OUTPUT);
    dev.waitPinReady();

    // dev.getSomething() TODO
    // dev.setAPN()
    // dev.setBAND()
    // dev.setSomething()

    dev.getIMEI(imei);
    Serial.printf("[APP] IMEI: %s\n", imei);
    dev.getIMSI(imsi);
    Serial.printf("[APP] IMSI: %s\n", imsi);

    // dev.waitRegistration(); // TODO
    // here TCP/IP must be ready

    Serial.printf("[APP] Active Time: %d\n", (int)dev.active_time);
    Serial.printf("[APP] Periodic TAU: %d\n", (int)dev.periodic_tau);
}

void loop()
{
    Serial.print("Blink LED ");
    digitalWrite(LED, 1);
    delay(500);
    digitalWrite(LED, 0);
    delay(500);
}


/// RESULT: 
/*
> Executing task in folder HI-TEST: C:\Users\1124\.platformio\penv\Scripts\platformio.exe run --target upload --environment wizio-test <
Processing wizio-test (platform: wizio-HiSilicon; board: wizio-test; framework: arduino)
-------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
←[32m←[1m←[0m<<<<<<<<<<<< WIZIO HISILICON HI2115 TEST BOARD 2022 Georgi Angelov >>>>>>>>>>>>
CONFIGURATION: https://docs.platformio.org/page/boards/wizio-HiSilicon/wizio-test.html
PLATFORM: WizIO - Hisilicon Hi2115 (1.0.0) > WizIO HiSilicon Hi2115 Test Board
HARDWARE: CORTEX M0, HISILICON HI2115 100MHz, 64KB RAM, 256KB Flash
DEBUG: Current (uart) On-board (uart)
PACKAGES:
 - framework-wizio-HiSilicon 1.0.0
 - toolchain-gccarmnoneeabi 1.70201.0 (7.2.1)
  * LINKER       : Default
LIBSOURCE_DIRS: C:\Users\1124\.platformio\packages\framework-wizio-HiSilicon\arduino\libraries\Hi2115
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 5 compatible libraries
Scanning dependencies...
Dependency Graph
|-- <Modem>
Building in release mode
Compiling .pio\build\wizio-test\sdk\startup\startup.S.o
Compiling .pio\build\wizio-test\sdk\startup\vectors.c.o
Compiling .pio\build\wizio-test\arduino\arduino\IPAddress.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\Print.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\Stream.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\WMath.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\WString.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\cbuf.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\dtostrf.c.o
Compiling .pio\build\wizio-test\arduino\arduino\new.cpp.o
Compiling .pio\build\wizio-test\arduino\arduino\wiring_pulse.c.o
Compiling .pio\build\wizio-test\arduino\arduino\wiring_shift.c.o
Compiling .pio\build\wizio-test\arduino\core\HardwareSerial.cpp.o
Compiling .pio\build\wizio-test\arduino\core\arduino_main.cpp.o
Compiling .pio\build\wizio-test\arduino\core\interface.c.o
Compiling .pio\build\wizio-test\arduino\core\memory.c.o
Compiling .pio\build\wizio-test\arduino\core\wiring.c.o
Compiling .pio\build\wizio-test\arduino\core\wiring_digital.c.o
Compiling .pio\build\wizio-test\arduino\variant\variant_pins.c.o
Compiling .pio\build\wizio-test\src\main.cpp.o
Linking .pio\build\wizio-test\program.elf
Checking size .pio\build\wizio-test\program.elf
Building .pio\build\wizio-test\program.dat
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  11.3% (used 7432 bytes from 65536 bytes)
Flash: [==        ]  20.8% (used 54436 bytes from 262144 bytes)
Building .pio\build\wizio-test\program.bin
BIN FILE SIZE: 53 kB
SIGNATURE: C14A1F3F6358ECD88C6D401DB92A0013E3EBDF7BE0158B1649A2DC127B3CC7D5
MANIFEST: .pio\build\wizio-test\manifest.txt
Configuring upload protocol...
Looking for upload port...
Auto-detected: COM44
Uploading: program
[PACKER] BEGIN
Neul UE Firmware Packager v3.34.0.3
Adding .pio\build\wizio-test\manifest.txt...
Adding C:\Users\1124\Documents\PlatformIO\Projects\HI-TEST\.pio\build\wizio-test\.\program.dat...
Adding C:\Users\1124\Documents\PlatformIO\Projects\HI-TEST\.pio\build\wizio-test\.\program.sha...
Created firmware package file .pio\build\wizio-test\program.fwpkg
[PACKER] DONE
[UPDATER] BEGIN
[UPDATER] FIRMWARE BASE: BC68JAR02A02
Neul firmware package updater v3.34.0.3
Adding .pio\build\wizio-test\manifest.txt...
Adding .pio\build\wizio-test\.\program.dat...
Adding .pio\build\wizio-test\.\program.sha...
Adding ApplicationA Sha256
Application finished.
[UPDATER] NEW FIRMWARE: .pio\build\wizio-test\NEW_BC68JAR02A02.fwpkg
[UPDATER] DONE
Ready
=========== [SUCCESS] Took 5.83 seconds ==========

and so on...

*/