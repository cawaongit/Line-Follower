#include <M5Stack.h>

#define IR_L2 34
#define IR_L1 13
#define IR_C 36
#define IR_R1 35
#define IR_R2 21

#define M5_BUTTON_A 39
#define M5_BUTTON_B 38
#define M5_BUTTON_C 37

#define MOTOR_RIGHT 16
#define MOTOR_LEFT 17
#define INDICATOR 5

#define SENSOR_COUNT 5

// LEDC
#define MOTOR_RIGHT_CHANNEL 1
#define MOTOR_LEFT_CHANNEL 2
#define RESOLUTION_BITS 10
#define FREQUENCY 100

enum RobotState {
    FOLLOW, // Robot is following the line
    SEARCH // Robot is searching for the line
};

// Screen - Refresh Rate
#define REFRESH_DELAY 50
unsigned long lastRefresh = 0;

// Settings - MOTOR
#define BASE_SPEED 512
#define MIN_SPEED 230
#define HARD_MIN_SPEED 50 // Used as a hard limit for the motors, to prevent them from stopping at all.
#define MAX_SPEED 1023
#define SEARCH_SPEED 280
#define RECOVER_SPEED 340

// Settings - Algorithm
#define CORRECTION_STRENGTH 200
#define DAMPING_STRENGTH 260

int weights[SENSOR_COUNT] = {5, 1, 0, -1, -5};

// Settings - Search
#define SEARCH_RAMP_START_MS 0
#define SEARCH_RAMP_FULL_MS 900
#define SEARCH_RAMP_MAX_BONUS 400
#define SEARCH_BASE_TURN_BIAS 300

// Motor calibration
#define MOTOR_LEFT_CORRECTION 0
#define MOTOR_RIGHT_CORRECTION 0

// Memory - General
RobotState state = FOLLOW;
int sensorValues[SENSOR_COUNT];

// Memory - Follow
float lastError = 0;

// Memory - Search
unsigned long searchStartTimeStamp = 0;
float lastSeenPosition = 0;

void setup() {
    M5.begin();

    // Sensors
    pinMode(IR_L2, INPUT);
    pinMode(IR_L1, INPUT);
    pinMode(IR_C, INPUT);
    pinMode(IR_R1, INPUT);
    pinMode(IR_R2, INPUT);

    // Buttons
    pinMode(M5_BUTTON_A, INPUT);
    pinMode(M5_BUTTON_B, INPUT);
    pinMode(M5_BUTTON_C, INPUT);

    // Motors
    ledcSetup(MOTOR_RIGHT_CHANNEL, FREQUENCY, RESOLUTION_BITS);
    ledcAttachPin(MOTOR_RIGHT, MOTOR_RIGHT_CHANNEL);
    ledcWrite(MOTOR_RIGHT_CHANNEL, 0);

    ledcSetup(MOTOR_LEFT_CHANNEL, FREQUENCY, RESOLUTION_BITS);
    ledcAttachPin(MOTOR_LEFT, MOTOR_LEFT_CHANNEL);
    ledcWrite(MOTOR_LEFT_CHANNEL, 0);

    pinMode(INDICATOR, OUTPUT);
}

void debugDisplay(int speedRight, int speedLeft) {
    if (millis() - lastRefresh < REFRESH_DELAY)
        return;

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
    M5.Lcd.printf("%.2f", lastSeenPosition);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(110, 150);
    M5.Lcd.printf("State: %d", state);

    M5.Lcd.setCursor(10, 200);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        M5.Lcd.printf("S%d:%d ", SENSOR_COUNT - i, sensorValues[i]);
    }

    lastRefresh = millis();
}

void move(int leftSpeed, int rightSpeed) {
    leftSpeed = constrain(leftSpeed + MOTOR_LEFT_CORRECTION, 0, MAX_SPEED);
    rightSpeed = constrain(rightSpeed + MOTOR_RIGHT_CORRECTION, 0, MAX_SPEED);

    debugDisplay(rightSpeed, leftSpeed);

    ledcWrite(MOTOR_LEFT_CHANNEL, leftSpeed);
    ledcWrite(MOTOR_RIGHT_CHANNEL, rightSpeed);
}

