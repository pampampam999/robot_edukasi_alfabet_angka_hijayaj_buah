// define pin reciver
#define D0_PIN 25
#define D1_PIN 26
#define D2_PIN 27
#define D3_PIN 14

// Pin definitions for L298N motor driver
#define IN1 33
#define IN2 32
#define IN3 13
#define IN4 12

// TAMBAHI NA NB TOR

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

    // Initialize motors
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    Serial.println("RF Receiver and Motor Driver Ready");
}

void loop() {
    int d0_state = digitalRead(D0_PIN);
    int d1_state = digitalRead(D1_PIN);
    int d2_state = digitalRead(D2_PIN);
    int d3_state = digitalRead(D3_PIN);

    if (d0_state == HIGH) {
        Serial.println("Button D0 pressed - Move Forward");
        moveForward();
    } else if (d1_state == HIGH) {
        Serial.println("Button D1 pressed - Move Backward");
        moveBackward();
    } else if (d2_state == HIGH) {
        Serial.println("Button D2 pressed - Turn Left");
        turnLeft();
    } else if (d3_state == HIGH) {
        Serial.println("Button D3 pressed - Turn Right");
        turnRight();
    } else {
        //stopMotors();
    }

    delay(100); // Debounce delay
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
    //ledcWrite(0, 255); // Set ENA speed to max
    //ledcWrite(1, 255); // Set ENB speed to max
}

void turnRight() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    //ledcWrite(0, 255); // Set ENA speed to max
    //ledcWrite(1, 255); // Set ENB speed to max
}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    //ledcWrite(0, 0); // Set ENA speed to 0
    //ledcWrite(1, 0); // Set ENB speed to 0
}
