#include <RH_ASK.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <cert.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <LiquidCrystal.h>
#include "time.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

esp_timer_handle_t timer;
// Khởi tạo đối tượng RTC_DS1307
RTC_DS1307 rtc;
#define SDA_PIN 17   // Chân SDA tùy chỉnh
#define SCL_PIN 18  // Chân SCL tùy chỉnh

#define EEPROM_ADDRESS 0x50 // Địa chỉ của EEPROM

// Thời gian chờ của watchdog (giây)
#define WATCHDOG_TIMEOUT_S 300

// Thông tin máy chủ NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 (Giờ Việt Nam)
const int   daylightOffset_sec = 0;
#define E_OK 0
#define E_NOT_OK 1

//Khởi tạo với các chân LCD
LiquidCrystal lcd(14, 27, 26, 25, 33, 32);
int lcdpin[6] = {4, 27, 26, 25, 33, 32};

Preferences preferences;

#define URL_fw_Bin "https://raw.githubusercontent.com/taelectronics/IOT-Toyota/main/RF-Transmitter-Reciever-main/ESP32_RF_SLAVE/.pio/build/esp32doit-devkit-v1/firmware.bin"
#define URL_fw_Version "https://raw.githubusercontent.com/taelectronics/IOT-Toyota/main/RF-Transmitter-Reciever-main/ESP32_RF/ImageFile/bin_version.txt"

// Thông tin Firebase
#define FIREBASE_HOST_PRIMARY "https://iot-toyota-1-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH_PRIMARY "SDE8HaiVyzKObClOF3FSoXBwh9K8qIjcvoC5KoMw"

#define FIREBASE_HOST_BACKUP "https://iot-toyota-backup-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH_BACKUP "1HCXadicLQ5GF0afUchHYwglSdV3y5LaBx3q5dGo"
#define TIME_RF_MAX 300
#define TIME_RF_RESET 200
#define RETRY_FIREBASE 4
#define OUTPUT1 19
#define OUTPUT2 23
#define SETBUTTTON 16
#define UPBUTTON 21
#define DOWNBUTTON 22
#define SELECT_FIREBASE 12
#define QUICK_UPDATE 4
#define NUMBER_OF_STATION 25
#define NUMBER_OF_DATA 30
#define AUTO_MODE 0
#define MANUAL_MODE 1
// Khởi tạo Firebase
FirebaseData firebaseDataPrimary;
FirebaseAuth firebaseAuthPrimary;
FirebaseConfig firebaseConfigPrimary;

// Khởi tạo Firebase
FirebaseData firebaseDataBackup;
FirebaseAuth firebaseAuthBackup;
FirebaseConfig firebaseConfigBackup;

String wifi_ssid = "TBHN employees";
String wifi_password = "Tbhn@2022";

