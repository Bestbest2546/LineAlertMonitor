// #include <WiFi.h>
// #include <HTTPClient.h>


#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "TTTA@DOM";
const char* password = "Ttta@2021";
const char* lineToken = "MRYeObqxXQSBqX7oblrg0iczqjw7LhaBVkUzIYBqAqR";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi!");
}

void sendLineNotifyWithSticker(const char* message, const char* token, int packageId, int stickerId) {
  WiFiClientSecure client;
  HTTPClient http;
  
  client.setInsecure();
  
  http.begin(client, "https://notify-api.line.me/api/notify");
  http.addHeader("Authorization", String("Bearer ") + token);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String data = "message=" + String(message);
  data += "&stickerPackageId=" + String(packageId);
  data += "&stickerId=" + String(stickerId);
  
  int httpResponseCode = http.POST(data);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error sending data. HTTP error code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void loop() {
  // Example usage: Send a message with sticker to Line Notify
  sendLineNotifyWithSticker("Hello from ESP8266!", lineToken, 11538, 51626496);

  delay(5000);  // Wait for 5 seconds before sending the next message
}

