#include "math.h"
#include <Servo.h>

// NOTE: Trigger Pin: 30

//
//
#define REWARDS_PER_DEGREE 5      // Defines the number of rewards per degree of rotation.
#define FIRST_DIRECTION 1         // Sets the first direction of rotation: 0 = backward, 1 = forward.
#define TIMES_TO_REWARD 1         // Number of times to trigger a reward.
#define DIR_PIN 4                 // Direction pin for controlling the stepper motor's direction.
#define STEP_PIN 5                // Step pin for controlling the stepper motor's stepping.
#define M0pin 10                  // Pin for controlling microstep settings on the stepper motor driver.
#define M1pin 11                  // Pin for controlling microstep settings on the stepper motor driver.
#define M2pin 12                  // Pin for controlling microstep settings on the stepper motor driver.
#define ROTATION 25               // Number of degrees the servo will rotate in each movement.
#define SERVO_ROTATION 45         // The rotation range for the servo motor (degrees).
#define STEPS_PER_REVOLUTION 200  // Number of steps for the stepper motor to complete one full revolution.
#define MICROSTEP_STATUS 1        // Defines whether microstepping is enabled (0 = no microstepping, 1 = microstepping at 1/2 step).
#define WATER_REWARD_DURATION 20  // Duration of water reward (in arbitrary time units).
#define LICK_DURATION 1500        // Duration for the licking behavior (in arbitrary time units).
#define BLINK_PERIOD 250          // Period for blinking the LED, in milliseconds.

int servoPos = 45;              // Initial servo position.


// OBJECTS
Servo myservo;                  // Create an object to control the servo motor.


// PINS
// Pin assignments for various control signals.
int servoPin = 2;               // Pin connected to the servo motor.
int relayPin = 3;               // Pin connected to the relay for controlling power or other devices.
int ledPin = 9;                 // Pin for controlling the reward LED (lights up when a reward is given).
int blinkingLEDpin = 6;         // Pin for controlling the blinking LED (used for status indication).
int rewardInputPin = 7;         // Pin for receiving reward trigger signal from another device (e.g., recorder Arduino).
int readyToRewardStatusPin = 8; // Pin to indicate the status of reward availability to another device.
int switchPin = 13;             // Microswitch pin used to detect an angle or position.
int initializedPin = 24;        // Pin set to HIGH when the stepper motor is initialized, indicating readiness.
int stepIncPin = 26;            // Pin that becomes HIGH to indicate step increment action.
int stepDecPin = 27;            // Pin that becomes HIGH to indicate step decrement action.
int stepUpdatedPin = 28;        // Pin for signaling step update to another system or device.
int startPin = 29;              // Pin that signals the start of the experiment or process.
int trigPin = 30;               // Pin used as a trigger for specific actions in the system.
int dirChangePin = 20;          // Pin used to change the direction of motor rotation based on external input.

int motorTrigPin = 40;          // Pin for controlling the motor trigger.
int servoTrigPin = 41;          // Pin for controlling the servo trigger.
int solenoidTrigPin = 42;       // Pin for controlling a solenoid (if used in the setup).


// the setup routine runs once when you press reset:
// Variables to keep track of system status.
int relayHighCount = 0;         // Counter to keep track of how many times the relay is triggered.
int rewardCount = 0;            // Counter to keep track of the number of rewards given.
int currentRewards = 0;         // Stores the current number of rewards for the system.
int current_degree = 0;         // Current degree of rotation for the motor or servo.
int stepsPerRevolution = STEPS_PER_REVOLUTION * (MICROSTEP_STATUS + 1);  // Adjusted steps per revolution considering microstepping.
int times = 0;                  // Counter for time or attempts before rewarding.
int switchStatus = 0;           // Stores the status of the microswitch (whether it is pressed or not).
double degPerStep = 360 / ((double)stepsPerRevolution);  // Degrees per step for the motor, used for calculating rotation.
int blinkCounter = 0;           // Counter to control the blinking LED.


// FLAGS
// Flags used to track the status of different actions in the system.
bool isRelayHigh = false;       // Flag to indicate whether the relay is activated.
bool blinkingLEDstat = false;   // Flag to control the state of the blinking LED.
bool reward = false;            // Flag to track if a reward is triggered.
bool rewardCondition = false;   // Flag to indicate if the system is in a state to give a reward.
bool isForward = true;          // Flag to track whether the rotation is in the forward direction (true) or backward (false).
bool isServoActive = true;      // Flag to track whether the servo is active or being used in the system.

void initialize_degree();
void stepIncrementTrig();
void stepDecrementTrig();

void ChangeDirISR() {
  change_direction();
}

// Rotates the joystick by a fixed angle (22.5 degrees).
void rotate_joystick() {
  rotate_joystick_deg(22.5);  // Calls the function to rotate by 22.5 degrees.
}

