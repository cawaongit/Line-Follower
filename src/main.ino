#include <M5Stack.h>

#define SENSOR_0 21
#define SENSOR_1 35
#define SENSOR_2 36
#define SENSOR_3 13
#define SENSOR_4 34

#define MOTOR_RIGHT 16 // Same channel as pin
#define MOTOR_LEFT 17 // Same channel as pin
#define INDICATOR 5

#define SENSOR_COUNT 5
#define HISTORY_SIZE 10
#define MAX_VALUE 31

#define BASE_SPEED 6

// LEDC
#define MOTOR_RIGHT_CHANNEL 1
#define MOTOR_LEFT_CHANNEL 2
#define RESOLUTION_BITS 8
#define FREQUENCY 50

#define NO_BOARD NO_BOARD_FLAG // PlatformIO Compile Flag

// Makes sure all values are way over the maximum possible value of the combined sensor
#define UNDEFINED_READ MAX_VALUE + 10

int readIndex = 0;
int history[HISTORY_SIZE];
int freq[MAX_VALUE + 1];

int lastReads[SENSOR_COUNT];

struct instruction {
    byte leftSpeed;
    byte rightSpeed;
};

instruction LUT[] = {
    {6, 6},   // 0  - 0 0 0 0 0 - All Sensors active (intersection)

    {10, 3},   // 1  - 0 0 0 0 1 -
    {10, 8},   // 2  - 0 0 0 1 0 -
    {10, 6},   // 3  - 0 0 0 1 1 -

    {10,10},  // 4  - 0 0 1 0 0 - Center
    {9, 10},   // 5  - 0 0 1 0 1 -
    {10, 8},   // 6  - 0 0 1 1 0 -
    {10, 6},   // 7  - 0 0 1 1 1 -

    {8, 10},   // 8  - 0 1 0 0 0 -
    {8, 10},    // 9  - 0 1 0 0 1 -
    {8, 10},   // 10 - 0 1 0 1 0 -
    {10, 8},    // 11 - 0 1 0 1 1 -

    {8, 10},   // 12 - 0 1 1 0 0 -
    {8, 10},    // 13 - 0 1 1 0 1 -
    {10, 10},  // 14 - 0 1 1 1 0 - Usual center
    {10, 9},    // 15 - 0 1 1 1 1 -

    {3 ,10},   // 16 - 1 0 0 0 0 -
    {3, 10},   // 17 - 1 0 0 0 1 -
    {10, 8},   // 18 - 1 0 0 1 0 -
    {10, 8},   // 19 - 1 0 0 1 1 -

    {10, 10},   // 20 - 1 0 1 0 0 -
    {10, 10},    // 21 - 1 0 1 0 1 -
    {10, 9},   // 22 - 1 0 1 1 0 -
    {10, 8},    // 23 - 1 0 1 1 1 -

    {8, 10},   // 24 - 1 1 0 0 0 -
    {8, 10},    // 25 - 1 1 0 0 1 -
    {8, 10},   // 26 - 1 1 0 1 0 -
    {8, 10},    // 27 - 1 1 0 1 1 -

    {6, 10},   // 28 - 1 1 1 0 0 -
    {8, 10},    // 29 - 1 1 1 0 1 -
    {9, 10},  // 30 - 1 1 1 1 0 -
    {0,0}     // 31 - 1 1 1 1 1 - No sensors
};

void setup() {
    M5.begin();
    readIndex = 0;

    for (int i = 0; i < HISTORY_SIZE; i++) {
        // Set all the history to the impossible value to make sure we can differentiate undefined values from actual values
        history[i] = UNDEFINED_READ;
    }

    for (int i = 0; i < MAX_VALUE; i++) {
        freq[i] = 0;
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
    readIndex = (readIndex + 1) % HISTORY_SIZE;
}

void debugDisplay(int speedLeft, int speedRight) {
    int currentValue = history[readIndex];

    M5.Lcd.clear();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.printf("%d", speedLeft);
    M5.Lcd.setCursor(280, 10);
    M5.Lcd.printf("%d", speedRight);

    M5.Lcd.setCursor(150, 100);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("%d", currentValue);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 200);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        M5.Lcd.printf("S%d:%d ", SENSOR_COUNT - i, !lastReads[SENSOR_COUNT - i - 1]);
    }
}

void move(int left, int right) {
    ledcWrite(MOTOR_LEFT_CHANNEL, left);
    ledcWrite(MOTOR_RIGHT_CHANNEL, right);
}

void loop() {
    lastReads[0] = !digitalRead(SENSOR_0); // ^ 1
    lastReads[1] = !digitalRead(SENSOR_1); // ^ 2
    lastReads[2] = !digitalRead(SENSOR_2); // ^ 4
    lastReads[3] = !digitalRead(SENSOR_3); // ^ 8
    lastReads[4] = !digitalRead(SENSOR_4); // ^ 16

    int value = transformInput();

    // Handle History and frequency table
    /*
    int previousValue = history[readIndex];
    if (previousValue != UNDEFINED_READ)
        freq[previousValue]--;

    history[readIndex] = value; // Write to the history

    freq[value]++;

    int max = 0;
    int actionValue = 0;
    for (int i = 0; i < MAX_VALUE; i++) {
        if (freq[i] > max) {
            max = freq[i];
            actionValue = i;
        }
    }*/

    history[readIndex] = value; // Write to the history
    instruction action = LUT[value];

    debugDisplay(action.leftSpeed * BASE_SPEED, action.rightSpeed * BASE_SPEED);

    move(action.leftSpeed * BASE_SPEED, action.rightSpeed * BASE_SPEED);

    updateIndex();
    //delay(2); // ~2 reads/sec
}
