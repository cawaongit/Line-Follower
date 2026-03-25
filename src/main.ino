#include <M5Stack.h>

#define SENSOR_0 34
#define SENSOR_1 13
#define SENSOR_2 36
#define SENSOR_3 35
#define SENSOR_4 21

void setup() {
    M5.begin();

    pinMode(SENSOR_0, INPUT);
    pinMode(SENSOR_1, INPUT);
    pinMode(SENSOR_2, INPUT);
    pinMode(SENSOR_3, INPUT);
    pinMode(SENSOR_4, INPUT);

    M5.Lcd.setTextSize(2);
}

void loop() {
    int cap0 = digitalRead(SENSOR_0); // Ok 1
    int cap1 = digitalRead(SENSOR_1); // Ok 2
    int cap2 = digitalRead(SENSOR_2); // Ok 4
    int cap3 = digitalRead(SENSOR_3); // OK 8
    int cap4 = digitalRead(SENSOR_4); // Ok 16

    int result = trad(cap4, cap3, cap2, cap1, cap0);

    M5.Lcd.clear();
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.print(result);

    delay(500);
}


int trad(int cap4, int cap3, int cap2, int cap1, int cap0) {
    return (cap4 * 16) + (cap3 * 8) + (cap2 * 4) + (cap1 * 2) + (cap0);
}