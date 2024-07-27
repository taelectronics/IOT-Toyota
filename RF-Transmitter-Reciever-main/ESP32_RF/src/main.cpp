#include <RH_ASK.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <cert.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <LiquidCrystal.h>
#include "time.h"
#include <SPI.h>
#include "esp_task_wdt.h"
// Thông tin máy chủ NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 (Giờ Việt Nam)
const int   daylightOffset_sec = 0;

// Thời gian chờ của watchdog (giây)
#define WATCHDOG_TIMEOUT_S 300

// Khởi tạo đối tượng WiFiUDP để giao tiếp với NTP
WiFiUDP ntpUDP;

// Khởi tạo đối tượng NTPClient với múi giờ Việt Nam (GMT+7) và cập nhật mỗi 1 giây
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7*3600, 10000);  // GMT+7, cập nhật mỗi 1 giây

//Khởi tạo với các chân LCD
LiquidCrystal lcd(14, 27, 26, 25, 33, 32);

Preferences preferences;

#define URL_fw_Bin "https://raw.githubusercontent.com/taelectronics/IOT-Toyota/main/RF-Transmitter-Reciever-main/ESP32_RF/.pio/build/esp32doit-devkit-v1/firmware.bin"
#define URL_fw_Version "https://raw.githubusercontent.com/taelectronics/IOT-Toyota/main/RF-Transmitter-Reciever-main/ESP32_RF/ImageFile/bin_version.txt"

#define NUMBER_TIMES 10
// Define the UART port and pins
HardwareSerial SerialModbus(2);
#define RS485_RXD2 16  // GPIO16, UART2 RXD
#define RS485_TXD2 17  // GPIO17, UART2 TXD
#define RS485_TX_ENABLE 18  // GPIO18, UART2 TX enable
#define SELECT_FIREBASE_PRIMARY 34
#define SELECT_FIREBASE_BACKUP 35
// ModbusMaster object
ModbusMaster node;
// Define Modbus slave ID and register ranges
#define SLAVE_ID 1
#define REG_START_ON 0
#define REG_START_OFF 500

// Thông tin Firebase
#define FIREBASE_HOST_PRIMARY "https://iot-toyota-1-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH_PRIMARY "SDE8HaiVyzKObClOF3FSoXBwh9K8qIjcvoC5KoMw"

#define FIREBASE_HOST_BACKUP "https://iot-toyota-backup-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH_BACKUP "1HCXadicLQ5GF0afUchHYwglSdV3y5LaBx3q5dGo"

#define RETRY_MODBUS 1
#define RETRY_FIREBASE 1
#define E_OK 0
#define E_NOT_OK 1
// Khởi tạo Firebase
FirebaseData firebaseDataPrimary;
FirebaseAuth firebaseAuthPrimary;
FirebaseConfig firebaseConfigPrimary;

// Khởi tạo Firebase
FirebaseData firebaseDataBackup;
FirebaseAuth firebaseAuthBackup;
FirebaseConfig firebaseConfigBackup;

