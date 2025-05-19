#include <Arduino_FreeRTOS.h>      // FreeRTOS library for multitasking
#include "SevSeg.h"                // Library for 7-segment display
#include <timers.h>                // FreeRTOS timers
#include <LiquidCrystal.h>         // LCD display library

SevSeg myDisplay;                  // 7-segment display object

unsigned long timer;
int deciSecond = 0;

// Task handles for FreeRTOS tasks
TimerHandle_t xCountdownTimer;
TaskHandle_t xTaskGameState;
TaskHandle_t xTaskJoyStickFlickUp;
TaskHandle_t xTaskJoyStickPress;
TaskHandle_t xTaskJoyStickFlickRight;
TaskHandle_t xTaskJoyStickFlickLeft;
TaskHandle_t xTaskJoyStickFlickDown;
TaskHandle_t xTaskButtonPress;
TaskHandle_t xTaskPotentiometerTwist;
TaskHandle_t xTaskEndGame;
TaskHandle_t xTaskDisplaySeven;

const int BUTTONPIN = 4;           // Button pin for Bop-It action
const int JOYSTICKBUTTON = 7;      // Joystick button pin

const int ARRAYSIZE = 7;           // Number of game tasks
float countValue = 1.0;            // Countdown timer value

int currTask;                      // Index of current active task

TaskHandle_t TasksHandles[ARRAYSIZE];  // Array of task handles for game actions
bool taskFinished[ARRAYSIZE];          // Flags to indicate task completion

// LCD pin assignments
const int rs = 53, en = 52, d4 = 51, d5 = 50, d6 = 49, d7 = 48;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Task names to display on LCD
const String TASKDISPLAYS[ARRAYSIZE] = {
  "JoyStick Push-It", "Flick-It Up", "Flick-It Right",
  "Flick-It Left", "Flick-It Down", "Bop-It", "Twist-It"
};

const int OFFBOARDLED = 35;        // Pin for external LED

// Joystick direction logic (initial values)
bool left = analogRead(A0) < 100,
     right = analogRead(A0) > 900,
     down = analogRead(A1) < 100,
     up = analogRead(A0) > 900;

// Task function declarations
void TaskBlink(void *pvParameters);
void TaskAnalogRead(void *pvParameters);