// Rotates the joystick by the specified number of degrees.
void rotate_joystick_deg(double deg) {
  digitalWrite(motorTrigPin, HIGH);  // Activate the motor trigger to start movement.
  
  // Calculate the number of steps based on the degrees to rotate.
  int steps = (int)(deg / degPerStep);
  
  // Check if direction change is needed based on the current degree and limits.
  if (isForward && ((double)current_degree + (double)steps * degPerStep > 115)) {
    change_direction();  // Change direction if the movement exceeds the limit.
  } else if (!isForward && ((double)current_degree - (double)steps * degPerStep < -115)) {
    change_direction();  // Change direction if the movement exceeds the negative limit.
  }

  // Update current degree based on the movement direction.
  if (isForward) {
    current_degree += steps * degPerStep;
  } else {
    current_degree -= steps * degPerStep;
  }
  
  // Perform the steps to rotate the motor.
  for (int i = 0; i < steps; i++) {
    // Perform one step: pulse the step pin to move the motor.
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100000);  // Wait for the step duration.
    digitalWrite(STEP_PIN, LOW);

    // Increment or decrement the step depending on the direction.
    if (isForward) {
      stepIncrementTrig();  // Trigger step increment for forward motion.
    } else {
      stepDecrementTrig();  // Trigger step decrement for reverse motion.
    }
  }
  
  digitalWrite(motorTrigPin, LOW);  // Deactivate the motor trigger after movement.
}


void change_direction() {
  isForward = !isForward;
  if (isForward) {
    digitalWrite(DIR_PIN, HIGH);
  } else {
    digitalWrite(DIR_PIN, LOW);
  }
}

