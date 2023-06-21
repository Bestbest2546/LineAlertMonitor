#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>

const char* ssid = "TTTA@DOM";
const char* password = "Ttta@2021";
const char* lineToken = "MRYeObqxXQSBqX7oblrg0iczqjw7LhaBVkUzIYBqAqR";

ESP8266WebServer server(80);
WiFiClient esp32Client;

String receivedwatt;
String receivedkWh;

float previouskWh = 0.0;      // ค่า kWh ของวันก่อนหน้า
float dailyExpense = 0.0;

void handleESP32Data() {
  receivedwatt = server.arg("plain");

  Serial.print("Received data from ESP32: ");
  Serial.println(receivedwatt + "watt");

  // ประมวลผลข้อมูลที่ได้รับจาก ESP32 และทำการตอบสนอง
  server.send(200, "text/plain", "Data received by ESP01");

  // ส่งข้อมูลไปยัง Line Notify
  float wattValue = receivedwatt.toFloat();
  if (wattValue > 5000.0) {
    char message[100];
    snprintf(message, sizeof(message), "ระวังค่าไฟเพิ่มนะจ้ะ Watt %.2f", wattValue);
    sendLineNotifyWithSticker(message, lineToken, 11538, 51626496);
  }
  
  delay(600000);
}

void AlertTime() {
  receivedkWh = server.arg("plain");
  Serial.print("Received data from ESP32: ");
  Serial.println(receivedkWh + "kWh");

  // ประมวลผลข้อมูลที่ได้รับจาก ESP32 และทำการตอบสนอง
  server.send(200, "text/plain", "Data received by ESP01");

  float kWhValue = receivedkWh.toFloat();

  // คำนวณรายจ่ายจากผลต่าง kWh ระหว่างวัน
  float dailykWhDifference = kWhValue - previouskWh;
  float dailyExpense = dailykWhDifference * 10.0;  // ราคาต่อหน่วย kWh = 10.0

  // อัปเดตค่า kWh และวันก่อนหน้า
  previouskWh = kWhValue;
  
  // อัปเดตรายจ่ายรวมของแต่ละวัน
  dailyExpense += dailyExpense;

  // ส่งข้อมูลไปยัง Line Notify
  char message[100];
  snprintf(message, sizeof(message), "ระวังค่าไฟเพิ่มนะจ้ะ kWh %.2f หน่วย สรุปค่าไฟของวันนี้คือ %.2f บาท", kWhValue, dailyExpense);
  sendLineNotifyWithSticker(message, lineToken, 11538, 51626496);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  setTime(23, 59, 0, 18, 6, 2023); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/load", handleESP32Data);

  server.begin();
  Serial.println("Server started");

  // Cron.begin();

  // Cron.add("0 0 18 * * *", sendLineNotifyToLine);

}

void sendLineNotifyWithSticker(const char* message, const char* token, int packageId, int stickerId) {
  // ฟังก์ชันส่งข้อมูลไปยัง Line Notify พร้อมสติกเกอร์
  // ...
}

void myFunction() {
  // ฟังก์ชันที่ต้องการทำงานทุก 18:00 นาฬิกาไทย
  AlertTime();
}

void loop() {
  if (second() == 0) {
    // ตรวจสอบว่าเป็นเวลา 18:00 นาฬิกาไทยหรือไม่
    if (hour() == 18 && minute() == 0) {
      // ทำงานที่ต้องการทุก 18:00 นาฬิกาไทย
      // เรียกใช้ฟังก์ชันที่ต้องการทำงาน
      myFunction();
    }
  }

  server.handleClient();

  if (!esp32Client.connected()) {
    esp32Client = WiFiClient();
    const char* esp32IP = "192.168.1.242";

    if (esp32Client.connect(esp32IP, 80)) {
      Serial.println("Connected to ESP32");
      delay(1000);
    }
  }
}
