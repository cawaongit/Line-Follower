#include <M5Stack.h>

#define SENSOR_0 21
#define SENSOR_1 35
#define SENSOR_2 36
#define SENSOR_3 13
#define SENSOR_4 34

#define BTN_A 39
#define BTN_B 38
#define BTN_C 37

#define MOTOR_RIGHT 16 // Same channel as pin
#define MOTOR_LEFT 17 // Same channel as pin
#define INDICATOR 5

#define SENSOR_COUNT 5
#define HISTORY_SIZE 10
#define MAX_VALUE 31

#define BASE_SPEED 50

// LEDC
#define MOTOR_RIGHT_CHANNEL 1
#define MOTOR_LEFT_CHANNEL 2
#define RESOLUTION_BITS 8 // Maximum value of speed is 1023 with 10 bits
#define FREQUENCY 40

#define NO_BOARD NO_BOARD_FLAG // PlatformIO Compile Flag

// Makes sure all values are way over the maximum possible value of the combined sensor
#define UNDEFINED_READ MAX_VALUE + 10


int lastReads[SENSOR_COUNT];

int additionalLeftSpeed = 0;
int additionalRightSpeed = 0;

void setup() {
    M5.begin();

    for (int i = 0; i < HISTORY_SIZE; i++) {
        // Set all the history to the impossible value to make sure we can differentiate undefined values from actual values
    }

    for (int i = 0; i < MAX_VALUE; i++) {
    }

    // Define pins
    if (NO_BOARD) {
        pinMode(SENSOR_0, INPUT_PULLUP);
        pinMode(SENSOR_1, INPUT_PULLUP);
        pinMode(SENSOR_2, INPUT_PULLUP);
        pinMode(SENSOR_3, INPUT_PULLUP);
        pinMode(SENSOR_4, INPUT_PULLUP);
    } else {
        pinMode(SENSOR_0, INPUT);
        pinMode(SENSOR_1, INPUT);
        pinMode(SENSOR_2, INPUT);
        pinMode(SENSOR_3, INPUT);
        pinMode(SENSOR_4, INPUT);
        pinMode(BTN_A, INPUT);
        pinMode(BTN_B, INPUT);
        pinMode(BTN_C, INPUT);
    }

    ledcSetup(MOTOR_RIGHT_CHANNEL, FREQUENCY, RESOLUTION_BITS);
    ledcAttachPin(MOTOR_RIGHT, MOTOR_RIGHT_CHANNEL);
    ledcWrite(MOTOR_RIGHT_CHANNEL, 0);

    ledcSetup(MOTOR_LEFT_CHANNEL, FREQUENCY, RESOLUTION_BITS);
    ledcAttachPin(MOTOR_LEFT, MOTOR_LEFT_CHANNEL);
    ledcWrite(MOTOR_LEFT_CHANNEL, 0);

    pinMode(INDICATOR, OUTPUT);

    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("Flag: %d", NO_BOARD);
}

int transformInput() {
    if (NO_BOARD)
        return random(MAX_VALUE + 1);

    int value = 0;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        value |= (lastReads[i] & 1) << i;
    }
    return value;
}

void updateIndex() {
}

void debugDisplay(int speedLeft, int speedRight) {
    M5.Lcd.clear();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(125, 10);
    M5.Lcd.printf("Total");
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.printf("%d", speedLeft);
    M5.Lcd.setCursor(280, 10);
    M5.Lcd.printf("%d", speedRight);

    M5.Lcd.setCursor(130, 50);
    M5.Lcd.printf("Base");
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.printf("%d", BASE_SPEED);
    M5.Lcd.setCursor(280, 50);
    M5.Lcd.printf("%d", BASE_SPEED);

    M5.Lcd.setCursor(110, 90);
    M5.Lcd.printf("Modifier");
    M5.Lcd.setCursor(10, 90);
    M5.Lcd.printf("%d", additionalLeftSpeed);
    M5.Lcd.setCursor(280, 90);
    M5.Lcd.printf("%d", additionalRightSpeed);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 200);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        M5.Lcd.printf("S%d:%d ", SENSOR_COUNT - i, lastReads[SENSOR_COUNT - i - 1]);
    }
}

void move(int left, int right) {
    // The robot goes on the right with same speed on left and right so - on the left motor and + on the right motor
    ledcWrite(MOTOR_LEFT_CHANNEL, left - 13); // Between 12 and 13
    ledcWrite(MOTOR_RIGHT_CHANNEL, right);
}

void loop() {
    M5.update();
    int err = 0;

    lastReads[0] = digitalRead(SENSOR_0);
    lastReads[1] = digitalRead(SENSOR_1);
    lastReads[2] = digitalRead(SENSOR_2);
    lastReads[3] = digitalRead(SENSOR_3);
    lastReads[4] = digitalRead(SENSOR_4);

    err += lastReads[0] ? -2 : 0;
    err += lastReads[1] ? -1 : 0;
    err += lastReads[3] ? 1 : 0;
    err += lastReads[4] ? 2 : 0;

    if (!digitalRead(BTN_A)) {
        additionalLeftSpeed += 1;
        delay(200);
    }

    if (digitalRead(BTN_B)) {
        Serial.println("B pressed");
    }

    if (!digitalRead(BTN_C)) {
        additionalRightSpeed += 1;
        delay(200);
    }

    int speedLeft = BASE_SPEED + err * 2 + additionalLeftSpeed;
    int speedRight = BASE_SPEED - err * 2 + additionalRightSpeed;

    debugDisplay(speedLeft, speedRight);
    move(speedLeft, speedRight);
}