void setup() {
  // Initialize all pins as input or output as needed for the setup.
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(M0pin, OUTPUT);
  pinMode(M1pin, OUTPUT);
  pinMode(M2pin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(blinkingLEDpin, OUTPUT);
  pinMode(rewardInputPin, INPUT);
  pinMode(readyToRewardStatusPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(initializedPin, OUTPUT);
  pinMode(stepIncPin, OUTPUT);
  pinMode(stepDecPin, OUTPUT);
  pinMode(switchPin, INPUT);
  pinMode(startPin, INPUT);
  pinMode(stepUpdatedPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(motorTrigPin, OUTPUT);
  pinMode(solenoidTrigPin, OUTPUT);
  pinMode(servoTrigPin, OUTPUT);
  // pinMode(dirChangePin, INPUT);  // Unused, commented out

  // Attach servo control to the servoPin
  myservo.attach(servoPin);
  
  // Set up an interrupt to change direction when the pin triggers
  attachInterrupt(digitalPinToInterrupt(dirChangePin), ChangeDirISR, RISING);

  // Set initial pin states to LOW as part of the setup.
  digitalWrite(initializedPin, LOW);
  digitalWrite(stepIncPin, LOW);
  digitalWrite(stepDecPin, LOW);
  digitalWrite(stepUpdatedPin, LOW);
  digitalWrite(trigPin, LOW);
  myservo.write(0);  // Set the servo to initial position
  digitalWrite(readyToRewardStatusPin, HIGH);
  digitalWrite(relayPin, HIGH);  // Activate relay
  digitalWrite(motorTrigPin, LOW);
  digitalWrite(solenoidTrigPin, LOW);
  digitalWrite(servoTrigPin, LOW);

  // Optional delay for initial setup time
  // delay(1000); 

  // Set initial LED state and motor direction
  digitalWrite(ledPin, LOW);
  digitalWrite(DIR_PIN, HIGH);  // Set motor direction
  if (MICROSTEP_STATUS == 1) {
    digitalWrite(M0pin, HIGH);  // Set microstep pin based on the status
  } else {
    digitalWrite(M0pin, LOW);
  }
  digitalWrite(M1pin, LOW);  // Set microstep pins
  digitalWrite(M2pin, LOW);

  // Blink the built-in LED to signal setup completion
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  // Wait for the start pin to be HIGH before continuing
  while (digitalRead(startPin) != HIGH) {
  }

  // Initialize the joystick's degree position
  initialize_degree();
}

void loop() {
  // Check if startPin is HIGH, else exit loop
  if (digitalRead(startPin) != HIGH) {
    return;
  }

  // Check for reward condition
  rewardCondition = digitalRead(rewardInputPin);
  if (rewardCondition) {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn on built-in LED
    times++;

    // If the set number of rewards is reached
    if (times == TIMES_TO_REWARD) {
      digitalWrite(trigPin, HIGH);  // Trigger reward action
      digitalWrite(readyToRewardStatusPin, LOW);  // Update reward status
      rewardCount++;
      currentRewards++;

      // Control blinking LED and main LED
      digitalWrite(blinkingLEDpin, LOW);
      digitalWrite(ledPin, HIGH);

      // If servo is active, move servo for reward dispensing
      if (isServoActive) {
        digitalWrite(servoTrigPin, HIGH);
        for (servoPos = 0; servoPos < SERVO_ROTATION; servoPos++) {
          myservo.write(servoPos);  // Rotate servo
          delay(10);
        }
        digitalWrite(servoTrigPin, LOW);
        delay(50);
      }

      // Dispense water reward
      digitalWrite(relayPin, LOW);
      delay(WATER_REWARD_DURATION);
      digitalWrite(relayPin, HIGH);
      delay(LICK_DURATION);

      // Reverse servo motion after reward
      if (isServoActive) {
        digitalWrite(servoTrigPin, HIGH);
        for (servoPos = SERVO_ROTATION; servoPos > -1; servoPos--) {
          myservo.write(servoPos);
          delay(10);
        }
        digitalWrite(servoTrigPin, LOW);
      }

      // Reset reward status and prepare for next reward
      digitalWrite(ledPin, LOW);
      digitalWrite(readyToRewardStatusPin, HIGH);
      times = 0;
      digitalWrite(trigPin, LOW);
    }

    digitalWrite(LED_BUILTIN, LOW);  // Turn off built-in LED
  }

  // Blink LED at regular intervals
  blinkCounter++;
  if (blinkCounter >= BLINK_PERIOD) {
    blinkingLEDstat = !blinkingLEDstat;  // Toggle blinking LED state
    digitalWrite(blinkingLEDpin, blinkingLEDstat);
    blinkCounter = 0;
  }

  // Check if enough rewards have been given to rotate joystick
  if (currentRewards >= REWARDS_PER_DEGREE) {
    if ((current_degree == 0) || (current_degree == 45) || (current_degree == 90) || (current_degree == -45) || (current_degree == -90)) {
      if (currentRewards >= 2 * REWARDS_PER_DEGREE) {
        digitalWrite(trigPin, HIGH);  // Trigger joystick rotation
        rotate_joystick();
        digitalWrite(trigPin, LOW);
        currentRewards = 0;
      }
    } else {
      digitalWrite(trigPin, HIGH);
      rotate_joystick();
      digitalWrite(trigPin, LOW);
      currentRewards = 0;
    }
    delay(1);  // Delay between reads for stability
  }
}


// Initialize the degree and reset the stepper motor
void initialize_degree() {
  int cnt = 0;
  int sw = digitalRead(switchPin);
  
  // Move stepper motor until switch is pressed or steps per revolution is reached
  while (sw != LOW && cnt < stepsPerRevolution) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100000);
    cnt++;
    delay(1);
    sw = digitalRead(switchPin);  // Read switch state
    Serial.println(sw);  // Print switch state for debugging
  }
  
  delay(500);  // Wait for 500ms
  change_direction();  // Change motor direction
  for (int i = 0; i < stepsPerRevolution / 2; i++) {
    // Perform one step
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100000);
  }

  // Change direction if specified
  if (FIRST_DIRECTION == 1) {
    change_direction();
  }
  
  current_degree = 0;  // Reset current degree to 0
  
  // Indicate initialization is complete
  digitalWrite(initializedPin, HIGH);
  
  // Notify the other Arduino that orientation degree is set to 0
  digitalWrite(stepIncPin, HIGH);
  digitalWrite(stepDecPin, HIGH);
  digitalWrite(stepUpdatedPin, HIGH);
  delayMicroseconds(500);
  digitalWrite(stepUpdatedPin, LOW);
  delay(25);
  digitalWrite(stepIncPin, LOW);
  digitalWrite(stepDecPin, LOW);
  delay(1000);  // Wait for 1 second before finishing
}

// Trigger step increment action
void stepIncrementTrig() {
  digitalWrite(stepIncPin, HIGH);  // Set step increment pin high
  digitalWrite(stepUpdatedPin, HIGH);  // Set step updated pin high
  delayMicroseconds(500);  // Wait for 500 microseconds
  digitalWrite(stepUpdatedPin, LOW);  // Set step updated pin low
  delay(2);  // Small delay
  digitalWrite(stepIncPin, LOW);  // Set step increment pin low
}

// Trigger step decrement action
void stepDecrementTrig() {
  digitalWrite(stepDecPin, HIGH);  // Set step decrement pin high
  digitalWrite(stepUpdatedPin, HIGH);  // Set step updated pin high
  delayMicroseconds(500);  // Wait for 500 microseconds
  digitalWrite(stepUpdatedPin, LOW);  // Set step updated pin low
  delay(2);  // Small delay
  digitalWrite(stepDecPin, LOW);  // Set step decrement pin low
}
