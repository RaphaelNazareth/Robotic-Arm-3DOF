#include <Arduino.h>
#include <math.h>
#include <Servo.h>

#define outputA 2
#define outputB 3

// Servo objects for controlling the servos
Servo servo1;
Servo servo2;
Servo servo3;

const int limitSwitchPin = 4;
int counter = 0;
int aState;
int aLastState;
float angle_per_pulse = 2.25; // Encoder step size
float current_angle = 0;
float desired_angle = 0;
unsigned long lastMovementTime = 0;
const unsigned long timeout = 7000;

bool angleSet = false;

// PWM pulse widths for the servo (in microseconds)
const int servoStop = 1500;    // Neutral
const int servoForward = 1630; // Clockwise
const int servoBackward = 1360; // Counterclockwise

const float deadZone = 3.0; // Dead zone size in degrees (±3°)

const float yi_offset = 7.5;
const int zero_point_T3 = 235;

const float L1 = 12.0; // Length of the first link
const float L2 = 16.0; // Length of the second link

// Function to calculate inverse kinematics for a 2-link planar robot
void calculate_thetas(float x, float y, float &theta1, float &theta2) {
  float distance = sqrt(x * x + y * y);

  if (distance > (L1 + L2) || distance < fabs(L1 - L2)) {
    Serial.println("Point is out of reach for the given link lengths.");
    return;
  }

  float alpha = acos((L1 * L1 + distance * distance - L2 * L2) / (2 * L1 * distance));
  float beta = acos((L1 * L1 + L2 * L2 - distance * distance) / (2 * L1 * L2));
  float phi = atan2(y, x);

  theta1 = phi + alpha;  // Elbow-down configuration
  theta2 = M_PI - beta;  // Angle at the second joint

  theta1 = degrees(theta1);
  theta2 = degrees(theta2);
}

float calculate_theta3(float xi, float yi) {
  if (yi >= 0) {
    yi = yi + 8;
    if (yi == 0) {
      Serial.println("Invalid input for theta3 calculation: yi + 6 cannot be zero.");
      return -999;
    }
  } else {
    yi = yi - 8;
    if (yi == 0) {
      Serial.println("Invalid input for theta3 calculation: yi + 6 cannot be zero.");
      return -999;
    }
  }

  float theta3_rad = atan2(xi, yi);
  float theta3_deg = degrees(theta3_rad);

  if (xi == 0 && yi <= 0) {
    theta3_deg = -(zero_point_T3 - theta3_deg);
  } else if (xi > 0 && yi <= 0) {
    theta3_deg = -(180 - theta3_deg);
  } else {
    theta3_deg = -(theta3_deg + zero_point_T3);
  }

  if (theta3_deg < -235) {
    theta3_deg -= 14;
  }

  return theta3_deg;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Wait for the serial port to connect.
  }

  servo1.attach(9);     // Servo arm bawah
  servo2.attach(10);    // Servo arm atas
  servo3.attach(11);    // Servo 360deg base
  servoMag.attach(6)    // Servo pencet tombol magnet

  servo1.write(130);
  servo2.write(90);

  pinMode(outputA, INPUT);
  pinMode(outputB, INPUT);
  pinMode(limitSwitchPin, INPUT_PULLUP);
  myServo.writeMicroseconds(servoStop); // Initialize servo to stop

  Serial.begin(9600);
  Serial.println("Enter the desired angle of rotation to begin:");
  aLastState = digitalRead(outputA);

  // Find limit switch with timeout
  unsigned long startTime = millis();
  while (digitalRead(limitSwitchPin) == HIGH && millis() - startTime < 10000) {
      myServo.writeMicroseconds(servoForward); // Move forward to find the limit switch
      delay(10);
  }
  myServo.writeMicroseconds(servoStop); // Stop servo after finding limit
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    int comma = input.indexOf(',');

    if (comma > 0) {
      String xi_str = input.substring(0, comma);
      String yi_str = input.substring(comma + 1);

      float xi = xi_str.toFloat();
      float yi = yi_str.toFloat();

      yi += yi_offset;
      float target_y = 3;
      float x = sqrt(xi * xi + yi * yi);

      for (float current_y = 18; current_y >= target_y; current_y -= 3) {
        float theta1, theta2;
        calculate_thetas(x, current_y, theta1, theta2);
        float theta3 = calculate_theta3(xi, yi);

        if (theta3 == -999) {
          return;
        }

        Serial.print("For y = ");
        Serial.print(current_y);
        Serial.print(", Calculated angles - Theta 1: ");
        Serial.print(theta1, 2);
        Serial.print("\xC2\xB0, Theta 2: ");
        Serial.print(theta2, 2);
        Serial.print("\xC2\xB0, Theta 3: ");
        Serial.println(theta3, 2);

        int servo1_angle = constrain(map(theta1, 0, 180, 0, 180), 0, 180);
        int servo2_angle = constrain(map(theta2, 0, 180, 0, 180), 0, 180);
        int servo3_angle = constrain(map(theta3, -235, 235, 0, 180), 0, 180);

        servo1.write(servo1_angle);
        servo2.write(servo2_angle);
        servo3.write(servo3_angle);

        delay(3500);
      }

      for (float current_y = target_y; current_y <= 18; current_y += 3) {
        float theta1, theta2;
        calculate_thetas(x, current_y, theta1, theta2);
        float theta3 = calculate_theta3(xi, yi);

        if (theta3 == -999) {
          return;
        }

        Serial.print("For y = ");
        Serial.print(current_y);
        Serial.print(", Calculated angles - Theta 1: ");
        Serial.print(theta1, 2);
        Serial.print("\xC2\xB0, Theta 2: ");
        Serial.print(theta2, 2);
        Serial.print("\xC2\xB0, Theta 3: ");
        Serial.println(theta3, 2);

        int servo1_angle = constrain(map(theta1, 0, 180, 0, 180), 0, 180);
        int servo2_angle = constrain(map(theta2, 0, 180, 0, 180), 0, 180);
        int servo3_angle = constrain(map(theta3, -235, 235, 0, 180), 0, 180);

        servo1.write(servo1_angle);
        servo2.write(servo2_angle);
        servo3.write(servo3_angle);

        delay(3500);
      }

      // After completing everything, reset the servo angles
      servo1.write(90); // Theta 1 = 130
      servo2.write(70);  // Theta 2 = 90
      delay(2000);
      servo1.write(110); // Theta 1 = 130
      servo2.write(80);  // Theta 2 = 90
      delay(2000);
      servo1.write(130); // Theta 1 = 130
      servo2.write(90);  // Theta 2 = 90
      delay(2000);
      Serial.println("Servos reset to Theta 1 = 130, Theta 2 = 90.");
    } else {
      Serial.println("Invalid input format. Please enter in the format: xi, yi");
    }
  }

  delay(100);
}
