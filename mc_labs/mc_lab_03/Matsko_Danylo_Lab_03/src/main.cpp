#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>

// OLED 128x64 I2C: SDA = D2, SCL = D1
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Wi-Fi
const char* ssid = "Brawl Pass";
const char* password = "012345678900";

const char* apiURL = "http://api.exchangerate.host/latest?base=USD&symbols=UAH,EUR,PLN";

const unsigned long updateInterval = 5 * 60 * 1000;
unsigned long lastUpdate = 0;

const unsigned long scrollInterval = 5000;
unsigned long lastScroll = 0;

const char* currencies[] = {"UAH", "EUR", "PLN"};
const int numCurrencies = 3;
float rates[numCurrencies]; 

int currentCurrencyIndex = 0;

void showMessage(const String& line1, const String& line2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); 
  u8g2.drawStr(0, 20, line1.c_str());
  u8g2.drawStr(0, 45, line2.c_str());
  u8g2.sendBuffer();
}

void updateExchangeRate() {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, apiURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc; 
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      for (int i = 0; i < numCurrencies; i++) {
        const char* curr = currencies[i];
        rates[i] = doc["rates"][curr];
        Serial.printf("Currency %s: %.2f\n", curr, rates[i]);
      }
    } else {
      showMessage("Error", "JSON parse fail");
    }
  } else {
    showMessage("Error", "HTTP failed");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  u8g2.begin();
  showMessage("Connecting to", "WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  showMessage("Connected!", WiFi.localIP().toString());
  delay(2000);

  updateExchangeRate(); 
  lastUpdate = millis();
  lastScroll = millis();
}

void loop() {
  if (millis() - lastUpdate > updateInterval) {
    updateExchangeRate();
    lastUpdate = millis();
  }

  if (millis() - lastScroll > scrollInterval) {
    char title[20];
    snprintf(title, sizeof(title), "USD -> %s", currencies[currentCurrencyIndex]);

    char value[20];
    snprintf(value, sizeof(value), "%.2f %s", rates[currentCurrencyIndex], currencies[currentCurrencyIndex]);

    showMessage(title, value);

    currentCurrencyIndex++;
    if (currentCurrencyIndex >= numCurrencies) {
      currentCurrencyIndex = 0;
    }

    lastScroll = millis();
  }
}
