#include <M5Stack.h>

#define SENSOR_0 21
#define SENSOR_1 35
#define SENSOR_2 36
#define SENSOR_3 13
#define SENSOR_4 34

#define BTN_A 39
#define BTN_B 38
#define BTN_C 37

#define MOTOR_RIGHT 16
#define MOTOR_LEFT 17
#define INDICATOR 5

#define SENSOR_COUNT 5

// LEDC
#define MOTOR_RIGHT_CHANNEL 1
#define MOTOR_LEFT_CHANNEL 2
#define RESOLUTION_BITS 10
#define FREQUENCY 100
#define MAX_SPEED 1023
#define MIN_SPEED 0 // 230 - Temp 0

// Settings
#define BASE_SPEED 250 // Base Speed which is modified depending on the steering
#define STEERING_SCALE 250 // Steering value scaled against this value
#define DEAD_ZONE 0.05 // Correction dead zone, used to not correct if the steering error is too small
#define CENTER_SENSOR_PENALTY 0.8 // Used to penalize the correction if the center sensor is on the line, prevents the bot from freaking out to much

// Reduces speeds at turns
#define REDUCED_SPEED_THRESHOLD 0.20 // The steering value threshold at which the speed is reduced, this is used in case the turn is sharp and slower speeds should be used to lose the line
#define SPEED_REDUCTION 0.50 // Speed reduction factor, used to reduce the speed

int reads[SENSOR_COUNT];

float weights[5] = {-1, -0.5, 0, 0.5, 1};
float lastError = 0;
int lostLineTime = 0;

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

void debugDisplay(float steering, int speedLeft, int speedRight) {
    M5.Lcd.clear();
    M5.Lcd.setTextSize(2);

    M5.Lcd.setCursor(120, 10);
    M5.Lcd.printf("Motors");
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.printf("%d", speedLeft);
    M5.Lcd.setCursor(280, 10);
    M5.Lcd.printf("%d", speedRight);

    M5.Lcd.setCursor(97, 90);
    M5.Lcd.setTextSize(5);
    M5.Lcd.printf("%.2f", steering);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 200);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        M5.Lcd.printf("S%d:%d ", SENSOR_COUNT - i, reads[SENSOR_COUNT - i - 1]);
    }
}

void move(float steering) {
    steering = constrain(steering, -1, 1);

    int left = (int) (BASE_SPEED + steering * STEERING_SCALE);
    int right = (int) (BASE_SPEED - steering * STEERING_SCALE);

    if (fabs(steering) > REDUCED_SPEED_THRESHOLD) {
        if (steering < 0)
            left = 0;
        else
            right = 0;
    }

    debugDisplay(steering, left, right);

    ledcWrite(MOTOR_LEFT_CHANNEL, constrain(left, MIN_SPEED, MAX_SPEED));
    ledcWrite(MOTOR_RIGHT_CHANNEL, constrain(right, MIN_SPEED, MAX_SPEED));
}

void loop() {
    M5.update();

    reads[0] = digitalRead(SENSOR_0);
    reads[1] = digitalRead(SENSOR_1);
    reads[2] = digitalRead(SENSOR_2);
    reads[3] = digitalRead(SENSOR_3);
    reads[4] = digitalRead(SENSOR_4);

    float error = 0.0f;
    int offLineSensors = 0;

    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (reads[i] == HIGH) {
            error += weights[i];
            offLineSensors++;
        }
    }

    bool centered = reads[2] == LOW;

    if (offLineSensors == SENSOR_COUNT) {
        error = lastError;
        lostLineTime++;
    } else {
        if (offLineSensors > 0)
            error /= offLineSensors;

        lastError = error;
        lostLineTime = 0;
    }

    float steering = error * (lostLineTime / 20 + 1);
    if (centered)
        steering *= CENTER_SENSOR_PENALTY;

    if (fabs(steering) < DEAD_ZONE)
        steering = 0;

    move(steering);
}
