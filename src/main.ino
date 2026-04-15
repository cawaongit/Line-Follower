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

#define BASE_SPEED 70
#define CORRECTION_MULTIPLIER 2
#define EXTERNAL_SENSORS_MULTIPLIER 3

#define MAX_SETTING_MODE 4

// LEDC
#define MOTOR_RIGHT_CHANNEL 1
#define MOTOR_LEFT_CHANNEL 2
#define RESOLUTION_BITS 8 // Maximum value of speed is 1023 with 10 bits
#define FREQUENCY 40

int reads[SENSOR_COUNT];
int accumulatedError[SENSOR_COUNT];

// Parameters
int selectionMode = 0;

int additionalLeftSpeed = 0;
int additionalRightSpeed = 0;
int externalPinWeight = 2;
int correctionMultiplier = 2;
int baseSpeed = 70;

void setup() {
    M5.begin();

    // Define pins
    pinMode(SENSOR_0, INPUT);
    pinMode(SENSOR_1, INPUT);
    pinMode(SENSOR_2, INPUT);
    pinMode(SENSOR_3, INPUT);
    pinMode(SENSOR_4, INPUT);
    pinMode(BTN_A, INPUT);
    pinMode(BTN_B, INPUT);
    pinMode(BTN_C, INPUT);

    ledcSetup(MOTOR_RIGHT_CHANNEL, FREQUENCY, RESOLUTION_BITS);
    ledcAttachPin(MOTOR_RIGHT, MOTOR_RIGHT_CHANNEL);
    ledcWrite(MOTOR_RIGHT_CHANNEL, 0);

    ledcSetup(MOTOR_LEFT_CHANNEL, FREQUENCY, RESOLUTION_BITS);
    ledcAttachPin(MOTOR_LEFT, MOTOR_LEFT_CHANNEL);
    ledcWrite(MOTOR_LEFT_CHANNEL, 0);

    pinMode(INDICATOR, OUTPUT);
}

void debugDisplay(int speedLeft, int speedRight) {
    M5.Lcd.clear();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(125, 10);
    M5.Lcd.printf("Total");
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.printf("%d", speedLeft + additionalLeftSpeed);
    M5.Lcd.setCursor(280, 10);
    M5.Lcd.printf("%d", speedRight + additionalRightSpeed);

    M5.Lcd.setCursor(130, 50);
    M5.Lcd.printf("Base");
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.printf("%d", speedRight);
    M5.Lcd.setCursor(280, 50);
    M5.Lcd.printf("%d", speedLeft);

    M5.Lcd.setCursor(80, 90);
    M5.Lcd.print("Settings Mode: ");

    switch (selectionMode) {
        case 0:
            M5.Lcd.setCursor(40, 110);
            M5.Lcd.printf("Add Correction Speed");

            M5.Lcd.setCursor(10, 135);
            M5.Lcd.printf("L: %d", additionalLeftSpeed);
            M5.Lcd.setCursor(260, 135);
            M5.Lcd.printf("R: %d", additionalRightSpeed);
            break;
        case 1:
            M5.Lcd.setCursor(10, 110);
            M5.Lcd.printf("Subtract Correction Speed");

            M5.Lcd.setCursor(10, 135);
            M5.Lcd.printf("L: %d", additionalLeftSpeed);
            M5.Lcd.setCursor(260, 135);
            M5.Lcd.printf("R: %d", additionalRightSpeed);
            break;
        case 2:
            M5.Lcd.setCursor(40, 110);
            M5.Lcd.printf("External Pins Weight");
            M5.Lcd.setCursor(150, 135);
            M5.Lcd.printf("%d", externalPinWeight);
            break;
        case 3:
            M5.Lcd.setCursor(30, 110);
            M5.Lcd.printf("Correction Multiplier");
            M5.Lcd.setCursor(150, 135);
            M5.Lcd.printf("%d", correctionMultiplier);
            break;
        case 4:
            M5.Lcd.setCursor(100, 110);
            M5.Lcd.printf("Base Speed");
            M5.Lcd.setCursor(150, 135);
            M5.Lcd.printf("%d",baseSpeed);
            break;
        default:
            M5.Lcd.setCursor(40, 110);
            M5.Lcd.printf("Undefined");
    }

    M5.Lcd.setCursor(130, 105);
    M5.Lcd.printf("");

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 200);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        M5.Lcd.printf("S%d:%d ", SENSOR_COUNT - i, reads[SENSOR_COUNT - i - 1]);
    }
}

void move(int left, int right) {
    debugDisplay(left, right);
    ledcWrite(MOTOR_LEFT_CHANNEL, left + additionalLeftSpeed);
    ledcWrite(MOTOR_RIGHT_CHANNEL, right + additionalRightSpeed);
}

void handleSettings() {
    if (!digitalRead(BTN_B)) {
        selectionMode++;
        if (selectionMode > MAX_SETTING_MODE) {
            selectionMode = 0;
        }

        delay(200);
    }

    if (!digitalRead(BTN_A)) {
        switch (selectionMode) {
            case 0:
                additionalLeftSpeed++;
                break;
            case 1:
                additionalLeftSpeed--;
                break;
            case 2:
                externalPinWeight++;
                break;
            case 3:
                correctionMultiplier++;
                break;
            case 4:
                baseSpeed += 10;
                break;
        }

        delay(200);
    }

    if (!digitalRead(BTN_C)) {
        switch (selectionMode) {
            case 0:
                additionalRightSpeed++;
                break;
            case 1:
                additionalRightSpeed--;
                break;
            case 2:
                externalPinWeight--;
                break;
            case 3:
                correctionMultiplier--;
                break;
            case 4:
                baseSpeed -= 10;
                break;
        }

        delay(200);
    }
}

void loop() {
    M5.update();
    handleSettings();

    reads[0] = digitalRead(SENSOR_0);
    reads[1] = digitalRead(SENSOR_1);
    reads[2] = digitalRead(SENSOR_2);
    reads[3] = digitalRead(SENSOR_3);
    reads[4] = digitalRead(SENSOR_4);

    int err = 0;

    err += -(reads[0] * accumulatedError[0] + externalPinWeight);
    err += -(reads[1] * accumulatedError[1]);
    err += reads[0] * accumulatedError[3];
    err += reads[1] * accumulatedError[4] + externalPinWeight;

    bool centered = !reads[2] && reads[1] == reads[3];

    if (centered) {
        for (int i = 0; i < SENSOR_COUNT; i++) {
            accumulatedError[i] = 0;
        }

        move(BASE_SPEED, BASE_SPEED);
        return;
    }

    for (int i = 0; i < SENSOR_COUNT; i++) {
        accumulatedError[i] += reads[i];
    }

    int speedLeft = baseSpeed + err ;
    int speedRight = baseSpeed - err;

    move(speedLeft, speedRight);
    delay(10);
}
