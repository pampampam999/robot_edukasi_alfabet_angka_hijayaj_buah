#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Network credentials
bool wifiConnected = false;
String newSSID = "";
String newPassword = "";

unsigned long wifiDisconnectedTime = 0;
bool wifiDisconnectedFlag = false;
const unsigned long WIFI_RECONNECT_TIMEOUT = 120000; // 2 minutes

unsigned long wifiReconnectStartTime = 0;
bool isReconnecting = false;

// WiFi AP
const char *ssidAP = "Robot Edukasi";
const char *passwordAP = "admin123";

// ------------ EEPROM ------------
const int EEPROM_SIZE = 512;
const int MAX_SSID_LENGTH = 32;
const int MAX_PASSWORD_LENGTH = 64;

// IP local 192.168.4.1

// EEPROM addresses
const int EEPROM_SSID_ADDR = 0;
const int EEPROM_PASS_ADDR = EEPROM_SSID_ADDR + MAX_SSID_LENGTH + 1;

// Motor pins
// #define MOTOR_RIGHT_PIN_1 5
// #define MOTOR_RIGHT_PIN_2 18
// #define MOTOR_LEFT_PIN_1 19
// #define MOTOR_LEFT_PIN_2 21
// #define MOTOR_RIGHT_EN 13
// #define MOTOR_LEFT_EN 12

// Motor A
int motorAPin1 = 12; 
int motorAPin2 = 14;
int enableAPin = 13;

// Motor B
int motorBPin1 = 26; 
int motorBPin2 = 25;
int enableBPin = 27; 

// Motor C
int motorCPin1 = 19; 
int motorCPin2 = 23;
int enableCPin = 18; 

// Motor pins
#define MOTOR_SATU_PIN_1 12
#define MOTOR_SATU_PIN_2 14
#define MOTOR_DUA_PIN_1 26
#define MOTOR_DUA_PIN_2 25
#define MOTOR_TIGA_PIN_1 19
#define MOTOR_TIGA_PIN_2 23
#define MOTOR_SATU_EN 13
#define MOTOR_DUA_EN 27
#define MOTOR_TIGA_EN 18

// WiFi Reset PIN
#define RESET_BUTTON_PIN 4
#define RESET_BUTTON_HOLD_TIME 5000  // 5 seconds hold time for reset

// PWM configuration
#define PWM_FREQUENCY    5000
#define PWM_RESOLUTION   8
#define PWM_CHANNEL_LEFT 0
#define PWM_CHANNEL_RIGHT 1

// Safety configurations
#define COMMAND_TIMEOUT  1000    // 1 second timeout for commands
#define MAX_SPEED       255     // Maximum PWM value
#define ACCELERATION    10      // Speed change per update
#define MOTOR_UPDATE_INTERVAL 20 // Motor update interval in milliseconds

// Setting PWM properties bi
const int freq = 1000;
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmChannelC = 2;
const int resolution = 8;
int dutyCycle = 200;

// Command structure
struct Command {
    const char* action;
    int speed;
    unsigned long timestamp;
};

// Motor control structure
struct MotorState {
    int targetSpeed;
    int currentSpeed;
    bool forward;
    unsigned long lastUpdate;
};

MotorState motorSatu = {0, 0, true, 0};
MotorState motorDua = {0, 0, true, 0};
MotorState motorTiga = {0, 0, true, 0};
unsigned long lastCommandTime = 0;

unsigned long resetButtonPressTime = 0;
bool resetButtonPressed = false;

AsyncWebServer server(80);

