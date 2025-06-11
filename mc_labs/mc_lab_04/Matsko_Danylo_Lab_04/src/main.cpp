#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h> // для часу (через NTP)

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi & MQTT
const char* ssid = "Brawl Pass";
const char* password = "012345678900";
const char* mqtt_server = "9a5d1102c4ff49e28175850c39141c2c.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "ESP8266V";
const char* mqtt_pass = "Esp8266V";
const char* mqtt_topic = "emqx/esp8266";

WiFiClientSecure secureClient;
PubSubClient client(secureClient);

void showConnectingAnimation() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.print("Connected");
  display.setCursor(20, 35);
  for (int i = 0; i < 5; i++) {
    display.print(".");
    display.display();
    delay(1000);
  }
}

void showWaiting() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextSize(1);
  display.println("Waiting for message...");
  display.display();
}

void showMessage(String message) {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  char timeStr[20];
  sprintf(timeStr, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Message: ");
  display.println(message);
  display.setCursor(0, 20);
  display.print("Received at:");
  display.setCursor(0, 30);
  display.print(timeStr);
  display.display();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for time sync");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTime synchronized");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Received: ");
  Serial.println(message);
  showMessage(message);
}

void connectToMQTT() {
  secureClient.setInsecure();  

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");
      client.subscribe(mqtt_topic);
      client.publish(mqtt_topic, "ESP connected!");
      showConnectingAnimation();
      showWaiting();
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 sec");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.display();

  connectToWiFi();
  setupTime();
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
}
