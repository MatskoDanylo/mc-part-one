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

// Змінні для збереження стану алгоритмів
uint8_t currentLED = 0;   // Індекс світлодіода (0 ... 2)
bool normalLEDOn = false; // Стан для звичайного режиму (ON/OFF)

bool lastButtonPressed = false; 
bool virtualButtonPressed = false;

ESP8266WebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <title>Керування алгоритмом світлодіодів</title>
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
    // Для підтримки мобільних пристроїв:
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

// Функція для звичайного почергового блимання
void runNormalSequence() {
  if (millis() - lastStepTime >= stepDelay) {
    lastStepTime = millis();
    if (!normalLEDOn) {
      for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(ledPins[i], LOW);
      }
      digitalWrite(ledPins[currentLED], HIGH);
      normalLEDOn = true;
      Serial.print("Звичайний режим: діод ");
      Serial.print(currentLED);
      Serial.println(" ON");
    } else {
      digitalWrite(ledPins[currentLED], LOW);
      normalLEDOn = false;
      Serial.print("Звичайний режим: діод ");
      Serial.print(currentLED);
      Serial.println(" OFF");
      currentLED = (currentLED + 1) % 3;
    }
  }
}

// Функція для режиму при утриманні кнопки.
void runHeldSequence() {
  if (millis() - lastStepTime >= stepDelay) {
    lastStepTime = millis();
    for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], LOW);
    }
    digitalWrite(ledPins[(currentLED + 1) % 3], HIGH);
    digitalWrite(ledPins[(currentLED + 2) % 3], HIGH);
    Serial.print("Режим при утриманні: діоди ");
    Serial.print((currentLED + 1) % 3);
    Serial.print(" та ");
    Serial.print((currentLED + 2) % 3);
    Serial.println(" ON");
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
  Serial.print("Підключення до WiFi");
  unsigned long startAttemptTime = millis();
  unsigned long lastDotPrint = startAttemptTime;
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < 15000)) {
    if (millis() - lastDotPrint >= 100) {
      Serial.print(".");
      lastDotPrint = millis();
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nПідключено!");
    Serial.print("IP адреса ESP8266: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nНе вдалося підключитися до WiFi");
  }

  server.on("/", handleRoot);
  server.on("/buttonDown", handleButtonDown);
  server.on("/buttonUp", handleButtonUp);
  server.begin();
  Serial.println("Веб-сервер запущено");
}

void loop() {
  server.handleClient();

  bool physButton = (digitalRead(BUTTON_PIN) == LOW);
  bool effectiveButtonPressed = physButton || virtualButtonPressed;

  if (effectiveButtonPressed != lastButtonPressed) {
    lastStepTime = millis();
    for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], LOW);
    }
    if (!effectiveButtonPressed) {  
      currentLED = (currentLED + 2) % 3;
      normalLEDOn = false;
      Serial.print("Режим нормального блимання починається з діода ");
      Serial.println(currentLED);
    }
    lastButtonPressed = effectiveButtonPressed;
  }
  
  if (effectiveButtonPressed) {
    runHeldSequence();
  } else {
    runNormalSequence();
  }
}