const char HTML_CONFIG_PAGE[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Robot WiFi Setup</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin:0; padding:0; }
        .container { max-width: 400px; margin: 20px auto; padding: 20px; background:#fff; border-radius:5px; box-shadow:0 2px 4px rgba(0,0,0,0.1); }
        h1 { text-align: center; }
        label { display:block; margin:10px 0 5px; }
        input { width:100%; padding:8px; margin-bottom:10px; border:1px solid #ddd; border-radius:4px; }
        button { width:100%; padding:10px; background:#007bff; color:#fff; border:none; border-radius:4px; cursor:pointer; }
        button:hover { background:#0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Robot WiFi Setup</h1>
        <form action="/configure" method="POST">
            <label for="ssid">SSID:</label>
            <input type="text" id="ssid" name="ssid">
            <label for="password">Password:</label>
            <input type="password" id="password" name="password">
            <button type="submit">Configure</button>
        </form>
    </div>
</body>
</html>
)";

const char CONTROL_PAGE[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <title>Robot Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { 
        font-family: Arial; 
        text-align: center; 
        margin: 0px auto; 
        padding-top: 30px;
      }
      .button {
        background-color: #2f4468;
        border: none;
        color: white;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 18px;
        margin: 6px 3px;
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <div class="control-section">
      <h1>Robot Control Panel</h1>
      <table style="margin: 0 auto;">
        <tr>
          <td colspan="3" align="center">
            <button class="button" onmousedown="sendCommand('forward');" ontouchstart="sendCommand('forward');" onmouseup="sendCommand('stop');" ontouchend="sendCommand('stop');">Forward</button>
          </td>
        </tr>
        <tr>
          <td align="center">
            <button class="button" onmousedown="sendCommand('left');" ontouchstart="sendCommand('left');" onmouseup="sendCommand('stop');" ontouchend="sendCommand('stop');">Left</button>
          </td>
          <td align="center">
            <button class="button" onmousedown="sendCommand('stop');" ontouchstart="sendCommand('stop');">Stop</button>
          </td>
          <td align="center">
            <button class="button" onmousedown="sendCommand('right');" ontouchstart="sendCommand('right');" onmouseup="sendCommand('stop');" ontouchend="sendCommand('stop');">Right</button>
          </td>
        </tr>
        <tr>
          <td colspan="3" align="center">
            <button class="button" onmousedown="sendCommand('backward');" ontouchstart="sendCommand('backward');" onmouseup="sendCommand('stop');" ontouchend="sendCommand('stop');">Backward</button>
          </td>
        </tr>
      </table>
    </div>
    <script>
      function sendCommand(action) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/action?go=" + action, true);
        xhr.send();
      }
    </script>
  </body>
</html>
)";

const char CONFIG_SUCCESS_PAGE[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Configuration Success</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .message { max-width: 400px; margin: 0 auto; padding: 20px; background: #f0f0f0; border-radius: 5px; }
        .redirect { margin-top: 20px; font-style: italic; }
    </style>
    <meta http-equiv="refresh" content="5;url=/control">
</head>
<body>
    <div class="message">
        <h2>Configuration Saved</h2>
        <p>Your WiFi settings have been saved. The robot is now connecting to your network.</p>
        <p class="redirect">You will be redirected to the control panel in 5 seconds...</p>
    </div>
</body>
</html>
)";

void connectToWiFi();
void saveWiFiCredentials(String ssid, String password);
void handleConfiguration(AsyncWebServerRequest *request);

void handleWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnected = false;
      wifiDisconnectedFlag = true;
      wifiDisconnectedTime = millis();
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi connected");
      wifiConnected = true;
      wifiDisconnectedFlag = false;
      break;

    default:
      break;
  }
}

void tryReconnectWiFi() {
  if (!wifiConnected && wifiDisconnectedFlag) {
    if (millis() - wifiDisconnectedTime > WIFI_RECONNECT_TIMEOUT) {
      Serial.println("Reconnection failed after 2 minutes, switching to AP mode...");
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ssidAP, passwordAP);
      wifiDisconnectedFlag = false;
      isReconnecting = false;
    } else {
      if (!isReconnecting) {
        Serial.println("Attempting to reconnect WiFi...");
        WiFi.begin(newSSID.c_str(), newPassword.c_str());
        wifiReconnectStartTime = millis();
        isReconnecting = true;
      }
      if (isReconnecting && millis() - wifiReconnectStartTime > 15000) {
        Serial.println("Reconnection attempt timed out.");
        isReconnecting = false;
      }
    }
  }
}

void saveWiFiCredentials(String ssid, String password) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.writeString(EEPROM_SSID_ADDR, ssid);
  EEPROM.writeString(EEPROM_PASS_ADDR, password);
  EEPROM.commit();
  EEPROM.end();
}

void loadWiFiCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  String storedSSID = EEPROM.readString(EEPROM_SSID_ADDR);
  String storedPassword = EEPROM.readString(EEPROM_PASS_ADDR);
  EEPROM.end();

  if (storedSSID.length() > 0 && storedPassword.length() > 0) {
    Serial.println("Loading WiFi credentials from EEPROM...");
    newSSID = storedSSID;
    newPassword = storedPassword;

    // Switch to STA mode
    WiFi.mode(WIFI_STA);
    connectToWiFi();
  } else {
    Serial.println("No WiFi credentials found. Starting in AP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAP, passwordAP);
    Serial.println("AP mode active. Please configure WiFi.");
  }
}

void connectToWiFi() {
  if (newSSID.length() == 0 || newPassword.length() == 0) {
    // If no credentials are available
    return;
  }

  Serial.println("Connecting to WiFi...");
  WiFi.begin(newSSID.c_str(), newPassword.c_str());
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\nFailed to connect. Returning to AP mode.");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAP, passwordAP);
    wifiConnected = false;
  }
}

void resetWiFiCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  // Clear SSID area
  for (int i = EEPROM_SSID_ADDR; i < EEPROM_SSID_ADDR + MAX_SSID_LENGTH; i++) {
      EEPROM.write(i, 0);
  }
  // Clear password area
  for (int i = EEPROM_PASS_ADDR; i < EEPROM_PASS_ADDR + MAX_PASSWORD_LENGTH; i++) {
      EEPROM.write(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
  
  newSSID = "";
  newPassword = "";
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passwordAP);
  
  Serial.println("WiFi credentials reset. AP mode activated.");
}

void handleConfiguration(AsyncWebServerRequest *request) {
  bool hasSSID = false;
  bool hasPassword = false;
  
  int params = request->params();
  for(int i=0; i<params; i++){
    const AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "ssid") {
        newSSID = p->value();
        hasSSID = true;
      }
      if (p->name() == "password") {
        newPassword = p->value();
        hasPassword = true;
      }
    }
  }

  if (hasSSID && hasPassword) {
    // Save the credentials
    saveWiFiCredentials(newSSID, newPassword);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(newSSID.c_str(), newPassword.c_str());
    
    String successPage = F("<!DOCTYPE html>"
                          "<html>"
                          "<head>"
                          "    <title>Configuration Success</title>"
                          "    <style>"
                          "        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }"
                          "        .message { max-width: 500px; margin: 0 auto; padding: 20px; background: #f0f0f0; border-radius: 5px; }"
                          "        .status { margin-top: 20px; font-weight: bold; }"
                          "        #countdown { font-weight: bold; }"
                          "        #control-link { display: none; margin-top: 20px; }"
                          "        .control-btn { padding: 10px 20px; background-color: #4CAF50; color: white; "
                          "                       text-decoration: none; border-radius: 4px; }"
                          "    </style>"
                          "    <script>"
                          "        var seconds = 30;"
                          "        function checkConnection() {"
                          "            var xhr = new XMLHttpRequest();"
                          "            xhr.open('GET', '/connection-status', true);"
                          "            xhr.onload = function() {"
                          "                if (xhr.status === 200) {"
                          "                    var response = JSON.parse(xhr.responseText);"
                          "                    if (response.connected) {"
                          "                        document.getElementById('status-msg').innerHTML = 'Connected to WiFi successfully!';"
                          "                        document.getElementById('ip-address').innerHTML = response.ip;"
                          "                        document.getElementById('control-link').style.display = 'block';"
                          "                        document.getElementById('control-link').href = 'http://' + response.ip + '/control';"
                          "                        document.getElementById('countdown-container').style.display = 'none';"
                          "                        clearInterval(checkInterval);"
                          "                    }"
                          "                }"
                          "            };"
                          "            xhr.send();"
                          "            seconds--;"
                          "            if (seconds <= 0) {"
                          "                document.getElementById('status-msg').innerHTML = 'Connection taking longer than expected. Please try:';"
                          "                document.getElementById('options').innerHTML = '<li>Check your WiFi password</li><li>Restart the device</li>';"
                          "                clearInterval(checkInterval);"
                          "            } else {"
                          "                document.getElementById('countdown').innerHTML = seconds;"
                          "            }"
                          "        }"
                          "        var checkInterval = setInterval(checkConnection, 1000);"
                          "        checkConnection();"
                          "    </script>"
                          "</head>"
                          "<body>"
                          "    <div class='message'>"
                          "        <h2>Configuration Saved</h2>"
                          "        <p>Your WiFi settings have been saved. The robot is now connecting to your network.</p>"
                          "        <div class='status'>"
                          "            <p id='status-msg'>Connecting to WiFi...</p>"
                          "            <p id='countdown-container'>Checking status: <span id='countdown'>30</span> seconds remaining</p>"
                          "            <ul id='options'></ul>"
                          "        </div>"
                          "        <p>When connected, your robot's IP address will be: <span id='ip-address'>determining...</span></p>"
                          "        <a id='control-link' class='control-btn' href='#'>Go to Control Panel</a>"
                          "    </div>"
                          "</body>"
                          "</html>");
                          
    request->send(200, "text/html", successPage);
  } else {
    request->send(400, "text/plain", "Missing SSID or password");
  }
}

