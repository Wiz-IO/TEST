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
    printf("dst = %0.3fkm\r\n", dst); //dst = 9.281km
}

void test_float(void)
{
    puts("[TEST-FLOAT] BEGIN\r\n");
    double a, b, s, pos;
    double radLat1 = 31.11;
    double radLat2 = 121.29;
    double lat = atof("58.12348976");
    double deg = atof("0.5678");
    double sec = 10.0;
    double sum = deg + sec;
    sec = round(lat);
    lat += sec;
    printf("sum=%f\r\n", lat + deg);
    printf("atof1=%.3f atof2=%f var=%f sum=%f\r\n", lat, deg, sec, sum);
    printf("atof1=%.3f atof2=%f var=%f\r\n", lat, deg, sec); //Works OK, prints: "atof1=58.123 atof2=0.567800 var=58.000000"
    a = sin(45.0);
    b = cos(30.0);
    s = sqrt(81);
    pos = 2 * asin(sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2)));
    printf("Float test, a=%.2f,b=%.2f,radLat1=%.3f,radLat2=%.3f, s=%.5f\r\n", a, b, radLat1, radLat2, s);
    printf("Float test, pos=%g\r\n", pos);
    sprintf(m_strTemp, "Float test, a=%.2f,b=%.2f,radLat1=%.3f,radLat2=%.3f, s=%.5f\r\n", a, b, radLat1, radLat2, s);
    printf(m_strTemp);
    puts("[TEST-FLOAT] END\r\n");
}