String wifi_ssid = "CKA Automation";
String wifi_password = "cka12345";
byte WifiStatus;
int TimeSendRF;
// Example arrays
byte StationValue;
byte Firebase_Backup_Status = E_NOT_OK;
byte Firebase_Primary_Status = E_NOT_OK;
byte SetTimeHMI = E_NOT_OK;
int hourGet = 0, minGet = 0 , secGet = 0;
uint16_t HMITime[2];
uint16_t timeValue;
uint16_t TimeGet[40] = {};
uint16_t TimeProcessed[20][20];
uint16_t TimeCompare[20][20];
String StationName[20] = {"/S00", "/S01", "/S02", "/S03", "/S04", "/S05", "/S06", "/S07", "/S08", "/S09", "/S10", "/S11", "/S12", "/S13", "/S14", "/S15", "/S16", "/S17", "/S18", "/S19"};
String FirmwareVer = "24.7.13.10.38";
byte Status[20] = {0x0F, 0x0F, 0x0F, 0x0F,0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F};
byte Firebase_Primary_Set[20] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
byte Firebase_Backup_Set[20] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
RH_ASK rf_driver(2000, 21, 19);
String stationPath = "/Station/Status";
String ROMData = "";
String data_Firebase;
String convertStatusArrayToString(byte arr[20], int inputSize);
void reduceArray(const uint16_t inputArray[], uint16_t outputArray[],  int inputSize);
bool readRegisters(uint16_t startReg, uint16_t count, uint16_t* values);
void firmwareUpdate();
int FirmwareVersionCheck(void);
bool checkTimeRange(uint16_t timeArray[], uint16_t arraySize, uint16_t timenow);
void writeStringToROM(String myString);
String readStringFromROM();
String receiveStringFromSerial();
void SettingbySoftware();
boolean ConnectToWifi(byte retry);
String getStringBetween(String input, String delimiter, int index);
int CalculateTimeValue();
void printLocalTime();
bool getFirebaseData(FirebaseData &firebaseData, String &a, const String &path);
bool compareStrings(String str1, String str2);
void preTransmission();
void postTransmission();
String arrayToString(uint16_t arr[], int size);
void sendRF();
void screen1();
bool ReadTimeFromHMI();
void ReadFromRom();
void setup() {
  Serial.begin(115200);
  delay(10);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.clearWriteError();
  lcd.setCursor(0, 0);
  lcd.print("V:");lcd.print(FirmwareVer);
  lcd.setCursor(0, 1);
  lcd.print("Wifi: "); lcd.print(wifi_ssid);
  delay(1000);
  // Đọc chuỗi từ ROM và hiển thị lên Serial
  ReadFromRom();
  // Cấu hình Firebase
  firebaseConfigPrimary.host = FIREBASE_HOST_PRIMARY;
  firebaseConfigPrimary.signer.tokens.legacy_token = FIREBASE_AUTH_PRIMARY;
  firebaseConfigPrimary.timeout.serverResponse = 5 * 1000;

  // // Cấu hình Firebase Backup
  firebaseConfigBackup.host = FIREBASE_HOST_BACKUP;
  firebaseConfigBackup.signer.tokens.legacy_token = FIREBASE_AUTH_BACKUP;
  firebaseConfigBackup.timeout.serverResponse = 5 * 1000;

  // Kết nối Firebase
  Firebase.begin(&firebaseConfigBackup, &firebaseAuthBackup);
  Firebase.reconnectWiFi(true);

  // Kết nối Firebase
  Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
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
    // Thiết lập các hàm tiền và hậu truyền thông
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  Serial.println(FirmwareVer);
    // Cấu hình máy chủ NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (FirmwareVersionCheck()) 
  {
    firmwareUpdate();
  }
  pinMode(SELECT_FIREBASE_PRIMARY, INPUT_PULLUP);
  pinMode(SELECT_FIREBASE_BACKUP,INPUT_PULLUP);
  // Khởi tạo watchdog
  esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);

}
int count;
void loop()
{
  if((false == ReadTimeFromHMI())||(SetTimeHMI == E_NOT_OK))
  {
    printLocalTime();
  }
  esp_task_wdt_reset();
  SettingbySoftware();
  screen1();
  for(int RowIndex = 0; RowIndex < 20; RowIndex++)
  {
    for(int ColIndex = 0; ColIndex <20; ColIndex++)
    {
      TimeCompare[RowIndex][ColIndex] = TimeProcessed[RowIndex][ColIndex];
    }
  }
  esp_task_wdt_reset();
  for (uint16_t i = 1; i < 20; i++)
  {
    if (true == readRegisters(1000+i*40, 40, TimeGet))
    {
      esp_task_wdt_reset();
      reduceArray(TimeGet, TimeProcessed[i], 40);
      delay(10);
    }
  }
  for(int RowIndex = 1; RowIndex < 20; RowIndex++)
  {
    for(int ColIndex = 0; ColIndex <20; ColIndex++)
    {
      if(TimeCompare[RowIndex][ColIndex] != TimeProcessed[RowIndex][ColIndex])
      {
        Firebase_Primary_Set[RowIndex] = E_NOT_OK;
        Firebase_Backup_Set[RowIndex] = E_NOT_OK;
        break;
      }
    }
  }
for( int Num = 1; Num < 20; Num++)
{
  if((Firebase_Primary_Set[Num] == E_NOT_OK))
  {
      Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
      break;
  }
}

for( int Num = 1; Num < 20; Num++)
{
  if((Firebase_Primary_Set[Num] == E_NOT_OK))
  { 
    esp_task_wdt_reset();
    // Lựa chọn sử dụng DataBase Primary
    if (Firebase.ready())
    {

      data_Firebase = arrayToString(TimeProcessed[Num], 20);
      Serial.println(data_Firebase);
      if (Firebase.setString(firebaseDataPrimary, stationPath.c_str() + StationName[Num], data_Firebase.c_str()))
      {
        Serial.print("Write Successfully to The Primary Firebase: ");Serial.println(Num);
        Firebase_Primary_Set[Num] = E_OK;
      }
      else
      {
        Firebase_Primary_Set[Num] = E_NOT_OK;
        Serial.println("Failed to Write Primary Firebase: "); Serial.println(Num);
        Serial.println(firebaseDataPrimary.errorReason());
      }
      TimeSendRF = 5;
      WifiStatus = E_OK;
    }
    else
    {
      Firebase_Primary_Set[Num] = E_NOT_OK;
      Serial.println("Firebase is not ready");
      if (WiFi.status() != WL_CONNECTED)
      {
        WifiStatus = E_NOT_OK;
        Serial.println("Wifi isn't contected");
        WiFi.reconnect();
      }
    }
  delay(1000);
  }
}
for( int Num = 1; Num < 20; Num++)
{
  if((Firebase_Backup_Set[Num] == E_NOT_OK))
  Firebase.begin(&firebaseConfigBackup, &firebaseAuthBackup);
  break;
}
  // Lựa chọn sử dụng DataBase Backup
for( int Num = 1; Num < 20; Num++)
{
  if((Firebase_Backup_Set[Num] == E_NOT_OK))
  {
    esp_task_wdt_reset();
    if (Firebase.ready())
    {
      data_Firebase = arrayToString(TimeProcessed[Num], 20);
      Serial.println(data_Firebase);
      if (Firebase.setString(firebaseDataBackup, stationPath.c_str() + StationName[Num] , data_Firebase.c_str()))
      {
        Serial.print("Write Successfully to The Backup Firebase: ");
        Serial.println(Num);
        Firebase_Backup_Set[Num] = E_OK;
      }
      else
      {
        Firebase_Backup_Set[Num] = E_NOT_OK;
        Serial.print("Failed to Write Backup Firebase: "); Serial.println(Num);
        Serial.println(firebaseDataBackup.errorReason());
      }
      TimeSendRF = 5;
      WifiStatus = E_OK;
    }
    else
    {
      Firebase_Backup_Set[Num] = E_NOT_OK;
      Serial.println("Firebase is not ready");
      WifiStatus = E_NOT_OK;
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Wifi isn't contected");
        WiFi.reconnect();
      }
    }
  delay(1000);
  } 
}
if(TimeSendRF > 0)
{
  sendRF();
  TimeSendRF--;
  delay(1000);
}

}

