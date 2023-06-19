#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>


WiFiMulti wifiMulti;
ModbusMaster node;
#define modbusaddr 1

AsyncWebServer server(80);

const char* ssid = "TTTA@DOM"; // ชื่อ WiFi ของคุณ
const char* password = "Ttta@2021"; // รหัสผ่าน WiFi ของคุณ
const char* ntpServer = "th.pool.ntp.org"; // NTP server สำหรับไทย
const int timeZone = 7; // โซนเวลาของไทย (เขตเวลาอาเซียน)


#define WIFI_SSID "TTTA@DOM"
#define WIFI_PASSWORD "Ttta@2021"
#define INFLUXDB_URL "http://202.151.182.220:8086"
#define INFLUXDB_TOKEN "Pd9gfPjLZKvUTpUyksUFGCNls7PQ1Gtua0JjgxZI_4lEb3m6JqmnGAbQ_XRSANYIyDaRV_Af5WzuWVC7tqw2lg=="
#define INFLUXDB_ORG "the tiger team academy"
#define INFLUXDB_BUCKET "ttta"

const char* lineToken = "rtzy53J8WDXkJiPQaRB4iCVXKm662QddoV2fWejryzf"; // Your Line Notify Token

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("ESP32GRID");

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, timeZone * 3600);

void preTransmission()
{

}

void postTransmission()
{
  delay(2);
}

float reform_uint16_2_float32(uint16_t u1, uint16_t u2)
{
  uint32_t num = ((uint32_t)u1 & 0xFFFF) << 16 | ((uint32_t)u2 & 0xFFFF);
  float numf;
  memcpy(&numf, &num, 4);
  return numf;
}

float getRTU(uint16_t m_startAddress)
{
  uint8_t m_length = 2;
  uint16_t result;
  float x;

  node.clearResponseBuffer();

  result = node.readInputRegisters(m_startAddress, m_length);
  if (result == node.ku8MBSuccess)
  {
    return reform_uint16_2_float32(node.getResponseBuffer(0), node.getResponseBuffer(1));
  }
}

void sendLineNotifyWithSticker(const char* message, const char* token, int packageId, int stickerId)
{
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

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  }
  else
  {
    Serial.print("Error sending data. HTTP error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}



void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 15, 4);
  WiFi.begin(ssid, password);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Connecting to WiFi");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  timeClient.begin();
  delay(200);

  Serial.println("Connected to WiFi!");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);
  delay(200);
  server.begin();
    delay(200);
  Serial.println("HTTP server started");

  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    delay(200);
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
      delay(200);
  }

  node.begin(modbusaddr, Serial2);
    delay(200);
  node.preTransmission(preTransmission);
    delay(200);
  node.postTransmission(postTransmission);
   delay(200);
}

unsigned long previousAlertMillis = 0;
const unsigned long alertInterval = 3600000; // 1 ชั่วโมง

void loop() {
  timeClient.update();
   delay(200);
  setTime(timeClient.getEpochTime());
   delay(200);

  float Vol = getRTU(0x0000);
   delay(200);
  float Wat = getRTU(0x000C);
   delay(200);
  float Cur = getRTU(0x0006);
   delay(200);
  float Fre = getRTU(0x0046);
   delay(200);
  float TotalWh = getRTU(0x0156);
   delay(200);
  float PF = getRTU(0x001E);
   delay(200);

  sensor.clearFields();
  sensor.addField("Volt", Vol);
  delay(200);
  sensor.addField("Watts", Wat);
    delay(200);
  sensor.addField("Amp", Cur);
    delay(200);
  sensor.addField("kWh", TotalWh);
    delay(200);
  sensor.addField("HZ", Fre);
    delay(200);
  sensor.addField("PF", PF);

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("WiFi connection lost");
  }

  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
 

  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();

  if (Wat > 5000.0) {
    char watString[100];
    dtostrf(Wat, 1, 2, watString);
    String message = "Watt เกินกว่าปกติ " + String(watString) + " Watt";
    sendLineNotifyWithSticker(message.c_str(), lineToken, 11538, 51626496);
  }

  // ตรวจสอบเงื่อนไขและเรียกใช้ฟังก์ชัน Alert() เพียงครั้งเดียวเมื่อตรงตามเวลาที่กำหนด
  if (currentHour == 7 && currentMinute >= 1 && currentMinute <= 59 && currentSecond >= 0 && currentSecond <= 59) {
    if (previousAlertMillis == 0 || millis() - previousAlertMillis >= alertInterval) {
      previousAlertMillis = millis();
      Alert();
    }
  }

  // ตรวจสอบและส่งข้อความ Line Notify เมื่อ Wat เกินค่าที่กำหนด
  if (Wat > 5000.0) {
    char watString[100];
    dtostrf(Wat, 1, 2, watString);
    String message = "Watt เกินกว่าปกติ " + String(watString) + " Watt";
    sendLineNotifyWithSticker(message.c_str(), lineToken, 11538, 51626496);
  }
   delay(5000);
}

void Alert() {
  float TotalWh = getRTU(0x0156);
   delay(200);
  static float previouskWh = 0.0;
  // ดึงค่า kWh จากอุปกรณ์

  // คำนวณรายจ่ายจากผลต่าง kWh ระหว่างวัน
  float dailykWhDifference = TotalWh - previouskWh;
  float dailyExpense = dailykWhDifference * 4.7;

  // อัปเดตค่า kWh และวันก่อนหน้า
  previouskWh = TotalWh;

  // สร้างข้อความที่จะส่งใน Line Notify
  char kWh[100];
  char cost[100];
  dtostrf(dailykWhDifference, 1, 2, kWh);
  dtostrf(dailyExpense, 1, 2, cost);
  String message = "ระวังค่าไฟเพิ่มนะจ้ะ " + String(kWh) + " หน่วย สรุปค่าไฟของเมื่อวานคือ " + String(cost) + " บาท";
  sendLineNotifyWithSticker(message.c_str(), lineToken, 11538, 51626496);
}
