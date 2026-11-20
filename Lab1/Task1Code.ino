#include <DHT.h>

const char* NODE_ID = "p03";          
#define DHTPIN  4
#define DHTTYPE DHT22
const uint32_t READ_INTERVAL_MS = 10000;

DHT dht(DHTPIN, DHTTYPE);
uint32_t seqNo = 0;
uint32_t nextRead = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();
  nextRead = millis() + 2000;         // DHT22 ต้องรอ ~2 วินาทีหลังเปิดเครื่อง
}

void loop() {
  uint32_t now = millis();
  if ((int32_t)(now - nextRead) >= 0) {
    nextRead += READ_INTERVAL_MS;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("{\"error\":\"dht_read_failed\"}");
      return;
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"node\":\"%s\",\"seq\":%lu,\"temp_c\":%.1f,\"rh\":%.1f}",
             NODE_ID, (unsigned long)seqNo++, t, h);
    Serial.println(buf);
  }
}