void setup() {
  randomSeed(analogRead(A15));     // Seed random number generator
  lcd.begin(16, 2);                // Initialize LCD (16x2)
  lcd.clear();
  lcd.print("Press Button to");
  lcd.setCursor(0, 1);
  lcd.print("Start Bop-It");
  lcd.setCursor(0, 0);

  // 7-segment display pin setup
  byte digitPins[] = {34, 31, 30, 28};
  int displayType = COMMON_CATHODE;
  byte segmentPins[] = {33, 29, 26, 24, 23, 32, 27, 25};
  byte numDigits = 4;

  myDisplay.begin(displayType, numDigits, digitPins, segmentPins); // Initialize 7-segment display
  myDisplay.setBrightness(90);                                    // Set display brightness
  myDisplay.setNumber(0, 1);                                      // Display initial value

  Serial.begin(19200);                                            // Start serial communication

  while (!Serial) {}                                              // Wait for serial port to connect

  pinMode(BUTTONPIN, INPUT);                                      // Set button pin as input

  // Create FreeRTOS tasks for each game action and system function
  // xTaskCreate(TaskBlink, "Blink", 128, NULL, 2, NULL);
  xTaskCreate(TaskAnalogRead, "AnalogRead", 256, NULL, 2, NULL);
  xTaskCreate(TaskBlinkOffBoard, "BlinkOffBoard", 256, NULL, 2, NULL);
  xTaskCreate(TaskDisplaySeven, "TaskDisplaySeven", 256, NULL, 2, &xTaskDisplaySeven);
  xTaskCreate(TaskGameState, "TaskGameState", 256, NULL, 2, &xTaskGameState);
  xTaskCreate(TaskJoyStickPress, "TaskJoyStickPress", 256, NULL, 2, &xTaskJoyStickPress);
  xTaskCreate(TaskJoyStickFlickUp, "TaskJoyStickFlickUp", 256, NULL, 2, &xTaskJoyStickFlickUp);
  xTaskCreate(TaskJoyStickFlickRight, "TaskJoyStickFlickRight", 256, NULL, 2, &xTaskJoyStickFlickRight);
  xTaskCreate(TaskJoyStickFlickLeft, "TaskJoyStickFlickLeft", 256, NULL, 2, &xTaskJoyStickFlickLeft);
  xTaskCreate(TaskJoyStickFlickDown, "TaskJoyStickFlickDown", 256, NULL, 2, &xTaskJoyStickFlickDown);
  xTaskCreate(TaskButtonPress, "TaskButtonPress", 256, NULL, 2, &xTaskButtonPress);
  xTaskCreate(TaskPotentiometerTwist, "TaskPotentiometerTwist", 256, NULL, 2, &xTaskPotentiometerTwist);
  xTaskCreate(TaskEndGame, "TaskEndGame", 256, NULL, 2, &xTaskEndGame);

  // Create countdown timer for game
  xCountdownTimer = xTimerCreate("CountTimer", pdMS_TO_TICKS(20), pdTRUE, (void *)0, vCountTimerCallback);

  // Store task handles for easy suspension/resume
  TasksHandles[0] = xTaskJoyStickPress;
  TasksHandles[1] = xTaskJoyStickFlickUp;
  TasksHandles[2] = xTaskJoyStickFlickRight;
  TasksHandles[3] = xTaskJoyStickFlickLeft;
  TasksHandles[4] = xTaskJoyStickFlickDown;
  TasksHandles[5] = xTaskButtonPress;
  TasksHandles[6] = xTaskPotentiometerTwist;

  // Suspend end game and display tasks initially
  vTaskSuspend(xTaskEndGame);
  vTaskSuspend(xTaskDisplaySeven);
  suspendTasks(); // Suspend all game action tasks

  xTimerStart(xCountdownTimer, 1); // Start countdown timer

  vTaskStartScheduler();           // Start FreeRTOS task scheduler
}

void loop() {
  // Empty loop as FreeRTOS handles all tasks
}

/* Example task to blink built-in LED (commented out)
void TaskBlink(void *pvParameters) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
*/