String convertStatusArrayToString(byte arr[], int inputSize) {
  String result = "";
  for (int i = 0; i < inputSize; i++) {
    if (arr[i] < 10) 
    {
      result += "0"; // Đảm bảo tất cả các số đều có hai chữ số
    }
    result += String(arr[i], DEC); // Chuyển từng phần tử byte thành chuỗi số thập phân
  }
  return result;
}


// Function to read Modbus registers into an array
bool readRegisters(uint16_t startReg, uint16_t count, uint16_t* values) 
{
  uint8_t result = node.readHoldingRegisters(startReg, count);
  if (result == node.ku8MBSuccess) {
    for (int i = 0; i < count; i++) {
      values[i] = node.getResponseBuffer(i);
    }
    return true;
  } else {
    return false;
  }
}
bool ReadTimeFromHMI()
{
  bool result;
  result = readRegisters(9017, 3, HMITime);
  if(result == true)
  {
    timeValue = HMITime[2]*60 + HMITime[1];
    Serial.print("Time HMI: "); Serial.print(HMITime[2]); Serial.print(":"); Serial.println(HMITime[1]);
    Serial.println(timeValue);
  }
  hourGet = (int)HMITime[2];
  minGet = (int)HMITime[1];
  secGet = (int)HMITime[0];
  return result;
}

