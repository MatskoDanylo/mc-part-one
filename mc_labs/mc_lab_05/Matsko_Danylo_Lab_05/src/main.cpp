#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "Brawl Pass";
const char* password = "012345678900";

#define DHTPIN D2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

ESP8266WebServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void displayData(float temp, float hum) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Weather Station:");

  display.setTextSize(2);
  display.setCursor(0, 16);
  if (isnan(temp)) {
    display.println("T: Error");
  } else {
    display.print("T: ");
    display.print(temp);
    display.write(247);
    display.println("C");
  }

  display.setCursor(0, 40);
  if (isnan(hum)) {
    display.println("H: Error");
  } else {
    display.print("H: ");
    display.print(hum);
    display.println(" %");
  }

  display.display();
}

void handleRoot() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  displayData(t, h);

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    server.send(200, "text/html", "<h1>Failed to read from DHT sensor!</h1>");
    return;
  }

  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; }";
  html += ".container { background-color: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); padding: 20px; display: inline-block; }";
  html += "h1 { color: #333; }";
  html += "p { font-size: 1.2em; color: #555; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>Weather Station</h1>";
  html += "<p>Temperature: ";
  html += String(t);
  html += " &deg;C</p>";
  html += "<p>Humidity: ";
  html += String(h);
  html += " %</p>";
  html += "<p>Updated: " + String(millis() / 1000) + " seconds ago</p>";
  html += "<p><a href='/'>Refresh</a></p>";
  html += "</div>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(10);

  Wire.begin(D0, D1);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  display.display();
  delay(2000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Starting...");
  display.display();

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connecting to WiFi");
    display.setCursor(0,10);
    display.print("Attempt: ");
    display.println(++attempt);
    display.display();
    if (attempt > 20) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Connection");
      display.println("Failed");
      display.display();
      Serial.println("\nFailed to connect to WiFi!");
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("WiFi Connected!");
    display.setCursor(0,10);
    display.println(WiFi.localIP());
    display.display();
    delay(3000);
  }

  dht.begin();

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  static unsigned long lastDisplayUpdate = 0;
  const long displayUpdateInterval = 5000;

  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    displayData(t, h);
    lastDisplayUpdate = millis();
  }
}