#include <RH_ASK.h>
#include <SPI.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <cert.h>

String FirmwareVer = "24.7.13.9.24";
#define URL_fw_Bin "https://raw.githubusercontent.com/taelectronics/IOT-Toyota/main/RF-Transmitter-Reciever-main/ESP32_RF/ImageFile/firmware.bin"
#define URL_fw_Version "https://raw.githubusercontent.com/taelectronics/IOT-Toyota/main/RF-Transmitter-Reciever-main/ESP32_RF/ImageFile/bin_version.txt"

#define NUMBER_TIMES 10
// Define the UART port and pins
HardwareSerial SerialModbus(2);
#define RS485_RXD2 16  // GPIO16, UART2 RXD
#define RS485_TXD2 17  // GPIO17, UART2 TXD
#define RS485_TX_ENABLE 18  // GPIO18, UART2 TX enable
// ModbusMaster object
ModbusMaster node;
// Define Modbus slave ID and register ranges
#define SLAVE_ID 1
#define REG_START_ON 0
#define REG_START_OFF 500

// Thông tin WiFi
#define WIFI_SSID "Duong Hanh"
#define WIFI_PASSWORD "hanh1992"

// Thông tin Firebase
#define FIREBASE_HOST "https://iot-toyota-1-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "SDE8HaiVyzKObClOF3FSoXBwh9K8qIjcvoC5KoMw"

// Khởi tạo Firebase
FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;
// Example arrays
uint16_t TimeOn[10] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
uint16_t TimeOff[10] = {500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400};
String StationName[20] = {"S00", "S01", "S02", "S03", "S04", "S05", "S06", "S07", "S08", "S09", "S10", "S11", "S12", "S13", "S14", "S15", "S16", "S17", "S18", "S19"};

byte Status[20] = {1, 2, 0, 0, 1, 0, 3, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1};
RH_ASK rf_driver(2000, 21, 22);

byte RetryModbus = 0;
byte RetryFirebase = 0;
String stationPath = "/Station/Status";
String convertStatusArrayToString(byte arr[20]);
String convertTimeArrayToString(uint16_t TimeOn[], uint16_t TimeOff[], int size);
bool readRegisters(uint16_t startReg, uint16_t count, uint16_t* values);
void firmwareUpdate();
int FirmwareVersionCheck(void);
void setup() {
  Serial.begin(115200);

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Cấu hình Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Kết nối Firebase
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) 
  {
    Serial.println("Connected to Firebase");
  }
  // Khởi tạo RF driver
  if (!rf_driver.init()) {
    Serial.println("init failed");
    while (1) delay(1000);
  }
  Serial.println("Transmitter: rf_driver initialised");

  // Initialize UART for Modbus communication
  SerialModbus.begin(9600, SERIAL_8N1, RS485_RXD2, RS485_TXD2);
  pinMode(RS485_TX_ENABLE, OUTPUT);
  // Initialize Modbus communication
  node.begin(SLAVE_ID, SerialModbus);
  Serial.println(FirmwareVer);
  pinMode(23, INPUT_PULLUP);
  if (digitalRead(23) == LOW)
  {
    if (FirmwareVersionCheck()) 
    {
      firmwareUpdate();
    }
  }
}

void loop()
{
  String data_Firebase;
  for (int i = 1; i < 20; i++)
  {
    RetryModbus = 0;
    while (RetryModbus <= 3)
    {
      if (true == readRegisters(REG_START_ON + i * 10, NUMBER_TIMES, TimeOn))
      {
        RetryModbus = 0;
        if (true == readRegisters(REG_START_OFF + i * 10, NUMBER_TIMES, TimeOff))
        {
          String data_time = convertTimeArrayToString(TimeOn, TimeOff, 10);
          Serial.println(data_time);
          delay(2000);
          data_Firebase += "@" + StationName[i] + ":" + data_time;
          RetryModbus = 0;
          break;
        }
        else
        {
          RetryModbus++;
        }
      }
      else
      {
        RetryModbus++;
      }
      if (RetryModbus > 3)
      {
        break;
      }
    }

    delay(100);
  }

  data_Firebase += "@";

  RetryFirebase = 0;
  if (RetryModbus == 0)
  {
    while (RetryFirebase < 3)
    {
      if (Firebase.ready())
      {
        // Serial.println(data_Firebase);
        if (Firebase.setString(firebaseData, stationPath.c_str(), data_Firebase.c_str()))
        {
          Serial.println("Data for " + stationPath + " written successfully");
          break;
        }
        else
        {
          RetryFirebase++;
          Serial.println("Failed to write data for " + stationPath);
          Serial.println(firebaseData.errorReason());
        }
        delay(1000);
      }
      else
      {
        Serial.println("Firebase is not ready");
        RetryFirebase++;
      }
    }
  }

  String data_transmit = convertStatusArrayToString(Status);
  Serial.println("Data to transmit: " + data_transmit);
  // Convert String to char array
  char msg[data_transmit.length() + 1];
  data_transmit.toCharArray(msg, data_transmit.length() + 1);

  rf_driver.send((uint8_t *)msg, strlen(msg));
  rf_driver.waitPacketSent();
  delay(1000);
}

String convertStatusArrayToString(byte arr[20]) {
  String result = "*"; // Thêm ký tự * vào đầu chuỗi

  // Duyệt qua từng phần tử của mảng
  for (int i = 0; i < 20; i++) {
    if (arr[i] == 0) {
      result += "0"; // Nếu phần tử bằng 0, thêm '0' vào chuỗi
    } else {
      result += "1"; // Nếu phần tử khác 0, thêm '1' vào chuỗi
    }
  }

  result += "@"; // Thêm ký tự @ vào cuối chuỗi (thay vì #)
  return result;
}

String convertTimeArrayToString(uint16_t TimeOn[], uint16_t TimeOff[], int size) {
  String result = "*";

  for (int i = 0; i < size; i++) {
    result += String(TimeOn[i]) + "-" + String(TimeOff[i]) + (i < size - 1 ? "*" : "");
  }

  result += "*";
  return result;
}

// Function to read Modbus registers into an array
bool readRegisters(uint16_t startReg, uint16_t count, uint16_t* values) 
{
  uint8_t result = node.readHoldingRegisters(startReg, count);
  // Serial.println(result);
  if (result == node.ku8MBSuccess) {
    for (int i = 0; i < count; i++) {
      values[i] = node.getResponseBuffer(i);
    }
    return true;
  } else {
    return false;
  }
}

void firmwareUpdate(void) 
{
  Serial.println("Start to Update");
  
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      Serial.println("Update Successfully");
      delay(1000); // Đợi 1 giây trước khi reset
      ESP.restart(); // Reset ESP32
      break;
  }
}




int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  } 
  return 0;  
}