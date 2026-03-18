#include <M5Stack.h>

#define SENSOR_0 34
#define SENSOR_1 0
#define SENSOR_2 13
#define SENSOR_3 36
#define SENSOR_4 35

void setup() {
    M5.begin();
    int cap0 = 1; // Ok 1
    int cap1 = 1; // Ok 2
    int cap2 = 1; // Ok 4
    int cap3 = 1; // OK 8
    int cap4 = 1; // Ok 16
    trad(cap4, cap3, cap2, cap1, cap0);
}

void loop() {
// write your code here
}

int trad(int cap4, int cap3, int cap2, int cap1, int cap0) {
    M5.Lcd.setTextSize(12);
    int result = (cap4 * 16) + (cap3 * 8) + (cap2 * 4) + (cap1 * 2) + (cap0);
    M5.Lcd.println(result);

    return result;
}