void firmwareUpdate(void) 
{
  Serial.println("Start to Update");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Start to Update");
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Update fail");
      delay(2000);
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("No Update");
      delay(2000);
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Update");
      lcd.setCursor(0,1);
      lcd.print("Successfully");
      delay(2000);
      Serial.println("HTTP_UPDATE_OK");
      Serial.println("Update Successfully");
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

void reduceArray(const uint16_t inputArray[], uint16_t outputArray[],  int inputSize) {
    int outputSize = inputSize / 2;  // Kích thước của mảng đầu ra bằng một nửa mảng đầu vào
    for (int i = 0; i < outputSize; i++) 
    {
        outputArray[i] = inputArray[2 * i] * 60 + inputArray[2 * i + 1];
    }
}

bool checkTimeRange(uint16_t timeArray[], uint16_t arraySize, uint16_t timenow) {
    for (int i = 0; i < arraySize; i += 2) {
        uint16_t startTime = timeArray[i];
        uint16_t endTime = timeArray[i + 1];
        if (timenow >= startTime && timenow < endTime) {
            return true;
        }
    }
    return false;
}

void writeStringToROM(String myString) {
  // Khởi động Preferences với tên không gian lưu trữ (namespace)
  preferences.begin("my-app", false);

  // Ghi chuỗi vào ROM
  preferences.putString("myString", myString);

  // Kết thúc Preferences
  preferences.end();

  Serial.println("String đã được ghi vào ROM");
}

String readStringFromROM() {
  // Khởi động Preferences với tên không gian lưu trữ (namespace)
  preferences.begin("my-app", true);

  // Đọc chuỗi từ ROM
  String myString = preferences.getString("myString", "");

  // Hiển thị chuỗi lên Serial
  Serial.print("Chuỗi đọc được từ ROM: ");
  Serial.println(myString);

  // Kết thúc Preferences
  preferences.end();
  return myString;
}

int findSubstringPosition(String str1, String str2) {
  int position = str2.indexOf(str1); // Tìm vị trí của str1 trong str2

  return position;
}

String cutString(String input_str, int start_index, int end_index) {
  if (start_index >= end_index) {
    return ""; // Trả về chuỗi rỗng nếu start_index >= end_index
  } else if (start_index < 0) {
    start_index = 0; // Đảm bảo start_index không nhỏ hơn 0
  } else if (end_index > input_str.length()) {
    end_index = input_str.length(); // Đảm bảo end_index không lớn hơn độ dài của chuỗi
  }

  return input_str.substring(start_index, end_index);
}

String receiveStringFromSerial() 
{
  static bool receiving = false;
  static String receivedString = "";
  int startPosisiton = -1;
  int endPosisiton = -1;
  while (Serial.available() > 0) 
  {
    receivedString += (char)Serial.read();
  }
  startPosisiton = findSubstringPosition("$$$",receivedString);
  endPosisiton = findSubstringPosition("%%%",receivedString);
  if((startPosisiton >= 0) && (endPosisiton >=3)) 
  {
    return cutString(receivedString,startPosisiton+3 ,endPosisiton);
    Serial.print("Processed:");
    Serial.println(receivedString);
  }
  else
  {
    receivedString = "";
  }
  return receivedString;
}

bool compareStrings(String str1, String str2) 
{
  if (str1.length() != str2.length()) {
    return false; // Nếu độ dài khác nhau thì chuỗi khác nhau
  }
  for (int i = 0; i < str1.length(); ++i) {
    if (tolower(str1[i]) != tolower(str2[i])) {
      return false; // Nếu có ký tự khác nhau thì chuỗi khác nhau
    }
  }
  return true; // Nếu không có ký tự nào khác nhau, hai chuỗi giống nhau
}

void SettingbySoftware()
{
    if (Serial.available() > 0) 
    {
      String receivedString = receiveStringFromSerial();
      if (receivedString.length() > 0) 
      {
        // Ghi chuỗi vào ROM
        writeStringToROM(receivedString);
        delay(1000);
        // Đọc chuỗi từ ROM và hiển thị lên Serial
        if(compareStrings(receivedString,readStringFromROM()))
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Finished to Set");
          lcd.setCursor(0,1);
          lcd.print("Please reset");
          delay(5000);
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("failed to Set");
          delay(2000);
        }
      }
      else
      {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Data is Invalid");
          delay(2000);
      }
  }
}

// Hàm để lấy chuỗi nằm giữa 2 dấu *
String getStringBetween(String input, String delimiter, int index) {
  int delimiterCount = 0;
  int fromIndex = 0;
  
  while (delimiterCount < index) {
    fromIndex = input.indexOf(delimiter, fromIndex);
    if (fromIndex == -1) {
      return ""; // Không tìm thấy
    }
    delimiterCount++;
    fromIndex++; // Đi đến vị trí sau dấu *
  }
  
  int toIndex = input.indexOf(delimiter, fromIndex);
  if (toIndex == -1) {
    toIndex = input.length();
  }
  
  return input.substring(fromIndex, toIndex);
}
boolean ConnectToWifi(byte retry)
{
    // Kết nối WiFi
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.print("Connecting to Wi-Fi: ");
    Serial.print(wifi_ssid);
    Serial.print("-");
    Serial.println(wifi_password);
    for(int i=0; i<retry;i++)
    {
      SettingbySoftware();
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("Connecting is successfully");
        return true;
      }
      Serial.print(".");
      delay(300);
    }
    Serial.println("Fail to contect to Wifi");
    return false;
}