// Example arrays
volatile int DayOfWeekCalculate;
volatile byte StationValue = 2;
volatile byte WifiStatus = E_NOT_OK;
volatile byte ModbusStatus = E_NOT_OK;
int timeValue;
int LCDCount = 0;
String FirmwareVer = "3.8.2.54";
volatile uint8_t Status[NUMBER_OF_STATION] = {0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile int hourSetRTC, minSetRTC, secSetRTC, daySetRTC, monthSetRTC, yearSetRTC;
uint16_t TimeGet[NUMBER_OF_DATA];
volatile byte DayStatus = E_NOT_OK;
volatile byte SendIP = E_NOT_OK;
String IPPath = "/Station/IP/";
String TimePath = "/Station/Time/";
String stationPath = "/Station/Status/";
String StationName[NUMBER_OF_STATION] = {"S00", "S01", "S02", "S03", "S04", "S05", "S06", "S07", "S08", "S09", "S10", "S11", "S12", "S13", "S14", "S15", "S16", "S17", "S18", "S19","S20", "S21","S22", "S23", "S24"};
String ROMData = "";
String data_Firebase_Compare;
byte Primary_Get = E_NOT_OK;
byte Backup_Get = E_NOT_OK;
byte Primary_Set = E_NOT_OK;
byte SetTimeRTC = E_NOT_OK;
volatile byte RunMode = AUTO_MODE; // 0 là Auto, 1 là Manual
volatile byte QuickUpdate;
volatile byte EmergencyStop = 0;
volatile int displayMode = 0;
volatile int NumberOfMinuteCompare = 0;
volatile int NumberOfMinuteNow = 0;
void firmwareUpdate();
int FirmwareVersionCheck(void);
void writeStringToROM(String myString);
String readStringFromROM();
String receiveStringFromSerial();
void SettingbySoftware();
boolean ConnectToWifi(byte retry);
void printLocalTime();
void reduceArray(uint8_t buf[40], uint8_t result[20]);
void reduceArrayFireBase(const uint16_t inputArray[], uint16_t outputArray[],  int inputSize);
int processString(const String &input, uint8_t output[20]);
bool getFirebaseData(FirebaseData &firebaseData, String &a, const String &path);
void DisplayLCD();
void stringToArray(String str, uint16_t arr[], uint16_t size);
bool checkTimeRange(uint16_t timeArray[], uint16_t arraySize, uint16_t timenow);
int CalculateTimeValue();
bool checkArray(uint16_t arr[], uint16_t a);
void ProcessOutput();
void ReadFromRom();
String getStringBetween(String input, String delimiter, int index);
String replaceFirstStringWithNumber(String originalString, int number); 
void writeArrayToEEPROM(uint16_t startAddress, uint16_t* data, uint16_t length);
void readArrayFromEEPROM(uint16_t startAddress, uint16_t* data, uint16_t length);
void ProcessRTC();
void InitRTC();
bool compareStrings(String str1, String str2);
void updateTime(int hour, int minute, int second, int day, int month, int year);
void processDataInRom();
int getDayOfWeek(int day, int month, int year);
byte RTCStatus = 1;
int hourRTC = 0; 
int minRTC = 0;
int secRTC = 0;
int dayRTC = 0; 
int monthRTC = 0;
int yeadRTC = 0;
int count = 0;
int minCompare = 0;
int count_compare = 1;
uint8_t data_RF[40]={0}; 
uint64_t TimeRF;
uint16_t TimeGetRTC[27];

void IRAM_ATTR onTimer(void* arg);
void InitTimer();

void setup() 
{
  pinMode(OUTPUT1,OUTPUT);
  pinMode(OUTPUT2,OUTPUT);
  pinMode(SETBUTTTON,INPUT_PULLUP);
  pinMode(UPBUTTON,INPUT_PULLUP);
  pinMode(DOWNBUTTON,INPUT_PULLUP);
  pinMode(SELECT_FIREBASE,INPUT_PULLUP);
  pinMode(QUICK_UPDATE,INPUT_PULLUP);
  digitalWrite(OUTPUT1,LOW);
  digitalWrite(OUTPUT2,LOW);
  Serial.begin(115200);
  InitRTC();
  processDataInRom();
  ProcessRTC();
  ProcessOutput();
  DisplayLCD();
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V: ");lcd.print(FirmwareVer);
  lcd.setCursor(0, 1);
  lcd.print("Wifi: "); lcd.print(wifi_ssid);
  delay(1000);
  // Đọc chuỗi từ ROM và hiển thị lên Serial
  ReadFromRom();

  // Cấu hình Firebase
  firebaseConfigPrimary.host = FIREBASE_HOST_PRIMARY;
  firebaseConfigPrimary.signer.tokens.legacy_token = FIREBASE_AUTH_PRIMARY;
  firebaseConfigPrimary.timeout.serverResponse = 100;
  // Cấu hình Firebase Backup
  firebaseConfigBackup.host = FIREBASE_HOST_BACKUP;
  firebaseConfigBackup.signer.tokens.legacy_token = FIREBASE_AUTH_BACKUP;
  firebaseConfigBackup.timeout.serverResponse = 100;
  // Kết nối Firebase
  Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
  Firebase.reconnectWiFi(true);
  // Kiểm tra kết nối Firebase
  if (Firebase.ready()) 
  {
    Serial.println("Connected to Primary Firebase");
  } 
  else 
  {
    Serial.println("Firebase not ready");
  }
  // Kết nối Firebase Backup
  Firebase.begin(&firebaseConfigBackup, &firebaseAuthBackup);
  Firebase.reconnectWiFi(true);
  // Kiểm tra kết nối Firebase
  if (Firebase.ready()) {
    Serial.println("Connected to Backup Firebase");
  } 
  else 
  {
    Serial.println("Firebase not ready");
  }

  // Serial.println(FirmwareVer);

    if (FirmwareVersionCheck()) 
    {
    firmwareUpdate();
    }
  // Cấu hình máy chủ NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  DisplayLCD();
  if(CalculateTimeValue() != (-1))
  {
    updateTime(hourSetRTC, minSetRTC, secSetRTC, daySetRTC, monthSetRTC, yearSetRTC);
    SetTimeRTC = E_OK;
  }
  // Khởi tạo watchdog
  esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);

  InitTimer();
}

