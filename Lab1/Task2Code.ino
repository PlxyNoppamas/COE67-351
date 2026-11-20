#include <WiFi.h>
#include <MQTT.h>
#include <DHT.h>

// ============ EDIT THESE ============
const char* WIFI_SSID = "ploy";
const char* WIFI_PASS = "33333333";
const char* MQTT_HOST = "BROKER_IP_OR_HOSTNAME";
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "";          // เว้นว่างถ้า broker ไม่ต้อง login
const char* MQTT_PWD  = "";
const char* NODE_ID   = "p03";
// ====================================

#define DHTPIN  4
#define DHTTYPE DHT22
const uint32_t READ_INTERVAL_MS = 10000;
const int KEEPALIVE_S = 15;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient net;
MQTTClient mqtt(256);

char TOPIC_ENV[48], TOPIC_STATUS[48], CLIENT_ID[40];
uint32_t seqNo = 0;
uint32_t nextRead = 0;

void connectWifi() {
  Serial.printf("[WiFi] connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\n[WiFi] OK IP=%s\n", WiFi.localIP().toString().c_str());
}

void connectMqtt() {
  Serial.print("[MQTT] connecting ");
  while (true) {
    bool ok = (strlen(MQTT_USER) > 0) ? mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PWD)
                                      : mqtt.connect(CLIENT_ID);
    if (ok) break;
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" OK");
  mqtt.publish(TOPIC_STATUS, "online", true, 1);   // retained "online"
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  snprintf(TOPIC_ENV,    sizeof(TOPIC_ENV),    "coe67351/%s/env",    NODE_ID);
  snprintf(TOPIC_STATUS, sizeof(TOPIC_STATUS), "coe67351/%s/status", NODE_ID);
  snprintf(CLIENT_ID,    sizeof(CLIENT_ID),    "esp32s3-%s",         NODE_ID);

  dht.begin();
  connectWifi();

  mqtt.begin(MQTT_HOST, MQTT_PORT, net);
  mqtt.setOptions(KEEPALIVE_S, true, 2000);
  mqtt.setWill(TOPIC_STATUS, "offline", true, 1);  // Last Will: offline, retained, QoS 1
  connectMqtt();

  nextRead = millis() + 2000;
}

void loop() {
  mqtt.loop();
  if (WiFi.status() != WL_CONNECTED) connectWifi();   // แบบง่าย: ไม่ใช่ non-blocking
  if (!mqtt.connected()) connectMqtt();

  uint32_t now = millis();
  if ((int32_t)(now - nextRead) >= 0) {
    nextRead += READ_INTERVAL_MS;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("{\"error\":\"dht_read_failed\"}");
    } else {
      char buf[128];
      snprintf(buf, sizeof(buf),
               "{\"node\":\"%s\",\"seq\":%lu,\"temp_c\":%.1f,\"rh\":%.1f,\"rssi\":%d}",
               NODE_ID, (unsigned long)seqNo++, t, h, WiFi.RSSI());
      Serial.println(buf);
      Serial.printf("# payload %d bytes\n", (int)strlen(buf));
      if (!mqtt.publish(TOPIC_ENV, buf, false, 1)) {   // QoS 1
        Serial.println("# publish failed");
      }
    }
  }
  delay(10);
}
