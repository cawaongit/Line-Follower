#include <M5Stack.h>

#define SENSOR_0 34
#define SENSOR_1 13
#define SENSOR_2 36
#define SENSOR_3 35
#define SENSOR_4 21

#define MOTOR_RIGHT 16 // Same channel as pin
#define MOTOR_LEFT 17 // Same channel as pin
#define INDICATOR 5

#define SENSOR_COUNT 5
#define HISTORY_SIZE 10
#define MAX_VALUE 31

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
int freq[MAX_VALUE];

int lastReads[SENSOR_COUNT];

struct instruction {
    byte rightSpeed;
    byte leftSpeed;
};

instruction LUT[] = {
    {0, 0 }, // 0  - 0 0 0 0 0 - All Sensors active
    {}, // 1  - 0 0 0 0 1 -
    {}, // 2  - 0 0 0 1 0 -
    {}, // 3  - 0 0 0 1 1 -
    {}, // 4  - 0 0 1 0 0 - Center
    {}, // 5  - 0 0 1 0 1 -
    {}, // 6  - 0 0 1 1 0 -
    {}, // 7  - 0 0 1 1 1 -
    {}, // 8  - 0 1 0 0 0 -
    {}, // 9  - 0 1 0 0 1 -
    {}, // 10 - 0 1 0 1 0 -
    {}, // 11 - 0 1 0 1 1 -
    {}, // 12 - 0 1 1 0 0 -
    {}, // 13 - 0 1 1 0 1 -
    {}, // 14 - 0 1 1 1 0 - Usual center
    {}, // 15 - 0 1 1 1 1 -
    {}, // 16 - 1 0 0 0 0 -
    {}, // 17 - 1 0 0 0 1 -
    {}, // 18 - 1 0 0 1 0 -
    {}, // 19 - 1 0 0 1 1 -
    {}, // 20 - 1 0 1 0 0 -
    {}, // 21 - 1 0 1 0 1 -
    {}, // 22 - 1 0 1 1 0 -
    {}, // 23 - 1 0 1 1 1 -
    {}, // 24 - 1 1 0 0 0 -
    {}, // 25 - 1 1 0 0 1 -
    {}, // 26 - 1 1 0 1 0 -
    {}, // 27 - 1 1 0 1 1 -
    {}, // 28 - 1 1 1 0 0 -
    {}, // 29 - 1 1 1 0 1 -
    {}, // 30 - 1 1 1 1 0 -
    {255, 255}, // 31 - 1 1 1 1 1 - No sensors active
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
        return random(MAX_VALUE + 1); // Generate random value

    int value = 0;

    for (int i = 0; i < SENSOR_COUNT; i++) {
        value += lastReads[i] << i;
    }

    return value;
}

void updateIndex() {
    if (readIndex < HISTORY_SIZE) {
        readIndex++;
    } else {
        readIndex = 0;
    }
}

void debugDisplay(int speedLeft, int speedRight) {
    int currentValue = history[readIndex];

    M5.Lcd.clear();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.printf("%d", speedLeft);
    M5.Lcd.setCursor(290, 10);
    M5.Lcd.printf("%d", speedRight);

    M5.Lcd.setCursor(150, 100);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("%d", currentValue);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 200);
    for (int i = 0; i < SENSOR_COUNT; i++) {
        M5.Lcd.printf("S%d:%d ", i + 1, !lastReads[i]);
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
    }

    instruction action = LUT[actionValue];

    debugDisplay(action.leftSpeed, action.rightSpeed);

    move(action.leftSpeed, action.rightSpeed);
    updateIndex();
    delay(500); // ~2 reads/sec
}