// Task to read analog value from pin A7 (example sensor)
void TaskAnalogRead(void *pvParameters) {
  for (;;) {
    int sensorValue = analogRead(A7);
    // Serial.println(sensorValue); // Uncomment to print sensor value
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task to blink an external LED
void TaskBlinkOffBoard(void *pvParameters) {
  pinMode(OFFBOARDLED, OUTPUT);
  for (;;) {
    digitalWrite(OFFBOARDLED, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(OFFBOARDLED, LOW);
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

// Task to update and refresh the 7-segment display
void TaskDisplaySeven(void *pvParameters) {
  const TickType_t xFrequency = pdMS_TO_TICKS(20);
  for (;;) {
    int displayValue = (int)(countValue * 1000); // Convert float to int for display
    myDisplay.setNumber(displayValue, 3);        // Show value on display
    myDisplay.refreshDisplay();                  // Refresh display
    vTaskDelay(xFrequency);
  }
}

// Timer callback to decrement countdown value
void vCountTimerCallback(TimerHandle_t xTimer) {
  countValue -= 0.002; // Decrement timer value
}

bool start = false; // Game start flag

// Main game state task: handles starting, running, and ending rounds
void TaskGameState(void *pvParameters) {
  bool playing = false;
  for (;;) {
    // Start game if button pressed or already started
    if ((digitalRead(BUTTONPIN) && !start) || start) {
      start = true;
      playing = true;
      currTask = (random(0, ARRAYSIZE)); // Pick random task
      countValue = 1.0;
      vTaskResume(xTaskDisplaySeven);    // Resume display task
      suspendTasks();                    // Suspend all game action tasks
      vTaskResume(TasksHandles[currTask]); // Resume chosen task
      lcd.clear();
      lcd.print(TASKDISPLAYS[currTask]); // Display current task

      TickType_t startTime = xTaskGetTickCount();
      bool taskTimedOut = false;
      while (playing) {
        if (taskFinished[currTask]) {
          // Task completed successfully
          playing = false;
          taskFinished[currTask] = false;
          suspendTasks();
        } else if (xTaskGetTickCount() - startTime >= pdMS_TO_TICKS(10000)) {
          // Task timed out (10 seconds)
          taskTimedOut = true;
          playing = false;
          start = false;
          suspendTasks();
          vTaskResume(xTaskEndGame); // Show end game screen
          vTaskSuspend(NULL);        // Suspend this task
        }
        vTaskDelay(20 / portTICK_PERIOD_MS); // Check every 20ms
      }
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for joystick button press action
void TaskJoyStickPress(void *pvParameters) {
  pinMode(JOYSTICKBUTTON, INPUT_PULLUP);
  lcd.clear();
  lcd.print("Joystick Push-It");
  for (;;) {
    if (digitalRead(JOYSTICKBUTTON) == LOW) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for detecting joystick flick right
void TaskJoyStickFlickRight(void *pvParameters) {
  int curr_y_val = analogRead(A0);
  int new_y_val = analogRead(A0);
  for (;;) {
    new_y_val = analogRead(A0);
    if (new_y_val - curr_y_val > 300) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    curr_y_val = new_y_val;
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for detecting joystick flick down
void TaskJoyStickFlickDown(void *pvParameters) {
  int curr_x_val = analogRead(A1);
  int new_x_val = analogRead(A1);
  for (;;) {
    new_x_val = analogRead(A1);
    if (curr_x_val - new_x_val > 300) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    curr_x_val = new_x_val;
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for detecting joystick flick left
void TaskJoyStickFlickLeft(void *pvParameters) {
  int curr_y_val = analogRead(A0);
  int new_y_val = analogRead(A0);
  for (;;) {
    new_y_val = analogRead(A0);
    if (curr_y_val - new_y_val > 300) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    curr_y_val = new_y_val;
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for detecting joystick flick up
void TaskJoyStickFlickUp(void *pvParameters) {
  int curr_x_val = analogRead(A1);
  int new_x_val = analogRead(A1);
  for (;;) {
    new_x_val = analogRead(A1);
    if (new_x_val - curr_x_val > 300) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    curr_x_val = new_x_val;
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for button press action (Bop-It)
void TaskButtonPress(void *pvParameters) {
  for (;;) {
    if (digitalRead(BUTTONPIN) == HIGH) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task for potentiometer twist action (Twist-It)
void TaskPotentiometerTwist(void *pvParameters) {
  int sensorValue = analogRead(A7);
  bool lastDirection = sensorValue > 512;
  bool currentDirection = analogRead(A7) > 512;
  for (;;) {
    currentDirection = analogRead(A7);
    // Detect direction change
    if ((currentDirection == 0 && lastDirection == 1) || (currentDirection == 1 && lastDirection == 0)) {
      taskFinished[currTask] = true; // Mark task as finished
    }
    lastDirection = currentDirection;
    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

// Task to handle end-of-game state and reset
void TaskEndGame(void *pvParameters) {
  lcd.clear();
  lcd.print("Game Over, Reset");
  lcd.setCursor(0, 1);
  lcd.print("to Play Again");
  myDisplay.blank();                // Clear 7-segment display
  vTaskSuspend(xTaskDisplaySeven);  // Suspend display task
  for (;;) {
    if (digitalRead(BUTTONPIN) == HIGH) {
      start = true;
      lcd.clear();
      vTaskResume(xTaskGameState);  // Resume game state task
      vTaskSuspend(NULL);           // Suspend this task
    }
    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

// Utility function to suspend all game action tasks
void suspendTasks() {
  for (int i = 0; i < ARRAYSIZE; i++) {
    vTaskSuspend(TasksHandles[i]);
  }
}
