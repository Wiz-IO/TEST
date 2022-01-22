#include <Arduino.h>

extern "C" void dump_buffer(const char *text, const void *buffer, unsigned int len);

void TEST_MD5()
{ // http://www.md5.cz/
  CTX_MD5_T ctx;
  memset(&ctx, 0, sizeof(CTX_MD5_T));
  uint8_t buf[16];
  HAL->InitMd5(&ctx);
  HAL->Md5Update(&ctx, (const unsigned char *)"TEST 123456789", 14);
  HAL->Md5Final(&ctx, buf);
  dump_buffer("MD5", buf, 16); // [PASS] A0 73 59 73 44 A8 6A C3 0D 82 12 0A 88 EA FA 2B
  printf("\n");
}

void TEST_SHA()
{ // https://github.com/Wiz-IO/PIC32-FreeRTOS-LWIP-MBEDTLS/blob/master/middleware/mbedtls/sha1.c#L391
  CTX_SHA_T ctx;
  memset(&ctx, 0, sizeof(CTX_SHA_T));
  unsigned char buf[20];
  HAL->InitSha(&ctx);
  HAL->ShaUpdate(&ctx, (const unsigned char *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
  HAL->ShaFinal(&ctx, buf);
  dump_buffer("SHA", buf, 20); // [PASS] 84 98 3E 44 1C 3B D2 6E BA AE 4A A1 F9 51 29 E5 E5 46 70 F1
  printf("\n");
}

void TEST_SHA256()
{ // https://github.com/Wiz-IO/PIC32-FreeRTOS-LWIP-MBEDTLS/blob/master/middleware/mbedtls/sha256.c#L387
  CTX_SHA256_T ctx;
  memset(&ctx, 0, sizeof(CTX_SHA256_T));
  unsigned char buf[32];
  HAL->InitSha256(&ctx);
  HAL->Sha256Update(&ctx, (const unsigned char *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
  HAL->Sha256Final(&ctx, buf);
  dump_buffer("SHA256", buf, 32); // [PASS] 24 8D 6A 61 D2 06 38 B8 E5 C0 26 93 0C 3E 60 39 A3 3C E4 59 64 FF 21 67 F6 EC ED D4 19 DB 06 C1
  printf("\n");
}

void TEST_HMAC()
{ //https://www.freeformatter.com/hmac-generator.html
#define KEY "WizIO"
#define TEXT "Hello World"

  unsigned char buf[32];
  CTX_HMAC_T ctx;
  memset(&ctx, 0, sizeof(CTX_HMAC_T));
  HAL->HmacSetKey(&ctx, HMAC_MD5, (const unsigned char *)KEY, strlen(KEY));
  HAL->HmacUpdate(&ctx, (const unsigned char *)TEXT, strlen(TEXT));
  HAL->HmacFinal(&ctx, buf);
  dump_buffer("HMAC-MD5", buf, 16); // [PASS] 23 A6 0D A6 33 58 6B 93 F1 6A AC CF 0C 1D C4 03
  printf("\n");

  memset(&ctx, 0, sizeof(CTX_HMAC_T));
  HAL->HmacSetKey(&ctx, HMAC_SHA, (const unsigned char *)KEY, strlen(KEY));
  HAL->HmacUpdate(&ctx, (const unsigned char *)TEXT, strlen(TEXT));
  HAL->HmacFinal(&ctx, buf);
  dump_buffer("HMAC-SHA", buf, 20); // [PASS] DD 50 58 F6 B1 C0 F3 64 D4 3B D2 3E 9D A1 66 6B 2C CC 75 41
  printf("\n");

  memset(&ctx, 0, sizeof(CTX_HMAC_T));
  HAL->HmacSetKey(&ctx, HMAC_SHA256, (const unsigned char *)KEY, strlen(KEY));
  HAL->HmacUpdate(&ctx, (const unsigned char *)TEXT, strlen(TEXT));
  HAL->HmacFinal(&ctx, buf);
  dump_buffer("HMAC-SHA256", buf, 32); // [PASS] 31 30 BF AB 3C 21 36 1E 53 5B 47 9E 39 4D A5 F3 5D 05 E9 14 A4 6C 1E 72 D1 A3 B4 40 9D 54 2F 6D
  printf("\n");
}

void setup()
{
  Serial.begin(115200, true);
  printf("\n[APP] Quectel M66 CRYPTO TEST 2021 Georgi Angelov\n\n");
  pinMode(LED_NET, OUTPUT);

  TEST_MD5();
  TEST_SHA();
  TEST_SHA256();
  TEST_HMAC();
}

void loop()
{
  led_blink(LED_NET, 500);
}