void setupServerRoutes() {
  // Serve WiFi configuration page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", HTML_CONFIG_PAGE);
  });

  // Handle WiFi configuration form submission
  server.on("/configure", HTTP_POST, handleConfiguration);

  // Handle WiFi config to routes control page
  server.on("/connection-status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(128);
    doc["connected"] = WiFi.status() == WL_CONNECTED;
    doc["ip"] = WiFi.localIP().toString();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/reset-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    String resetPage = F(R"(
    <!DOCTYPE html>
    <html>
    <head>
    <title>Reset WiFi Configuration</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .container { max-width: 400px; margin: 0 auto; padding: 20px; }
        .warning { color: #ff4444; margin: 20px 0; }
        .button { 
            display: inline-block;
            padding: 10px 20px;
            margin: 10px;
            border-radius: 4px;
            text-decoration: none;
            color: white;
        }
        .reset { background-color: #ff4444; }
        .cancel { background-color: #666666; }
    </style>
    </head>
    <body>
    <div class="container">
        <h2>Reset WiFi Configuration</h2>
        <p class="warning">Warning: This will erase all saved WiFi settings!</p>
        <p>The robot will return to AP mode after reset.</p>
        <a href="/perform-reset" class="button reset">Reset WiFi</a>
        <a href="/control" class="button cancel">Cancel</a>
    </div>
    </body>
    </html>
        )");
        request->send(200, "text/html", resetPage);
    });

    // Add a route to perform the actual reset
    server.on("/perform-reset", HTTP_GET, [](AsyncWebServerRequest *request){
        resetWiFiCredentials();
        request->redirect("/");
    });

  // Serve the control page
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", CONTROL_PAGE);
  });

  // Handle robot control commands
  server.on("/action", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("go")) {
      request->send(400, "text/plain", "Missing 'go' parameter");
      return;
    }
    
    String actionStr = request->getParam("go")->value();
    const char* actionCStr = actionStr.c_str();
    
    Command cmd;
    cmd.action = actionCStr;
    cmd.speed = request->hasParam("speed") ? request->getParam("speed")->value().toInt() : 50;
    cmd.timestamp = millis();
    
    // Validate action
    if (strcmp(cmd.action, "forward") != 0 && 
        strcmp(cmd.action, "backward") != 0 && 
        strcmp(cmd.action, "left") != 0 && 
        strcmp(cmd.action, "right") != 0 && 
        strcmp(cmd.action, "stop") != 0) {
        request->send(400, "text/plain", "Invalid action");
        return;
    }
    
    processCommand(cmd);
    request->send(200, "text/plain", "OK");
  });

  // 404 Not Found handler
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
}

void checkResetButton() {
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      if (!resetButtonPressed) {
          resetButtonPressed = true;
          resetButtonPressTime = millis();
      } else if (millis() - resetButtonPressTime >= RESET_BUTTON_HOLD_TIME) {
          resetWiFiCredentials();
          resetButtonPressed = false;
      }
  } else {
      resetButtonPressed = false;
  }
}

void setMotorSpeed(MotorState& motor, int channel, int speed) {
  speed = constrain(speed, 0, MAX_SPEED);
  ledcWrite(channel, speed);
  motor.currentSpeed = speed;
}