void loop()
{
  LCDCount++;
  DisplayLCD();
  if(RTCStatus == 0)
  {
    lcd.setCursor(0,0);
    lcd.print("RTC not Running    ");
  }
  if(LCDCount > 100)
  {
    lcd.clear();
    lcd.getWriteError();
    lcd.clearWriteError();
    lcd.begin(16,2);
    LCDCount = 0;
  }
  if(SendIP == E_NOT_OK)
  {
  String IPAddress;
  IPAddress =  WiFi.localIP().toString();
  if(IPAddress == "")
  {
    IPAddress = "0.0.0.0";
  }
    Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
    if (Firebase.setString(firebaseDataPrimary, IPPath.c_str() + StationName[StationValue], IPAddress))
    {
      Serial.println("Finished to write IP");
      SendIP = E_OK;
    }
    else
    {
      Serial.println("Fail to write IP");
    }
  }

  if(SetTimeRTC == E_NOT_OK)
  {
    if(CalculateTimeValue() != (-1))
    {
      updateTime(hourSetRTC, minSetRTC, secSetRTC, daySetRTC, monthSetRTC, yearSetRTC);
      SetTimeRTC = E_OK;
    }
  }
  if((digitalRead(QUICK_UPDATE) == LOW) || (RunMode == MANUAL_MODE) || (EmergencyStop == 1))
  {
    QuickUpdate = 1;
  }
  else
  {
    QuickUpdate = 0;
  }
  uint8_t data_RF_lengt = 40;
  String data_Firebase;
  esp_task_wdt_reset();

  SettingbySoftware();
  if((NumberOfMinuteNow - NumberOfMinuteCompare) >= 2)
  {
    Primary_Set = E_NOT_OK;
    NumberOfMinuteCompare = NumberOfMinuteNow;
  }
  // Lựa chọn sử dụng DataBase Primary
  if(((Primary_Get == E_NOT_OK) && (digitalRead(SELECT_FIREBASE) == HIGH) ) || (QuickUpdate == 1))
  {
    Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
    if (getFirebaseData(firebaseDataPrimary, data_Firebase, stationPath.c_str() + StationName[StationValue])) 
    {
      Serial.println("Get Primary Firebase successful");
      Serial.println("Data: " + data_Firebase);
      stringToArray(data_Firebase, TimeGet, NUMBER_OF_DATA);
      Serial.println();
      if(compareStrings(data_Firebase_Compare,data_Firebase) == false)
      {
        writeArrayToEEPROM(0,TimeGet,NUMBER_OF_DATA);
        lcd.setCursor(0,0);
        lcd.print("Saved to ROM      ");
        Serial.println("Saved to ROM");
        processDataInRom();
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print("Nothing's change      ");
        Serial.println("Nothing's change");
      }

      data_Firebase_Compare = data_Firebase;
      Primary_Get = E_OK;
      WifiStatus = E_OK;
      delay(1000);
    } 
    else 
    {
      lcd.print("Primary FB Fail     ");
      Serial.println("Failed to get data from Primary");
      Primary_Get = E_NOT_OK;
      if (WiFi.status() != WL_CONNECTED)
      {
        lcd.setCursor(0,0);
        lcd.print("No Internet     ");
        WifiStatus = E_NOT_OK;
        Serial.println("Wifi isn't contected");
        WiFi.reconnect();
      }
      delay(1000);
    }
  }
  esp_task_wdt_reset();
  // Đọc dữ liệu từ đường dẫn "test/string"
  if(((Primary_Get == E_NOT_OK) || (digitalRead(SELECT_FIREBASE)==LOW))&&(Backup_Get == E_NOT_OK))
  {
    Firebase.begin(&firebaseConfigBackup, &firebaseAuthBackup);
    if (getFirebaseData(firebaseDataBackup, data_Firebase, stationPath.c_str() + StationName[StationValue])) 
    {
      Serial.println("Get Backup Firebase successful");
      Serial.println("Data: " + data_Firebase);
      stringToArray(data_Firebase, TimeGet, NUMBER_OF_DATA);
      if(compareStrings(data_Firebase_Compare,data_Firebase) == false)
      {
        writeArrayToEEPROM(0,TimeGet, NUMBER_OF_DATA);
        lcd.setCursor(0,0);
        lcd.print("Saved to ROM     ");
        Serial.println("Saved to ROM");
        processDataInRom();
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print("Nothing's change     ");
        Serial.println("Nothing's change");
      }
      data_Firebase_Compare = data_Firebase;
      Backup_Get = E_OK;
      WifiStatus = E_OK;
      delay(1000);
    } 
    else 
    {
      lcd.setCursor(0,0);
      lcd.print("Backup FB Fail     ");
      Serial.println("Failed to get data from Backup");
      Backup_Get = E_NOT_OK;
      if (WiFi.status() != WL_CONNECTED)
      {
        lcd.setCursor(0,0);
        lcd.print("No Internet     ");
        Serial.println("Wifi isn't contected");
        WiFi.reconnect();
      }
      WifiStatus = E_NOT_OK;
      delay(1000);
    }
  }
  if(Primary_Set == E_NOT_OK)
  {
    if (Firebase.setString(firebaseDataPrimary, TimePath.c_str() + StationName[StationValue], String(NumberOfMinuteNow)))
    {
      Serial.println("Finished to write NumberOfMinuteNow");
      Primary_Set = E_OK;
    }
    else
    {
      Serial.println("Fail to write NumberOfMinuteNow");
      Serial.println(firebaseDataPrimary.errorReason());
      Primary_Set = E_NOT_OK;
    }
  }
  delay(1000);
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

// Hàm rút gọn mảng 40 phần tử thành 20 phần tử
void reduceArray(uint8_t buf[40], uint8_t result[20]) {
  for (int i = 0; i < 20; i++) {
    // Lấy 2 phần tử liền nhau từ mảng buf và ghép lại thành 1 số
    result[i] = (buf[2 * i]-48) * 10 + (buf[2 * i + 1]-48);
  }
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
  String myString = preferences.getString("myString", "Chuỗi mặc định");

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
        // Đọc chuỗi từ ROM và hiển thị lên Serial
        if(compareStrings(receivedString,readStringFromROM()))
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Finished to Set");
          lcd.setCursor(0,1);
          lcd.print("Please reset");
          delay(2000);
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


void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println("Get time from Internet");
  minRTC = timeinfo.tm_min;
  hourRTC = timeinfo.tm_hour;
  secRTC = timeinfo.tm_sec;
  dayRTC = timeinfo.tm_mday;
  monthRTC = timeinfo.tm_mon;
  yeadRTC = timeinfo.tm_year;
  timeValue= timeinfo.tm_hour*60 + timeinfo.tm_min;
}

// Hàm xử lý chuỗi và tạo mảng uint8_t
int processString(const String &input, uint8_t output[20])
{
  // Kiểm tra độ dài của chuỗi
  if (input.length() < 40) {
    return -1; // Chuỗi ngắn hơn 40 ký tự
  }
  
  // Xử lý chuỗi và tạo mảng uint8_t
  for (int i = 0; i < 20; i++) {
    // Lấy 2 ký tự liền nhau và ghép lại thành 1 số
    int firstDigit = input[2 * i] - '0';  // Chuyển ký tự sang số
    int secondDigit = input[2 * i + 1] - '0';  // Chuyển ký tự sang số
    output[i] = firstDigit * 10 + secondDigit;
  }
  return 0; // Thành công
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
void Screen0()
{
  lcd.setCursor(0,0);
  lcd.print(DayOfWeekCalculate+2);
  lcd.print("-");
  lcd.print(hourRTC);
  lcd.print(":");
  lcd.print(minRTC);
  lcd.print(":");
  lcd.print(secRTC);
  lcd.print(" ");
  lcd.print(StationName[StationValue]);
  lcd.print(":");
  if(digitalRead(OUTPUT1)==LOW)
  {
    lcd.print("1");
  }
  else
  {
    lcd.print("0");
  }
  lcd.print("        ");
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

  lcd.print("P:");
  if(Primary_Get == E_OK)
  {
    lcd.print("1 ");
  }
  else
  {
    lcd.print("0 ");
  }
    
  lcd.print("B:");
  if(Backup_Get == E_OK)
  {
    lcd.print("1 ");
  }
  else
  {
    lcd.print("0 ");
  }
  lcd.print("D");
  lcd.print(":");
  if(DayStatus == E_OK)
  {
    lcd.print("1      ");
  }
  else
  {
    lcd.print("0     ");
  }
  
}

void Screen1()
{
  lcd.setCursor(0,0);
  lcd.print("RTC: ");
  lcd.print(hourRTC);
  lcd.print(":");
  lcd.print(minRTC);
  lcd.print("        ");
  lcd.setCursor(0,1);
  lcd.print(StationName[StationValue]);
  lcd.print(":");
  if(digitalRead(OUTPUT1)==HIGH)
  {
    lcd.print("ON ");
  }
  else
  {
    lcd.print("OFF");
  }
  lcd.print("        ");
}

void DisplayLCD()
{
  if(displayMode == 0)
  {
    Screen0();
  }
  else if(displayMode == 1)
  {
    Screen1();
  }
}

// Hàm chuyển đổi chuỗi thành mảng
void stringToArray(String str, uint16_t arr[], uint16_t size) {
  uint16_t startIndex = 0;
  uint16_t endIndex = 0;
  uint16_t arrIndex = 0;

  while (arrIndex < size && startIndex != -1) {
    startIndex = str.indexOf('*', endIndex);
    if (startIndex != -1) {
      endIndex = str.indexOf('*', startIndex + 1);
      if (endIndex != -1) {
        String numStr = str.substring(startIndex + 1, endIndex);
        arr[arrIndex] = numStr.toInt();
        arrIndex++;
      }
    }
  }
}

void reduceArrayFireBase(const uint16_t inputArray[], uint16_t outputArray[],  int inputSize) {
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

        if (startTime != 0 || endTime != 0) {
            if (timenow >= startTime && timenow <= endTime) {
                return true;
            }
        }
    }
    return false;
}

bool checkArray(uint16_t arr[], uint16_t a) {
    for (int i = 0; i < 20; i += 2) {
        if (i + 1 < 20) {  // Đảm bảo không vượt quá giới hạn của mảng
            if (a >= arr[i] && a < arr[i + 1]) {
                return true;  // Trả về 1 nếu điều kiện thỏa mãn
            }
        }
    }
    return false;  // Trả về 0 nếu không có cặp nào thỏa mãn điều kiện
}
int CalculateTimeValue() 
{
  struct tm timeinfo;
  int timeGet = 0;
  if (!getLocalTime(&timeinfo)) 
  {
    Serial.println("Failed to obtain time");
    return -1;
  }
  // Lấy giờ, phút và giây
  // Lấy giờ, phút, giây, ngày, tháng và năm
  hourSetRTC = timeinfo.tm_hour;
  minSetRTC = timeinfo.tm_min;
  secSetRTC = timeinfo.tm_sec;  // Đã sửa lỗi đánh máy ở đây
  daySetRTC = timeinfo.tm_mday;
  monthSetRTC = timeinfo.tm_mon + 1;  // Tháng bắt đầu từ 0, nên cần +1
  yearSetRTC = timeinfo.tm_year + 1900;  // Năm bắt đầu từ 1900, nên cần +1900
  timeGet = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  Serial.println("Set time to RTC");
  return timeGet;
}
void ProcessOutput()
{

    if(Status[StationValue] == 1)
    {
      digitalWrite(OUTPUT1,LOW);
    }
    else 
    {
      digitalWrite(OUTPUT1,HIGH);
    }
}

void ReadFromRom()
{
  ROMData = readStringFromROM();
  if(ROMData.length() != 0)
  {
    StationValue = (byte)(getStringBetween(ROMData, "*", 1).toInt());
    Serial.println("Read from ROM:");
    Serial.print("Station: "); Serial.println(StationValue);
    if((StationValue > 24) || (StationValue <1)) StationValue = 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wifi:");
      wifi_ssid = getStringBetween(ROMData, "*", 2);
      Serial.print("Wifi: "); Serial.println(wifi_ssid);
      wifi_password = getStringBetween(ROMData, "*", 3);
      Serial.print("Pass: "); Serial.println(wifi_password);
      lcd.setCursor(0, 1);
      lcd.print(wifi_ssid);
      ConnectToWifi(100);
  }
  else
  {
    String DataToWrite;
    DataToWrite = "*" + String(StationValue) + "*" + wifi_ssid + "*" +  wifi_password + "*";
    writeStringToROM(DataToWrite);
  }
}

// Hàm để lấy chuỗi nằm giữa 2 dấu *
String getStringBetween(String input, String delimiter, int index) 
{
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
  if (toIndex == -1) 
  {
    toIndex = input.length();
  }
  
  return input.substring(fromIndex, toIndex);
}

String replaceFirstStringWithNumber(String originalString, int number) 
{
  // Chuyển số thành chuỗi
  String numberString = String(number);
  
  // Tìm vị trí của dấu '*' đầu tiên và thứ hai
  int firstStarPos = originalString.indexOf('*');
  int secondStarPos = originalString.indexOf('*', firstStarPos + 1);

  // Nếu không tìm thấy đủ dấu '*', trả về chuỗi gốc
  if (firstStarPos == -1 || secondStarPos == -1) {
    return originalString;
  }

  // Tạo chuỗi mới với số thay thế
  String newString = originalString.substring(0, firstStarPos + 1) + numberString + originalString.substring(secondStarPos);

  return newString;
}


void InitRTC()
{
    // Khởi tạo I2C
  Wire.begin(SDA_PIN,SCL_PIN);

  // Kiểm tra kết nối RTC
  if (!rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    lcd.setCursor(0,0);
    lcd.print("Couldn't find RTC   ");
  }

  // Kiểm tra xem RTC có đang chạy không
  if (!rtc.isrunning()) 
  {
    Serial.println("RTC is NOT running!");
    lcd.setCursor(0,0);
    lcd.print("RTC is NOT running!    ");
    // Thiết lập thời gian ban đầu cho RTC
    // Chỉ cần thực hiện điều này một lần
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void ProcessRTC()
{
  DateTime now = rtc.now();
  uint16_t timeValueRTC;
  // Serial.print("yearRTC:"); Serial.println(now.year());
  // Serial.print("monthRTC:"); Serial.println(now.month());
  // Serial.print("dayRTC:"); Serial.println(now.day());
  // Serial.print("hourRTC:"); Serial.println(now.hour());
  // Serial.print("minuteRTC:"); Serial.println(now.minute());
  // Serial.print("minuteRTC:"); Serial.println(now.second());
  if((now.day() < 32) && (now.month() < 13) && (now.year() > 2023) && (now.hour() < 25) && (now.minute() < 61) && (now.second() < 61))
  {
    minRTC = now.minute();
    hourRTC = now.hour();
    secRTC = now.second();
    dayRTC = now.day();
    monthRTC = now.month();
    yeadRTC = now.year();
    RTCStatus = 1;
  }
  else
  {
    RTCStatus = 0;
    printLocalTime();
  }
  NumberOfMinuteNow = hourRTC*60+minRTC;
  if(minCompare != minRTC)
  {
    Primary_Get = E_NOT_OK;
    Backup_Get = E_NOT_OK;
    minCompare = minRTC;
  }
  timeValueRTC = (uint16_t)hourRTC * 60 + (uint16_t)minRTC;

  DayOfWeekCalculate = getDayOfWeek(dayRTC,monthRTC,yeadRTC);
  if(TimeGet[20+8] == 0x01)
  {
    RunMode = MANUAL_MODE;
  }
  else
  {
    RunMode = AUTO_MODE;
  }

  if(TimeGet[20 + DayOfWeekCalculate] == 0x01)
  {
    DayStatus = E_OK;
  }
  else
  {
    DayStatus = E_NOT_OK;
  }

  if(TimeGet[20 + 8] == 0x03)
  {
    SendIP = E_NOT_OK;
  }
  else
  {
    SendIP = E_OK;
  }

  if(RunMode == AUTO_MODE )
  {
    if((true == checkArray(TimeGetRTC, timeValueRTC)) && (DayStatus == E_OK))
    {
      Status[StationValue] = 1;
    }
    else
    {
      Status[StationValue] = 0;
    }
  }

  if(RunMode == MANUAL_MODE )
  {
    if(TimeGet[20 + 7] == 0x01)
    {
      Status[StationValue] = 1;
    }
    else
    {
      Status[StationValue] = 0;
    }
  }

  if(TimeGet[20 + 9] ==  0x01 )
  {
    Status[StationValue] = 0;
    EmergencyStop = 1;
  }
  else
  {
    EmergencyStop = 0;
  }
  
}

// Hàm để ghi mảng uint16_t vào EEPROM
void writeArrayToEEPROM(uint16_t startAddress, uint16_t* data, uint16_t length) 
{
  for (uint16_t i = 0; i < length; i++) 
  {
    uint16_t value = data[i];
    uint8_t highByte = (value >> 8) & 0xFF; // Byte cao
    uint8_t lowByte = value & 0xFF;         // Byte thấp

    Wire.beginTransmission(EEPROM_ADDRESS);
    Wire.write((int)(startAddress >> 8));   // MSB của địa chỉ
    Wire.write((int)(startAddress & 0xFF)); // LSB của địa chỉ
    Wire.write(highByte); // Ghi byte cao
    Wire.write(lowByte);  // Ghi byte thấp
    Wire.endTransmission();
    startAddress += 2; // Di chuyển đến địa chỉ tiếp theo
    delay(5); // Đợi để ghi xong
  }
}

// Hàm để đọc mảng uint16_t từ EEPROM
void readArrayFromEEPROM(uint16_t startAddress, uint16_t* data, uint16_t length) 
{
  for (uint16_t i = 0; i < length; i++) 
  {
    Wire.beginTransmission(EEPROM_ADDRESS);
    Wire.write((int)(startAddress >> 8));   // MSB của địa chỉ
    Wire.write((int)(startAddress & 0xFF)); // LSB của địa chỉ
    Wire.endTransmission();

    Wire.requestFrom(EEPROM_ADDRESS, 2); // Yêu cầu 2 byte
    if (Wire.available() == 2) {
      uint8_t highByte = Wire.read(); // Đọc byte cao
      uint8_t lowByte = Wire.read();  // Đọc byte thấp
      data[i] = (highByte << 8) | lowByte; // Kết hợp hai byte thành uint16_t
    }
    startAddress += 2; // Di chuyển đến địa chỉ tiếp theo
    delay(5); // Đợi để đọc xong
  }
}

void updateTime(int hour, int minute, int second, int day, int month, int year) {
  // Tạo một đối tượng DateTime với các giá trị truyền vào
  DateTime newTime = DateTime(year, month, day, hour, minute, second);
  
  // Cập nhật thời gian cho DS1307
  rtc.adjust(newTime);

  Serial.print("Updated time to: ");
  Serial.print(newTime.year(), DEC);
  Serial.print('/');
  Serial.print(newTime.month(), DEC);
  Serial.print('/');
  Serial.print(newTime.day(), DEC);
  Serial.print(' ');
  Serial.print(newTime.hour(), DEC);
  Serial.print(':');
  Serial.print(newTime.minute(), DEC);
  Serial.print(':');
  Serial.print(newTime.second(), DEC);
  Serial.println();
}

void processDataInRom()
{
  readArrayFromEEPROM(0, TimeGetRTC, NUMBER_OF_DATA);
  Serial.print("Read from 34C08: ");
  for(int i = 0; i< NUMBER_OF_DATA; i++)
  {
    Serial.print(TimeGetRTC[i]);
    Serial.print("-");
  }
  Serial.println();
}

int getDayOfWeek(int day, int month, int year) {
    // Điều chỉnh tháng và năm nếu tháng là January hoặc February
    if (month < 3) 
    {
        month += 12;
        year -= 1;
    }

    int k = year % 100; // Hai chữ số cuối của năm
    int j = year / 100; // Hai chữ số đầu của năm

    // Công thức Zeller’s Congruence
    int dayOfWeek = (day + 13 * (month + 1) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;

    // Điều chỉnh giá trị trả về để thứ Hai là 0, Chủ Nhật là 6
    dayOfWeek = (dayOfWeek + 5) % 7;

    return dayOfWeek;
}

void InitTimer()
{
      // Cấu hình và khởi tạo timer
    const esp_timer_create_args_t timer_args = {
        .callback = &onTimer,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "example_timer"
    };

    esp_err_t err = esp_timer_create(&timer_args, &timer);
    if (err != ESP_OK) {
        Serial.println("Failed to create timer");
        return;
    }

    // Đặt timer để hết hạn sau 1 giây (1000000 micro giây)
    err = esp_timer_start_once(timer, 1000000);
    if (err != ESP_OK) {
        Serial.println("Failed to start timer");
        return;
    }

    Serial.println("Timer started");
}

// Hàm callback sẽ được gọi khi timer hết hạn
void IRAM_ATTR onTimer(void* arg) {
    // Xử lý công việc khi ngắt xảy ra
    ProcessRTC();
    ProcessOutput();
    // Khởi động lại timer
    esp_timer_start_once(timer, 1000000); // 1 giây
}