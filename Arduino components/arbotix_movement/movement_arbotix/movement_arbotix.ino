#include <ax12.h>
#include <BioloidController.h>

BioloidController controller = BioloidController(1000000);
const int MaxServoID = 5;
bool systemReady = false;

// Servo position limits
const int MIN_POSITION = 200;
const int MAX_POSITION = 800;
const int CENTER_POSITION = 512;

const int min_move = 30;

// Track position for each servo
int servo_positions[6] = {0, CENTER_POSITION, CENTER_POSITION, CENTER_POSITION, CENTER_POSITION, CENTER_POSITION}; // Index 0 unused, 1-5 for servos
int current_servo = 1;

// Movement speed
const int MOVE_SPEED = 50;

void handleServoControl(char input) {
  // Servo selection
  if(input >= '1' && input <= '5') {
    int selectedServo = input - '0';
    if(selectedServo != current_servo){
      current_servo = selectedServo;
    }
    Serial.print("Selected Servo #");
    Serial.println(current_servo);
    return;
  }
  
  // Control commands
  switch(input) {
    case 'L':
    case 'l':
      Serial.print("Moving Servo #");
      Serial.print(current_servo);
      Serial.println(" LEFT");
      servo_positions[current_servo] -= min_move;
      servo_positions[current_servo] = constrain(servo_positions[current_servo], MIN_POSITION, MAX_POSITION);
      moveServoSmooth(current_servo, servo_positions[current_servo]);
      break;
      
    case 'R':
    case 'r':
      Serial.print("Moving Servo #");
      Serial.print(current_servo);
      Serial.println(" RIGHT");
      servo_positions[current_servo] += min_move;
      servo_positions[current_servo] = constrain(servo_positions[current_servo], MIN_POSITION, MAX_POSITION);
      moveServoSmooth(current_servo, servo_positions[current_servo]);
      break;
      
    case 'C':
    case 'c':
      Serial.print("Centering Servo #");
      Serial.println(current_servo);
      servo_positions[current_servo] = CENTER_POSITION;
      moveServoSmooth(current_servo, CENTER_POSITION);
      break;
      
    case 'S':
    case 's':
      Serial.print("Stopping Servo #");
      Serial.println(current_servo);
      stopServo(current_servo);
      break;
      
    case 'P':
    case 'p':
      printAllPositions();
      break;
      
    case 'H':
    case 'h':
      //printMenu();
      break;
  }
}

void moveServoSmooth(int servoID, int targetPosition) {
  // Set movement speed
  dxlSetGoalSpeed(servoID, MOVE_SPEED);
  
  // Move to target position
  SetPosition(servoID, targetPosition);
  
  Serial.print("Servo #");
  Serial.print(servoID);
  Serial.print(" -> Position: ");
  Serial.println(targetPosition);
}

void stopServo(int servoID) {
  // Get current position and set it as goal to stop movement
  int currentPos = dxlGetPosition(servoID);
  SetPosition(servoID, currentPos);
  servo_positions[servoID] = currentPos; // Update tracked position
}

void printAllPositions() {
  Serial.println("\n=== Current Servo Positions ===");
  for(int i = 1; i <= MaxServoID; i++) {
    int pos = dxlGetPosition(i);
    Serial.print("Servo #");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(pos);
    Serial.print(" / ");
    Serial.println(MAX_POSITION);
  }
  Serial.println("===============================\n");
}

void checkVoltage() {
  float voltage = dxlGetSystemVoltage(MaxServoID); 
  Serial.print("##### System Voltage: ");
  Serial.print(voltage);
  Serial.println(" volts.");
  if (voltage >= 10.0){
    Serial.println("ALL GOOD: Voltage is above 10 V");
    return;
  }
  Serial.println("PROBLEM: Voltage level below minimum (10 V)");
  Serial.println("Program stopped");
  while(true);
}

void moveServo01(char input){
  // Always control servo 1
  int selectedServo = 1;
  
  // Control commands
  switch(input) {
    case 'L':
    case 'l':
      servo_positions[selectedServo] -= min_move;
      servo_positions[selectedServo] = constrain(servo_positions[selectedServo], MIN_POSITION, MAX_POSITION);
      moveServoSmooth(selectedServo, servo_positions[selectedServo]);
      break;
      
    case 'R':
    case 'r':
      servo_positions[selectedServo] += min_move;
      servo_positions[selectedServo] = constrain(servo_positions[selectedServo], MIN_POSITION, MAX_POSITION);
      moveServoSmooth(selectedServo, servo_positions[selectedServo]);
      break;
      
    case 'C':
    case 'c':
      servo_positions[selectedServo] = CENTER_POSITION;
      moveServoSmooth(selectedServo, CENTER_POSITION);
      break;
      
    case 'S':
    case 's':
      stopServo(selectedServo);
      break;
  }
}

void moveServo03(char input){
  // Always control servo 1
  int selectedServo = 3;
  
  // Control commands
  switch(input) {
    case 'U':
    case 'u':
      servo_positions[selectedServo] -= min_move;
      servo_positions[selectedServo] = constrain(servo_positions[selectedServo], MIN_POSITION, MAX_POSITION);
      moveServoSmooth(selectedServo, servo_positions[selectedServo]);
      break;
      
    case 'D':
    case 'd':
      servo_positions[selectedServo] += min_move;
      servo_positions[selectedServo] = constrain(servo_positions[selectedServo], MIN_POSITION, MAX_POSITION);
      moveServoSmooth(selectedServo, servo_positions[selectedServo]);
      break;
      
    case 'C':
    case 'c':
      servo_positions[selectedServo] = CENTER_POSITION;
      moveServoSmooth(selectedServo, CENTER_POSITION);
      break;
      
    case 'S':
    case 's':
      stopServo(selectedServo);
      break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("");   
  Serial.println("#### Serial Communication Established.");
  checkVoltage();
  
  // Initialize to custom position
  int pos_arr[5] = {511, 610, 207, 311, 511};
  for(int i = 1; i <= MaxServoID; i++){
    SetPosition(i, pos_arr[i-1]);
    servo_positions[i] = pos_arr[i-1]; // Store initial positions
    delay(100);
  }
  
  Serial.println("#### WARNING: Ensure ALL 4 servos can freely rotate");
  Serial.println("Type 'R' once you are ready"); 
}

void loop() {
  if(Serial.available() > 0) {
    char input = Serial.read();
    
    // Wait for ready command
    if(!systemReady && (input == 'R' || input == 'r')) {
      systemReady = true;
      Serial.println("\n#### SYSTEM READY ####");
      return;
    }
    
    if(systemReady) {
      moveServo01(input);
      moveServo03(input);
    }
  }
}