void setMotorDirection(MotorState& motor, int pin1, int pin2, bool forward) {
  motor.forward = forward;
  digitalWrite(pin1, forward ? HIGH : LOW);
  digitalWrite(pin2, forward ? LOW : HIGH);
}

void updateMotor(MotorState& motor, int channel) {
  unsigned long currentTime = millis();
  
  if (currentTime - motor.lastUpdate >= MOTOR_UPDATE_INTERVAL) {
      if (motor.currentSpeed < motor.targetSpeed) {
          setMotorSpeed(motor, channel, motor.currentSpeed + ACCELERATION);
      } else if (motor.currentSpeed > motor.targetSpeed) {
          setMotorSpeed(motor, channel, motor.currentSpeed - ACCELERATION);
      }
      motor.lastUpdate = currentTime;
  }
}

// void stopMotors() {
//   leftMotor.targetSpeed = 0;
//   rightMotor.targetSpeed = 0;
  
//   // Immediately stop motors
//   setMotorSpeed(leftMotor, PWM_CHANNEL_LEFT, 0);
//   setMotorSpeed(rightMotor, PWM_CHANNEL_RIGHT, 0);
  
//   // Set all direction pins to LOW
//   digitalWrite(MOTOR_LEFT_PIN_1, LOW);
//   digitalWrite(MOTOR_LEFT_PIN_2, LOW);
//   digitalWrite(MOTOR_RIGHT_PIN_1, LOW);
//   digitalWrite(MOTOR_RIGHT_PIN_2, LOW);
// }

