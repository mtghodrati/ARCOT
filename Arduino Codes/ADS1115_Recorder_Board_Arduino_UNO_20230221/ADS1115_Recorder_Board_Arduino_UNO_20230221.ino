#include <Adafruit_ADS1X15.h>
#include <math.h>

// Thresholds and configurations for the system
#define BASELINE_SAMPLES_TOTAL 256  // Number of samples to calculate the baseline
#define THRESHOLD 5000              // High threshold for triggering a reward
#define LOWER_THRESHOLD 2500        // Lower threshold for resetting the trigger
#define TRIGGER_WIDTH 2000          // Duration of the trigger pulse (in microseconds)
#define FIRST_TIME_TRIGGER_DURATION 10  // Number of samples for the initial trigger

Adafruit_ADS1115 ads; /* Use this for the 16-bit version */

// Pin Configuration
constexpr int READY_PIN = 2;          // Pin to signal data readiness from ADC
int stepUpdatedPin = 3;               // Pin to detect step updates
int startPin = 4;                     // Pin to start/stop the system
int trigPin = 5;                      // Pin to output the trigger signal
int rewardPin = 7;                    // Pin to activate the reward mechanism
int readyToRewardPin = 8;             // Pin to check if reward is allowed
int stepIncPin = 9;                   // Pin for incrementing steps
int stepDecPin = 10;                  // Pin for decrementing steps
int initializedPin = 11;              // Pin to check initialization status

// Global variables for system state
volatile bool new_data = false;       // Flag for new data availability
volatile bool newStep = false;        // Flag for step update
volatile int16_t results1 = 0, results2 = 0;  // ADC readings
int16_t thrX = 0, thrY = 0;           // Threshold values for X and Y axes

// Timing variables
long startTime = 0;                   // Start time of the system
long previousTime = 0;                // Previous recorded time
long currTime = 0;                    // Current time
long trigTime = 0;                    // Last trigger activation time
long baselineSummationX = 0;          // Summation of X-axis values for baseline calculation
long baselineSummationY = 0;          // Summation of Y-axis values for baseline calculation

// System counters and flags
int nSampleBase = 0;                  // Number of baseline samples collected
int rewardNum = 0;                    // Number of rewards given
int firstTimeCount = 0;               // Counter for initial trigger duration
int isReadyToReward = 0;              // Flag to indicate readiness for reward
int startPinValue = 0;                // Value of the start pin
int currentStep = 0;                  // Current step value
int stepUpVal = 0, stepDownVal = 0;   // Flags for step increment and decrement

// System state flags
bool isActive = true;                 // Indicates if the system is active
bool returned = true;                 // Indicates if the system has returned to baseline
bool isFirstSample = true;            // Flag for the first sample after start
bool isFirstTimeAfterStart = true;    // Flag for the first activation after start

// Interrupt Service Routine (ISR) for new data readiness
void NewDataReadyISR() {
  new_data = true;
}

// ISR for step updates
void StepUpdatedISR() {
  newStep = true;
}

void setup(void) {
  // Configure pins for output
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(rewardPin, OUTPUT);
  digitalWrite(rewardPin, LOW);
  pinMode(readyToRewardPin, INPUT);

  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the ADS1115 ADC
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  // Configure interrupt pins
  pinMode(READY_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(READY_PIN), NewDataReadyISR, FALLING);
  pinMode(stepIncPin, INPUT);
  pinMode(stepDecPin, INPUT);
  pinMode(initializedPin, INPUT);
  pinMode(stepUpdatedPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(stepUpdatedPin), StepUpdatedISR, RISING);

  // Start continuous ADC conversions
  ads.setDataRate(128);
  ads.startADCReading(  , true);

  // Wait until the system is initialized
  while (digitalRead(initializedPin) != HIGH) {
    delay(1);
  }

  isFirstTimeAfterStart = false;

  // Perform baseline calculation
  baselineCalculation();

  Serial.print("High Threshold = ");
  Serial.print(THRESHOLD);
  Serial.print(", Low Threshold = ");
  Serial.println(LOWER_THRESHOLD);
  Serial.println("Time, value, baseline, currentStep, rewardNum");
  Serial.println("----------");

  startTime = micros();
  new_data = true;
}

void loop(void) {
  startPinValue = digitalRead(startPin);

  // If the system is not started, reset to the initial condition
  if (startPinValue != HIGH) {
    isFirstSample = true;
    isFirstTimeAfterStart = true;
    digitalWrite(trigPin, LOW);
    digitalWrite(rewardPin, LOW);
    return;
  } else if (isFirstTimeAfterStart) {
    // Recalculate baseline when the system restarts
    baselineCalculation();
    Serial.print("High Threshold = ");
    Serial.print(THRESHOLD);
    Serial.print(", Low Threshold = ");
    Serial.println(LOWER_THRESHOLD);
    Serial.println("Time, value, baseline, currentStep, rewardNum");
    Serial.println("----------");
    startTime = micros();
    isFirstTimeAfterStart = false;
  }

  // Keep track of steps
  if (newStep) {
    stepUpVal = digitalRead(stepIncPin);
    stepDownVal = digitalRead(stepDecPin);
    if (stepUpVal == HIGH && stepDownVal == HIGH) {
      currentStep = 0;
    } else if (stepUpVal == HIGH) {
      currentStep++;
    } else if (stepDownVal == HIGH) {
      currentStep--;
    }
    newStep = false;
  }

  // Reset trigger pin after the trigger width
  if (micros() - trigTime >= TRIGGER_WIDTH && !isFirstSample) {
    digitalWrite(trigPin, LOW);
  }

  // If there is no new data, return to the loop
  if (!new_data) {
    return;
  }

  // Process new ADC data
  currTime = micros() - startTime;
  results1 = ads.getLastConversionResults();
  digitalWrite(trigPin, HIGH);
  trigTime = micros();

  // Log data to the serial monitor
  Serial.print(currTime);
  Serial.print(",");
  Serial.print(results1);
  Serial.print(",");
  Serial.print(thrX);
  Serial.print(",");
  Serial.print(currentStep);
  Serial.print(",");
  Serial.print(rewardNum);
  Serial.println("$");

  new_data = false;

  // The first sample trigger is wider than the rest
  if (isFirstSample) {
    firstTimeCount++;
    if (firstTimeCount == FIRST_TIME_TRIGGER_DURATION + 1) {
      isFirstSample = false;
      digitalWrite(trigPin, LOW);
    }
  }

  // Rewarding
  isReadyToReward = digitalRead(readyToRewardPin);
  if (abs(results1 - thrX) > THRESHOLD && isReadyToReward == HIGH && returned) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(rewardPin, HIGH);
    delayMicroseconds(3000);
    digitalWrite(rewardPin, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    rewardNum++;
    returned = false;
  }

  // Reset reward state when below lower threshold
  if (abs(results1 - thrX) < LOWER_THRESHOLD) {
    returned = true;
    digitalWrite(rewardPin, LOW);
  }
}

void baselineCalculation() {
  Serial.println("Calculating Baseline...");
  while (nSampleBase <= BASELINE_SAMPLES_TOTAL) {
    if (new_data) {
      results1 = ads.getLastConversionResults();
      baselineSummationX += results1;
      nSampleBase++;
      new_data = false;
    }
    delay(1);
  }
  thrX = (int16_t)((float)baselineSummationX / nSampleBase);
}