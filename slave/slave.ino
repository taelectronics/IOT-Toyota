#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <EEPROM.h>
#include <VirtualWire.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* firmwareUrl = "https://github.com/your_username/your_repository/releases/latest/download/firmware.bin";

const int EEPROM_SIZE = 512; // Kích thước EEPROM, có thể điều chỉnh tùy nhu cầu

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Khởi tạo EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Ghi chuỗi vào EEPROM
  String dataToWrite = "Hello, ESP32!";
  writeStringToEEPROM(0, dataToWrite);

  // Đọc chuỗi từ EEPROM
  String readData = readStringFromEEPROM(0);
  Serial.println("Read from EEPROM: " + readData);

  // Khởi tạo VirtualWire cho RF433
  vw_set_rx_pin(12); // Pin 12 làm chân nhận RF
  vw_setup(2000); // Tốc độ baud 2000
  vw_rx_start(); // Bắt đầu nhận

  performOTA();
}

void loop() {
  String receivedData;
  int result = receiveUARTData(receivedData);
  if (result == 0) {
    Serial.println("Received Data: " + receivedData);
  } else {
    Serial.println("Timeout or error receiving data.");
  }

  uint32_t rfData;
  if (receiveRF433Data(rfData)) {
    Serial.print("Received RF Data: ");
    Serial.println(rfData, HEX);
  } else {
    Serial.println("Invalid RF Data or no data received.");
  }

  delay(1000); // Wait for a while before the next iteration
}

// Hàm ghi chuỗi vào EEPROM
void writeStringToEEPROM(int address, const String &data) {
  int len = data.length();
  EEPROM.writeByte(address, len);
  for (int i = 0; i < len; i++) {
    EEPROM.writeByte(address + 1 + i, data[i]);
  }
  EEPROM.commit();
}

// Hàm đọc chuỗi từ EEPROM
String readStringFromEEPROM(int address) {
  int len = EEPROM.readByte(address);
  char data[len + 1];
  for (int i = 0; i < len; i++) {
    data[i] = EEPROM.readByte(address + 1 + i);
  }
  data[len] = '\0'; // Kết thúc chuỗi
  return String(data);
}

// Hàm nhận dữ liệu từ UART
int receiveUARTData(String &data) {
  const unsigned long timeout = 3000; // 3 giây
  unsigned long startTime = millis();
  bool started = false;
  bool ended = false;
  data = "";

  while (millis() - startTime < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 0xFF && !started) {
        started = true;
        data = ""; // Bắt đầu một chuỗi mới
      } else if (c == 0xFE && started) {
        ended = true;
        break; // Kết thúc chuỗi
      } else if (started) {
        data += c; // Thêm ký tự vào chuỗi
      }
    }
  }

  if (started && ended) {
    return 0; // Thành công
  } else {
    return 1; // Timeout hoặc lỗi
  }
}

// Hàm nhận dữ liệu từ RF433
bool receiveRF433Data(uint32_t &data) {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;

  if (vw_get_message(buf, &buflen)) {
    if (buflen >= 7 && buf[0] == 0xFF && buf[buflen - 1] == 0xFE) {
      uint16_t crc = (buf[buflen - 3] << 8) | buf[buflen - 2];
      if (crc == calculateCRC(buf + 1, buflen - 4)) {
        data = (buf[1] << 16) | (buf[2] << 8) | buf[3];
        return true;
      }
    }
  }
  return false;
}

// Hàm tính toán CRC
uint16_t calculateCRC(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void performOTA() {
  HTTPClient http;
  http.begin(firmwareUrl);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);

    if (canBegin) {
      WiFiClient *client = http.getStreamPtr();
      size_t written = Update.writeStream(*client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      Serial.println("Not enough space to begin OTA");
    }
  } else {
    Serial.println("Cannot download firmware. HTTP error code: " + String(httpCode));
  }

  http.end();
}
