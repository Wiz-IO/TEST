#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static char m_strTemp[100];

#define PI 3.1415926
#define EARTH_RADIUS 6378.137

// Calculate radian
static float radian(float d)
{
    return d * PI / 180.0;
}

static float get_distance(float lat1, float lng1, float lat2, float lng2)
{
    float radLat1 = radian(lat1);
    float radLat2 = radian(lat2);
    float a = radLat1 - radLat2;
    float b = radian(lng1) - radian(lng2);
    float dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2))));
    dst = dst * EARTH_RADIUS;
    dst = round(dst * 10000) / 10000;
    return dst;
}

static void test_dis(void)
{
    float lat1 = 39.90744;
    float lng1 = 116.41615;
    float lat2 = 39.90744;
    float lng2 = 116.30746;
    float dst;
    // test: float as arguments
    dst = get_distance(lat1, lng1, lat2, lng2);
    printf("[6] dst = %0.3fkm\r\n", dst); 
}

void test_float(void)
{
    puts("[TEST-MATH] BEGIN\r\n"); // test puts()
    double a, b, s, pos;
    double radLat1 = 31.11;
    double radLat2 = 121.29;
    double lat = atof("58.12348976");
    double deg = atof("0.5678");
    double sec = 10.0;
    double sum = deg + sec;
    sec = round(lat);
    lat += sec;
    printf("[0] sum = %f\r\n", lat + deg);
    printf("[1] atof1 = %.3f atof2 = %f var = %f sum = %f\r\n", lat, deg, sec, sum);
    printf("[2] atof1 = %.3f atof2 = %f var = %f\r\n", lat, deg, sec);
    a = sin(45.0);
    b = cos(30.0);
    s = sqrt(81);
    pos = 2 * asin(sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2)));
    printf("[3] a = %.2f, b = %.2f, radLat1 = %.3f, radLat2 = %.3f, s = %.5f\r\n", a, b, radLat1, radLat2, s);
    printf("[4] pos = %g\r\n", pos);
    sprintf(m_strTemp, "[5] a = %.2f, b = %.2f, radLat1 = %.3f, radLat2 = %.3f, s = %.5f\r\n", a, b, radLat1, radLat2, s);
    printf(m_strTemp);
    test_dis();
    puts("[TEST-MATH] END\r\n");
}

/*
gcc.exe (MinGW.org GCC-8.2.0-3) 8.2.0

[TEST-MATH] BEGIN
[0] sum = 116.691290
[1] atof1 = 116.123 atof2 = 0.567800 var = 58.000000 sum = 10.567800
[2] atof1 = 116.123 atof2 = 0.567800 var = 58.000000
[3] a = 0.85, b = 0.15, radLat1 = 31.110, radLat2 = 121.290, s = 9.00000
[4] pos = 0.845889
[5] a = 0.85, b = 0.15, radLat1 = 31.110, radLat2 = 121.290, s = 9.00000
[6] dst = 9.282km
[TEST-MATH] END

TIRTOS Hello World
[TEST-MATH] BEGIN
[0] sum = 116.691290
[1] atof1 = 116.123 atof2 = 0.567800 var = 58.000000 sum = 10.567800
[2] atof1 = 116.123 atof2 = 0.567800 var = 58.000000
[3] a = 0.85, b = 0.15, radLat1 = 31.110, radLat2 = 121.290, s = 9.00000
[4] pos = 0.845889
[5] a = 0.85, b = 0.15, radLat1 = 31.110, radLat2 = 121.290, s = 9.00000
[6] dst = 9.282km
[TEST-MATH] END

BAREMETAL Hello World
[TEST-MATH] BEGIN
[0] sum = 116.691290
[1] atof1 = 116.123 atof2 = 0.567800 var = 58.000000 sum = 10.567800
[2] atof1 = 116.123 atof2 = 0.567800 var = 58.000000
[3] a = 0.85, b = 0.15, radLat1 = 31.110, radLat2 = 121.290, s = 9.00000
[4] pos = 0.845889
[5] a = 0.85, b = 0.15, radLat1 = 31.110, radLat2 = 121.290, s = 9.00000
[6] dst = 9.282km
[TEST-MATH] END


*/