void printLocalTime() 
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  hourGet = timeinfo.tm_hour;
  minGet = timeinfo.tm_min;
  secGet = timeinfo.tm_sec;
  timeValue = timeinfo.tm_hour*60 + timeinfo.tm_min;
  if(SetTimeHMI == E_NOT_OK)
  {
    uint16_t startAddress = 9017;
    // Ghi dữ liệu vào buffer truyền
    node.setTransmitBuffer(0, secGet); // Giá trị 0x1234 vào thanh ghi đầu tiên
    node.setTransmitBuffer(1, minGet); // Giá trị 0x5678 vào thanh ghi thứ hai
    node.setTransmitBuffer(2, hourGet); // Giá trị 0x9ABC vào thanh ghi thứ ba
    uint8_t result = node.writeMultipleRegisters(startAddress, 3);
      // Kiểm tra kết quả
    if (result == node.ku8MBSuccess) {
      Serial.println("Ghi thành công thời gian vào HMI!");
      SetTimeHMI = E_OK;
    } else {
      Serial.print("Lỗi: ");
      Serial.println(result, HEX);
    }
  }
  

}

void preTransmission() 
{
  digitalWrite(RS485_TX_ENABLE, HIGH);
}

void postTransmission() 
{
  digitalWrite(RS485_TX_ENABLE, LOW);
}

bool getFirebaseData(FirebaseData &firebaseData, String &a, const String &path) {
  if (Firebase.getString(firebaseData, path)) {
    if (firebaseData.dataType() == "string") {
      a = firebaseData.stringData();
      return true;
    }
  }
  Serial.println(firebaseData.errorReason());
  return false;
}

// Hàm chuyển đổi mảng thành chuỗi
String arrayToString(uint16_t arr[], int size) 
{
  String result = "*";
  for (int i = 0; i < size; i++) 
  {
    result += String(arr[i]);
    if (i < size - 1) 
    {
      result += "*";
    }
  }
  result += "*";
  return result;
}

void sendRF()
{
  String data_transmit = convertStatusArrayToString(Status, 20);
  Serial.println("Data to transmit: " + data_transmit);

  // Convert String to char array
  char msg[50];
  data_transmit.toCharArray(msg, data_transmit.length() + 1);
  rf_driver.send((uint8_t *)msg, strlen(msg));
  rf_driver.waitPacketSent();
  count++;
  if(count > 9) count = 1;
}

void screen1()
{
  lcd.setCursor(0,0);
  lcd.print("RF:");
  lcd.print(TimeSendRF);
  lcd.print(" ");
  lcd.print(hourGet);lcd.print(":");
  lcd.print(minGet);lcd.print(":");
  lcd.print(secGet);lcd.print("     ");
  lcd.setCursor(0,1);
  lcd.print("W:");
  if(WifiStatus == E_OK)
  {
    lcd.print("1 ");
  }
  else
  {
    lcd.print("0 ");
  }
  Firebase_Primary_Status = E_OK;
  Firebase_Backup_Status = E_OK;
  for(int i = 1; i<20; i++)
  {
    if(Firebase_Primary_Set[i] == E_NOT_OK)
    {
      Firebase_Primary_Status = E_NOT_OK;
    }

    if(Firebase_Backup_Set[i] == E_NOT_OK)
    {
      Firebase_Backup_Status = E_NOT_OK;
    }
  }
  lcd.print("P:");
  if(Firebase_Primary_Status == E_OK)
  {
    lcd.print("1 ");
  }
  else
  {
    lcd.print("0 ");
  }
    
  lcd.print("B:");
  if(Firebase_Backup_Status == E_OK)
  {
    lcd.print("1 ");
  }
  else
  {
    lcd.print("0 ");
  }
  lcd.print("     ");
}

void ReadFromRom()
{
  ROMData = readStringFromROM();
  if(ROMData.length() != 0)
  {
    StationValue = (byte)(getStringBetween(ROMData, "*", 1).toInt());
    Serial.println("Read from ROM:");
    Serial.print("Station: "); Serial.println(StationValue);
    if((StationValue > 19) || (StationValue <1)) StationValue = 1;
    if (WiFi.status() != WL_CONNECTED)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wifi:");
      wifi_ssid = getStringBetween(ROMData, "*", 2);
      Serial.print("Wifi: "); Serial.println(wifi_ssid);
      wifi_password = getStringBetween(ROMData, "*", 3);
      Serial.print("Pass: "); Serial.println(wifi_password);
      lcd.setCursor(0, 1);
      lcd.print(wifi_ssid);
      ConnectToWifi(200);
    }
  }
  else
  {
    String DataToWrite;
    DataToWrite = "*" + String(StationValue) + "*" + wifi_ssid + "*" +  wifi_password + "*";
    writeStringToROM(DataToWrite);
  }
}