void followLine(float error) {
    digitalWrite(INDICATOR, HIGH); // Used as a visual indicator that the robot is following a line

    float damping = error - lastError;
    float correction = CORRECTION_STRENGTH * error + DAMPING_STRENGTH * damping;

    // Ideal World Values - those are the values we want the motors to go.
    int leftSpeed = BASE_SPEED - correction;
    int rightSpeed = BASE_SPEED + correction;

    if (leftSpeed > 0 && leftSpeed < MIN_SPEED) {
        leftSpeed = MIN_SPEED - 20;
        rightSpeed += 20;
    }

    if (rightSpeed > 0 && rightSpeed < MIN_SPEED) {
        rightSpeed = MIN_SPEED - 20;
        leftSpeed += 20;
    }

    rightSpeed = constrain(rightSpeed, HARD_MIN_SPEED, MAX_SPEED);
    leftSpeed = constrain(leftSpeed, HARD_MIN_SPEED, MAX_SPEED);

    move(leftSpeed, rightSpeed);
    lastError = error;
}

int signOf(float value) {
    if (value < 0) return -1;
    if (value > 0) return 1;
    return 0;
}

void search() {
    digitalWrite(INDICATOR, LOW);

    unsigned long elapsedTime = millis() - searchStartTimeStamp;

    int direction = signOf(lastSeenPosition);
    if (direction == 0) {
        direction = signOf(lastError);
        if (direction == 0) direction = 1;
    }

    int rampedBonus = 0;
    if (elapsedTime > SEARCH_RAMP_START_MS) {
        unsigned long rampTime = elapsedTime - SEARCH_RAMP_START_MS;
        if (rampTime > (SEARCH_RAMP_FULL_MS - SEARCH_RAMP_START_MS)) {
            rampTime = (SEARCH_RAMP_FULL_MS - SEARCH_RAMP_START_MS);
        }

        rampedBonus = (int)((float)rampTime / (float)(SEARCH_RAMP_FULL_MS - SEARCH_RAMP_START_MS) * SEARCH_RAMP_MAX_BONUS);
    }

    int turnBias = SEARCH_BASE_TURN_BIAS + rampedBonus;

    int insideMotor = SEARCH_SPEED - turnBias;
    int outsideMotor = SEARCH_SPEED + turnBias;

    if (elapsedTime < SEARCH_RAMP_START_MS) {
        insideMotor = SEARCH_SPEED;
        outsideMotor = SEARCH_SPEED;
    }

    if (direction > 0) {
        move(insideMotor, outsideMotor);
    } else {
        move(outsideMotor, insideMotor);
    }
}

void readSensors(int &onLineSensors, float &position) {
    onLineSensors = 0;
    position = 0;

    int weight = 0;

    sensorValues[0] = digitalRead(IR_L2);
    sensorValues[1] = digitalRead(IR_L1);
    sensorValues[2] = digitalRead(IR_C);
    sensorValues[3] = digitalRead(IR_R1);
    sensorValues[4] = digitalRead(IR_R2);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        bool onLine = !sensorValues[i];
        if (onLine) {
            onLineSensors++;
            weight += weights[i];
        }
    }

    if (onLineSensors > 0) {
        position = (float) weight / (float) onLineSensors;
    } else {
        position = 0;
    }
}

void loop() {
    int onLineSensors = 0;
    float position = 0;

    readSensors(onLineSensors, position);

    if (onLineSensors == 0) {
        if (state != SEARCH) {
            state = SEARCH;
            searchStartTimeStamp = millis();
        }
    } else {
        lastSeenPosition = position;
        state = FOLLOW;
    }

    switch (state) {
        case FOLLOW:
            followLine(position);
            break;
        case SEARCH:
            search();
            break;
    }

    delay(5);
}