void processCommand(const Command& cmd) {
  // Update last command time
  lastCommandTime = millis();
  
  // Calculate PWM speed (0-255)
  int pwmSpeed = map(constrain(cmd.speed, 0, 100), 0, 100, 0, MAX_SPEED);
  
  if (strcmp(cmd.action, "forward") == 0) {
      // setMotorDirection(leftMotor, MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, true);
      // setMotorDirection(rightMotor, MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, true);
      // leftMotor.targetSpeed = pwmSpeed;
      // rightMotor.targetSpeed = pwmSpeed;
      Serial.println("forward");
      moveForward();
  }
  else if (strcmp(cmd.action, "backward") == 0) {
      // setMotorDirection(leftMotor, MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, false);
      // setMotorDirection(rightMotor, MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, false);
      // leftMotor.targetSpeed = pwmSpeed;
      // rightMotor.targetSpeed = pwmSpeed;
      Serial.println("backward");
      moveBackward();
  }
  else if (strcmp(cmd.action, "left") == 0) {
      // setMotorDirection(leftMotor, MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, false);
      // setMotorDirection(rightMotor, MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, true);
      // leftMotor.targetSpeed = pwmSpeed;
      // rightMotor.targetSpeed = pwmSpeed;
      Serial.println("left");
      rotasiCCW();
  }
  else if (strcmp(cmd.action, "right") == 0) {
      // setMotorDirection(leftMotor, MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, true);
      // setMotorDirection(rightMotor, MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, false);
      // leftMotor.targetSpeed = pwmSpeed;
      // rightMotor.targetSpeed = pwmSpeed;
      Serial.println("right");
      rotasiCW();
  }
  else if (strcmp(cmd.action, "stop") == 0) {
      stopMotors();
      Serial.println("stop");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Initial free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Configure motor control pins
  // pinMode(MOTOR_RIGHT_PIN_1, OUTPUT);
  // pinMode(MOTOR_RIGHT_PIN_2, OUTPUT);
  // pinMode(MOTOR_LEFT_PIN_1, OUTPUT);
  // pinMode(MOTOR_LEFT_PIN_2, OUTPUT);

  // sets the pins as outputs:
  pinMode(motorAPin1, OUTPUT);
  pinMode(motorAPin2, OUTPUT);
  pinMode(motorBPin1, OUTPUT);
  pinMode(motorBPin2, OUTPUT);
  pinMode(motorCPin1, OUTPUT);
  pinMode(motorCPin2, OUTPUT);
  
  // Configure LEDC for PWM
  //ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQUENCY, PWM_RESOLUTION);
  //ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQUENCY, PWM_RESOLUTION);
  
  // Attach PWM channels to enable pins
  //ledcAttachPin(MOTOR_LEFT_EN, PWM_CHANNEL_LEFT);
  //ledcAttachPin(MOTOR_RIGHT_EN, PWM_CHANNEL_RIGHT);
  
  // Initialize reset button pin
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  stopMotors();
  
  WiFi.onEvent(handleWiFiEvent);
  loadWiFiCredentials();
  
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);  // Disable modem sleep
  
  setupServerRoutes();
  
  server.begin();
  
  Serial.println("Server started");
  if (WiFi.getMode() == WIFI_AP) {
    Serial.println("Running in AP mode");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  if (WiFi.getMode() == WIFI_STA) {
    tryReconnectWiFi();
    delay(100);
  }
  
  if (millis() - lastCommandTime > COMMAND_TIMEOUT) {
    stopMotors();
  }
  
  checkResetButton();
  //updateMotor(leftMotor, PWM_CHANNEL_LEFT);
  //updateMotor(rightMotor, PWM_CHANNEL_RIGHT);
}

void stopMotors() {
  // Enable Motor memeberikan pwm 0
  ledcWrite(enableAPin, 0);
  ledcWrite(enableBPin, 0);  
  ledcWrite(enableCPin, 0);  
  
  // LOW pada semua PIN IN
  digitalWrite(motorAPin1, LOW);
  digitalWrite(motorAPin2, LOW);
  digitalWrite(motorBPin1, LOW);
  digitalWrite(motorBPin2, LOW);
  digitalWrite(motorCPin1, LOW);
  digitalWrite(motorCPin2, LOW);

  // Serial.println("Motor stopped");
}

void motorCW(int motor, int pwm){
  switch(motor){
    case 1:
      digitalWrite(motorAPin1, LOW);
      digitalWrite(motorAPin2, HIGH);
      analogWrite(enableAPin, pwm);
      Serial.print("Motor ");Serial.print(motor);
      Serial.println(" Moving CW");
    break;
      
    case 2:
      digitalWrite(motorBPin1, LOW);
      digitalWrite(motorBPin2, HIGH);
      analogWrite(enableBPin, pwm);
      Serial.print("Motor ");Serial.print(motor);
      Serial.println(" Moving CW");
    break;
    
    case 3:
      digitalWrite(motorCPin1, LOW);
      digitalWrite(motorCPin2, HIGH);
      analogWrite(enableCPin, pwm);
      Serial.print("Motor ");Serial.print(motor);
      Serial.println(" Moving CW");
    break;
    default:
    Serial.println("Motor CW PIN Salah");
    break;
  }
}

void motorCCW(int motor, int pwm){
  switch(motor){
    case 1:
      digitalWrite(motorAPin1, HIGH);
      digitalWrite(motorAPin2, LOW);
      analogWrite(enableAPin, pwm);
      Serial.print("Motor ");Serial.print(motor);
      Serial.println(" Moving CCW");
    break;
      
    case 2:
      digitalWrite(motorBPin1, HIGH);
      digitalWrite(motorBPin2, LOW);
      analogWrite(enableBPin, pwm);
      Serial.print("Motor ");Serial.print(motor);
      Serial.println(" Moving CCW");
    break;
    
    case 3:
      digitalWrite(motorCPin1, HIGH);
      digitalWrite(motorCPin2, LOW);
      analogWrite(enableCPin, pwm);
      Serial.print("Motor ");Serial.print(motor);
      Serial.println(" Moving CCW");
    break;
    default:
    Serial.println("Motor CCW PIN Salah");
    break;
  }
}

void moveForward(){
  motorCW(1,dutyCycle);
  motorCCW(2,dutyCycle);
  Serial.println("robot maju");
}

void moveBackward(){
  motorCCW(1,dutyCycle);
  motorCW(2,dutyCycle);
  Serial.println("robot mundur");
}

void rotasiCW(){
  motorCCW(1,dutyCycle);
  motorCCW(2,dutyCycle);
  motorCCW(3,dutyCycle);
  Serial.println("robot rotasi kanan");
}

void rotasiCCW(){
  motorCW(1,dutyCycle);
  motorCW(2,dutyCycle);
  motorCW(3,dutyCycle);
  Serial.println("robot rotasi kiri");
}