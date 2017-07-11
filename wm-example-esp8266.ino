/**
 * Wireless Monitor com NodeMCU ESP8266-12E
 */
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Wi-Fi configuration
const char* ssid = "";
const char* password = "";

// Wireless Monitor configuration
const String host = "";
const int port = 80;
const String api_key = "";
const String monitor_key = "";

// These will be setup during execution
String token = "";
int remaining = -1;

HTTPClient http;
DynamicJsonBuffer jsonBuffer;

bool auth() {
  http.begin(host, port, "/api/authenticate");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String payload = "api_key=" + api_key + "&monitor_key=" + monitor_key;
  Serial.print("payload: ");
  Serial.println(payload);

  const char* headerNames[] = { "X-RateLimit-Remaining" };
  http.collectHeaders(headerNames, sizeof(headerNames)/sizeof(headerNames[0]));

  int httpCode = http.POST(payload);
  String response = http.getString();

  if (httpCode == HTTP_CODE_OK) {
    JsonObject& root = jsonBuffer.parseObject(response);
    token = root["token"].as<String>();
    Serial.println("Token received. Ready to send data.");
    Serial.println(token);

    if (http.hasHeader("X-RateLimit-Remaining")) {
      remaining = http.header("X-RateLimit-Remaining").toInt();
    } else {
      Serial.println("Header: X-RateLimit-Remaining not found");
    }
  } else {
    token = "";
  }

  Serial.println(httpCode);
  Serial.println(response);

  http.end();
  return httpCode == HTTP_CODE_OK;
}

bool send() {
  http.begin(host, port, "/api/send");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);
  String payload = "{\"data\":{\"value\":28.00}}";
  int httpCode = http.POST(payload);
  String response = http.getString();

  Serial.println(httpCode);
  Serial.println(response);
  Serial.print(remaining);
  Serial.println(" requests remaining.");
  remaining--;

  http.end();
  return httpCode == HTTP_CODE_OK;
}

bool refreshToken() {
  http.begin(host, port, "/api/refresh-token");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);
  int httpCode = http.GET();
  String response = http.getString();

  if (httpCode == HTTP_CODE_OK) {
    JsonObject& root = jsonBuffer.parseObject(response);
    token = root["token"].as<String>();
    Serial.println("Token refreshed. Ready to send data again.");
    Serial.println(token);
  } else {
    token = "";
  }

  Serial.println(httpCode);
  Serial.println(response);

  http.end();
  return httpCode == HTTP_CODE_OK;
}

void setup() {

  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  Serial.println("Waiting for connection");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected.");
}

void loop() {

  if (WiFi.status()== WL_CONNECTED) {
    Serial.print("connecting to ");
    Serial.println(host);

    if (token == "") {
      auth();
    }

    bool ok = send();
    if (!ok) {
      token = "";
    }

    if (remaining < 10 ) {
      refreshToken();
    }

  } else {
    Serial.println("Error in WiFi connection");
  }
  delay(10000);
}
