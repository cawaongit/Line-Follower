#include <M5Stack.h>

void setup() {
    M5.begin();
    int cap0 = 0;
    int cap1 = 0;
    int cap2 = 0;
    int cap3 = 0;
    int cap4 = 0;
    trad(cap0, cap1, cap2, cap3, cap4);
}

void loop() {
// write your code here
}

int trad(int cap0, int cap1, int cap2, int cap3, int cap4) {

    M5.Lcd.setTextSize(12);

    int result = cap0 + cap1 + cap2 + cap3 + cap4;
    M5.Lcd.println(result);


    return result;
}