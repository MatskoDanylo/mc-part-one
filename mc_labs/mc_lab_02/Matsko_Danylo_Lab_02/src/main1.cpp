#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define SSID "ESP8266-AP"
#define PASSWORD "012345678900"
#define BLINK_INTERVAL 1000
#define HOLD_INTERVAL 500

#define IS_AP true // set to false for ESP #2


enum class Color {
  RED,
  YELLOW,
  GREEN
};

typedef struct led_s {
  const uint8_t pin;
  bool state;
  led_s *next;
  led_s *prev;
  Color color;
} led_t;

typedef struct button_s {
  uint8_t pin;
  bool state;
  bool wasPressed;
  uint32_t pressStartTime;
  bool hardIsHeld;
  bool webIsHeld;
  bool serialIsHeld;
} button_t;

led_t redLED = {13, LOW, nullptr, nullptr, Color::RED}; // GPIO2
led_t yellowLED = {14, LOW, nullptr, nullptr, Color::YELLOW}; // GPIO14
led_t greenLED = {2, LOW, nullptr, nullptr, Color::GREEN}; // GPIO13

button_t button = {12, LOW, false, 0, false, false, false}; //GPIO12

led_t *currentLED = &redLED;
uint32_t currentTime;
uint32_t previousBlinkTime = 0;
uint8_t serialData;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleHold(AsyncWebServerRequest *request) {
  button.webIsHeld = true;
  request->send_P(200, "text/html", "ok");
}

void handleReleased(AsyncWebServerRequest *request) {
  button.webIsHeld = false;
  request->send_P(200, "text/html", "ok");
}

void sendStartSignal(AsyncWebServerRequest *request) {
  Serial.print("h");
  request->send_P(200, "text/html", "ok");
}

void sendStopSignal(AsyncWebServerRequest *request) {
  Serial.print("r");
  request->send_P(200, "text/html", "ok");
}

void sendCurrentLEDtoWEB() {
  if (currentLED == nullptr) {
    return;
  }
  switch (currentLED->color) {
    case Color::RED:
      ws.textAll("red");
      break;
    case Color::YELLOW:
      ws.textAll("yellow");
      break;
    case Color::GREEN:
      ws.textAll("green");
      break;
  }
}

void setupLEDOrder() {
  redLED.next = &yellowLED;
  redLED.prev = &greenLED;

  yellowLED.next = &greenLED;
  yellowLED.prev = &redLED;

  greenLED.next = &redLED;
  greenLED.prev = &yellowLED;
}

void pinSetup() {
  pinMode(redLED.pin, OUTPUT);
  pinMode(yellowLED.pin, OUTPUT);
  pinMode(greenLED.pin, OUTPUT);
  pinMode(button.pin, INPUT_PULLUP);
}

void serverSetup() {
  if (IS_AP) {
    WiFi.softAP(SSID, PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  }

  // common server setup
  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/hold", HTTP_GET, handleHold);
  server.on("/release", HTTP_GET, handleReleased);
  server.on("/start", HTTP_GET, sendStartSignal);
  server.on("/stop", HTTP_GET, sendStopSignal);
  server.addHandler(&ws);
  server.begin();
}


void lightLEDs() {
  digitalWrite(redLED.pin, redLED.state);
  digitalWrite(yellowLED.pin, yellowLED.state);
  digitalWrite(greenLED.pin, greenLED.state);
}

void lightNextLED() {
  currentTime = millis();
  
  // Determine if any button input is active (hardware, web, or serial)
  bool effectivePressed = button.state || button.webIsHeld || button.serialIsHeld;
  
  // These static variables help manage the sequence across calls.
  static uint32_t buttonPressedTime = 0;
  static bool prevEffectivePressed = false;
  static bool normalLEDOn = false;
  
  // Detect a change in the button’s (effective) pressed state.
  if (effectivePressed != prevEffectivePressed) {
    if (effectivePressed) {
      // Button pressed now; record when this happened.
      buttonPressedTime = currentTime;
    } else {
      // Button was released;
      // mimic second-code behavior where on release we update currentLED to (currentLED + 2)%3.
      // In our circular linked list, this is equivalent to:
      currentLED = currentLED->prev;
      normalLEDOn = false;
      previousBlinkTime = currentTime;
    }
    prevEffectivePressed = effectivePressed;
  }
  
  // Wait until the blink (or step) interval passes.
  if (currentTime - previousBlinkTime < BLINK_INTERVAL)
    return;
  previousBlinkTime = currentTime;
  
  if (effectivePressed) {
    // When the button is held...
    if (currentTime - buttonPressedTime < 2000) {
      // For less than 2000ms: run the normal sequence: toggle current LED.
      if (!normalLEDOn) {
        // Turn off all LEDs …
        redLED.state = LOW;
        yellowLED.state = LOW;
        greenLED.state = LOW;
        // … and light up the current one.
        currentLED->state = HIGH;
        normalLEDOn = true;
      } else {
        // Now turn off the current LED, reset, and move to the next LED.
        currentLED->state = LOW;
        normalLEDOn = false;
        currentLED = currentLED->next;
      }
    } else {
      // Held for longer than 2000ms: run the held sequence.
      // Clear all LED states:
      redLED.state = LOW;
      yellowLED.state = LOW;
      greenLED.state = LOW;
      // Light up both neighboring LEDs (which corresponds to (currentLED+1)%3 and (currentLED+2)%3).
      currentLED->next->state = HIGH;
      currentLED->prev->state = HIGH;
      // And advance the sequence pointer (as in the second code’s held sequence).
      currentLED = currentLED->next;
    }
  } else {
    // When no button is pressed, continue running the normal sequence.
    if (!normalLEDOn) {
      redLED.state = LOW;
      yellowLED.state = LOW;
      greenLED.state = LOW;
      currentLED->state = HIGH;
      normalLEDOn = true;
    } else {
      currentLED->state = LOW;
      normalLEDOn = false;
      currentLED = currentLED->next;
    }
  }
  
  // Send update to web and apply the LED states.
  sendCurrentLEDtoWEB();
  lightLEDs();
}

void handleButtonHold() {
  button.state = digitalRead(button.pin) == LOW;

  if (button.state) {
    if (!button.wasPressed) {
      button.wasPressed = true;
      button.pressStartTime = millis();
    } else if (millis() - button.pressStartTime >= HOLD_INTERVAL) {
      button.hardIsHeld = true;
    }
  } else {
    if (button.wasPressed) {
      button.hardIsHeld = false;
    }
    button.wasPressed = false;
  }
}

void checkSerial() {
  if (Serial.available() > 0) {
    serialData = Serial.read();
    switch (serialData) {
      case 'h':
        button.serialIsHeld = true;
        break;
      case 'r':
        button.serialIsHeld = false;
        break;
    }
  }
}

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  setupLEDOrder();
  pinSetup();
  serverSetup();
}

void loop() {
  handleButtonHold();
  checkSerial();
  lightNextLED();
  ws.cleanupClients();
}