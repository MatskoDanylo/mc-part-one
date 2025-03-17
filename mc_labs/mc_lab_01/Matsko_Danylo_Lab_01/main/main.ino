#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <stdint.h>

const char* ssid = "Brawl Pass";
const char* password = "012345678900";

const uint8_t BUTTON_PIN = 12;
const uint8_t LED1 = 13;
const uint8_t LED2 = 14;
const uint8_t LED3 = 2;

const uint8_t ledPins[3] = {LED1, LED2, LED3};

unsigned long lastStepTime = 0;
const unsigned long stepDelay = 1000;

unsigned long buttonPressTime = 0;

uint8_t currentLED = 0;
bool normalLEDOn = false;

bool lastButtonPressed = false; 
bool virtualButtonPressed = false;

ESP8266WebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>LED Algorithm Control</title>
  <style>
    .btn {
      padding: 10px 20px;
      background-color: #4CAF50;
      border: none;
      border-radius: 5px;
      color: #FFF;
      font-size: 16px;
      cursor: pointer;
    }
    .btn:hover {
      background-color: #45a049;
    }
  </style>
</head>
<body>
  <button class="btn" id="webButton">Hold to start</button>
  <script>
    var btn = document.getElementById('webButton');
    btn.addEventListener('mousedown', function() {
      fetch('/buttonDown');
    });
    btn.addEventListener('mouseup', function() {
      fetch('/buttonUp');
    });
    // For mobile devices:
    btn.addEventListener('touchstart', function() {
      fetch('/buttonDown');
    });
    btn.addEventListener('touchend', function() {
      fetch('/buttonUp');
    });
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleButtonDown() {
  virtualButtonPressed = true;
  server.send(200, "text/plain", "Virtual button down");
}

void handleButtonUp() {
  virtualButtonPressed = false;
  server.send(200, "text/plain", "Virtual button up");
}

void runNormalSequence() {
  if (millis() - lastStepTime >= stepDelay) {
    lastStepTime = millis();
    if (!normalLEDOn) {
      for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(ledPins[i], LOW);
      }
      digitalWrite(ledPins[currentLED], HIGH);
      normalLEDOn = true;
    } else {
      digitalWrite(ledPins[currentLED], LOW);
      normalLEDOn = false;
      currentLED = (currentLED + 1) % 3;
    }
  }
}

void runHeldSequence() {
  if (millis() - lastStepTime >= stepDelay) {
    lastStepTime = millis();
    for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], LOW);
    }
    digitalWrite(ledPins[(currentLED + 1) % 3], HIGH);
    digitalWrite(ledPins[(currentLED + 2) % 3], HIGH);
    currentLED = (currentLED + 1) % 3;
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();
  unsigned long lastDotPrint = startAttemptTime;
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < 15000)) {
    if (millis() - lastDotPrint >= 100) {
      Serial.print(".");
      lastDotPrint = millis();
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("ESP8266 IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }

  server.on("/", handleRoot);
  server.on("/buttonDown", handleButtonDown);
  server.on("/buttonUp", handleButtonUp);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  bool physButton = (digitalRead(BUTTON_PIN) == LOW);
  bool effectiveButtonPressed = physButton || virtualButtonPressed;

  if (effectiveButtonPressed != lastButtonPressed) {
    if (effectiveButtonPressed) {
      buttonPressTime = millis();
    } else {  
      currentLED = (currentLED + 2) % 3;
      normalLEDOn = false;
      lastStepTime = millis();
    }
    lastButtonPressed = effectiveButtonPressed;
  }
  
  if (effectiveButtonPressed) {
    if (millis() - buttonPressTime < 2000) {
      runNormalSequence();
    } else {
      runHeldSequence();
    }
  } else {
    runNormalSequence();
  }
}
