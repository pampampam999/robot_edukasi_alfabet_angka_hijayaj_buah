// define pin reciver
#define D3_PIN 34
#define D2_PIN 35
#define D1_PIN 32
#define D0_PIN 33

// Pin definitions for L298N motor driver
#define ENA 25
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12
#define ENB 13

void setup() {
  Serial.begin(9600);

  // Setup pin reciver
  pinMode(D0_PIN, INPUT);
  pinMode(D1_PIN, INPUT);
  pinMode(D2_PIN, INPUT);
  pinMode(D3_PIN, INPUT);

  // Setup motor driver pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

   // Setup PWM channels
    ledcSetup(0, 5000, 8);  // Channel 0, 5 kHz, 8-bit resolution
    ledcSetup(1, 5000, 8);  // Channel 1, 5 kHz, 8-bit resolution

    // Attach PWM channels to GPIO pins
    ledcAttachPin(ENA, 0);  // Attach ENA to channel 0
    ledcAttachPin(ENB, 1);  // Attach ENB to channel 1
  

  // Initialize motors
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

  Serial.println("Ready");
}

void loop() {
  int buttonB = digitalRead(D0_PIN);   // Button B
  int buttonD = digitalRead(D1_PIN);   // Button D
  int buttonA = digitalRead(D2_PIN);   // Button A
  int buttonC = digitalRead(D3_PIN);   // Button C

    if (buttonA == HIGH) {
        Serial.println("Button D0 pressed - Move Forward");
        moveForward();
    } else if (buttonD == HIGH) {
        Serial.println("Button D1 pressed - Move Backward");
        moveBackward();
    } else if (buttonC == HIGH) {
        Serial.println("Button D2 pressed - Turn Left");
        turnLeft();
    } else if (buttonB == HIGH) {
        Serial.println("Button D3 pressed - Turn Right");
        turnRight();
    } else {
        stopMotors();
    }
}

void moveForward() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(0, 255); // Set ENA speed to max
    ledcWrite(1, 255); // Set ENB speed to max
}

void moveBackward() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(0, 255); // Set ENA speed to max
    ledcWrite(1, 255); // Set ENB speed to max
}

void turnLeft() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(0, 255); // Set ENA speed to max
    ledcWrite(1, 255); // Set ENB speed to max
}

void turnRight() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(0, 255); // Set ENA speed to max
    ledcWrite(1, 255); // Set ENB speed to max
}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(0, 0); // Set ENA speed to 0
    ledcWrite(1, 0); // Set ENB speed to 0
}
