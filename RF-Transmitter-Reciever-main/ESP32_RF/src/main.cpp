#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <cert.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <LiquidCrystal.h>
#include "time.h"
#include "esp_task_wdt.h"
// Thông tin máy chủ NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 (Giờ Việt Nam)
const int   daylightOffset_sec = 0;

// Thời gian chờ của watchdog (giây)
#define WATCHDOG_TIMEOUT_S 300

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
#define E_STOP_BUTTON 4
#define SELECT_FIREBASE_BACKUP 35
#define NUMBER_OF_STATION 25

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
byte FinishtoClear = E_NOT_OK;
// Example arrays
static byte modbusStatus = E_NOT_OK;
static byte StationValue;
static byte Firebase_Backup_Status = E_NOT_OK;
static byte Firebase_Primary_Status = E_NOT_OK;
static byte SetTimeHMI = E_NOT_OK;
static int hourGet = 0, minGet = 0 , secGet = 0;
static int dayGet = 0, monthGet = 0 , yearGet = 0, dayOfweekGet  = 0, dayOfweekProcessed = 0;
static uint16_t HMITime[7];
static uint16_t timeValue;
static uint16_t TimeGet[40] = {};
volatile uint16_t DayOfWeekToday[NUMBER_OF_STATION];
static uint16_t HMIDayOfWeek[NUMBER_OF_STATION*7];
static uint16_t HMIDayOfWeekCompare[NUMBER_OF_STATION*7];
static uint16_t TimeProcessed[NUMBER_OF_STATION][20];
static uint16_t TimeCompare[NUMBER_OF_STATION][20];
static uint16_t IPNumberArray[NUMBER_OF_STATION*4];
static uint16_t SettingParametter[80];
static uint16_t StationStatus[NUMBER_OF_STATION];
static byte CountNumber = 0;
String StationName[NUMBER_OF_STATION] = {"S00", "S01", "S02", "S03", "S04", "S05", "S06", "S07", "S08", "S09", "S10", "S11", "S12", "S13", "S14", "S15", "S16", "S17", "S18", "S19", "S20", "S21", "S22", "S23", "S24"};
String FirmwareVer = "4.8.4.19";
byte Firebase_Primary_Set[NUMBER_OF_STATION] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
byte Firebase_Backup_Set[NUMBER_OF_STATION] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
String stationStatusPath = "/Station/Status/";
String stationIPPath = "/Station/IP/";
String stationDayofWeekPath = "/Station/DayofWeek";
String ROMData = "";
String data_Firebase;
uint8_t  IPNumber[4];

byte WriteIP = E_NOT_OK;
void reduceArray(const uint16_t inputArray[], uint16_t outputArray[],  int inputSize);
bool readRegisters(uint16_t startReg, uint16_t count, uint16_t* values);
void firmwareUpdate();
int FirmwareVersionCheck(void);
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
void screen1();
bool ReadTimeFromHMI();
void ReadFromRom();
bool ReadDayOfWeekFromHMI(uint16_t startAddress);
bool ReadSettingParam(uint16_t startAddress);
String convertArrayToString(uint16_t arr[], int length, int startIndex, int numElements);
void parseIP(const String& ipStr, uint8_t ipArray[4]);
void readIPAddress();
bool WriteIPAddressToHMI(uint16_t startAddress, uint16_t count, uint16_t* inputArray) ;
bool WriteStatusToHMI(uint16_t startAddress, uint16_t count);
bool checkArray(uint16_t arr[], uint16_t a);
int getDayOfWeek(int day, int month, int year);
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
  // Đọc chuỗi từ ROM và hiển thị lên Serial
  ReadFromRom();
  firebaseConfigPrimary.host = FIREBASE_HOST_PRIMARY;
  firebaseConfigPrimary.signer.tokens.legacy_token = FIREBASE_AUTH_PRIMARY;
  // firebaseConfigPrimary.timeout.serverResponse = 1000;

  // // Cấu hình Firebase Backup
  firebaseConfigBackup.host = FIREBASE_HOST_BACKUP;
  firebaseConfigBackup.signer.tokens.legacy_token = FIREBASE_AUTH_BACKUP;
  // firebaseConfigBackup.timeout.serverResponse = 1000;

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
  // Khởi tạo watchdog
  esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
  pinMode(E_STOP_BUTTON, INPUT_PULLUP);
}
int count;
int firstTime = 0;
void loop()
{
  // if(digitalRead(E_STOP_BUTTON)==HIGH)
  // {
  //   lcd.setCursor(0,0);
  //   lcd.print("Emergency Stop");
  // }
  if((false == ReadTimeFromHMI())||(SetTimeHMI == E_NOT_OK))
  {
    printLocalTime();
  }
  if(true == ReadDayOfWeekFromHMI(200))
  {
    for(int i = 7; i<(NUMBER_OF_STATION*7); i++)
    {
      if(HMIDayOfWeekCompare[i] != HMIDayOfWeek[i])
      {
      Firebase_Primary_Set[i/7] = E_NOT_OK;
      Firebase_Backup_Set[i/7] = E_NOT_OK;
      }
    }
    for (int i = 0; i < (NUMBER_OF_STATION*7); i++)
    {
      HMIDayOfWeekCompare[i] = HMIDayOfWeek[i];
    }
    modbusStatus = E_OK;
    dayOfweekProcessed = getDayOfWeek(dayGet,monthGet,yearGet);
  }
  else
  {
    modbusStatus = E_NOT_OK;
  }
// nếu là chế độ manual thì gán kèm giá trị vào cùng với thứ trong tuần
// quy ước: nếu on thì gửi đi kèm 0x02 nếu of thì gửi 0x00

  if(true == ReadSettingParam(0))
  {
    if(SettingParametter[0]==1)
    {
      Serial.println("Manual Mode");
      for(int i = 1; i< NUMBER_OF_STATION; i++)
      {
        if(SettingParametter[i] == 1)
        {
          for(int y = 0; y<7; y++)
          {
            HMIDayOfWeek[i*7 + y] |= 0x02;
          }
        }
        else
        {
          for(int y = 0; y<7; y++)
          {
            HMIDayOfWeek[i*7 + y] = 0x0;
          }
        } 
        Firebase_Primary_Set[i] = E_NOT_OK;
        Firebase_Backup_Set[i] = E_NOT_OK;
      }
    }
    if(SettingParametter[30]==1)
    {
      Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
      delay(50);
      if (FinishtoClear == E_NOT_OK)
      {
        if(Firebase.deleteNode(firebaseDataPrimary,stationIPPath))
        {
          Serial.println("Finished to Clear");
          FinishtoClear = E_OK;
        }
      }
      for(int Index = 0; Index < NUMBER_OF_STATION*7; Index++)
      {
        HMIDayOfWeek[Index] |= 0x04;
      }
      for(int i = 0 ; i < NUMBER_OF_STATION; i++)
      {
        Firebase_Primary_Set[i] = E_NOT_OK;
      }

      WriteIP = E_NOT_OK;
      CountNumber = 0;
    }
    else
    {
      Serial.println("Don't update");
      FinishtoClear = E_NOT_OK;
      WriteIP = E_OK;
    }
    modbusStatus = E_OK;
  }
  else
  {
    modbusStatus = E_NOT_OK;
  }
  // if(digitalRead(E_STOP_BUTTON)==HIGH)
  // {
  //   for(int i = 0; i < NUMBER_OF_STATION*7; i++)
  //   {
  //     HMIDayOfWeek[i] = 0x00;
  //   }
  // }



  esp_task_wdt_reset();
  SettingbySoftware();
  screen1();
  for(int RowIndex = 0; RowIndex < NUMBER_OF_STATION; RowIndex++)
  {
    for(int ColIndex = 0; ColIndex <20; ColIndex++)
    {
      TimeCompare[RowIndex][ColIndex] = TimeProcessed[RowIndex][ColIndex];
    }
  }
  esp_task_wdt_reset();
  for (uint16_t i = 1; i < NUMBER_OF_STATION; i++)
  {
    if (true == readRegisters(1000+i*40, 40, TimeGet))
    {
      esp_task_wdt_reset();
      reduceArray(TimeGet, TimeProcessed[i], 40);
      delay(10);
    }
  }
  for(int RowIndex = 1; RowIndex < NUMBER_OF_STATION; RowIndex++)
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
for( int Num = 1; Num < NUMBER_OF_STATION; Num++)
{
  if((Firebase_Primary_Set[Num] == E_NOT_OK))
  {
      Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
      Serial.println("Begin with Primary Firebase");
      break;
  }
}

for( int Num = 1; Num < NUMBER_OF_STATION; Num++)
{
  if((Firebase_Primary_Set[Num] == E_NOT_OK) && (modbusStatus == E_OK))
  { 
    esp_task_wdt_reset();
    // Lựa chọn sử dụng DataBase Primary
    if (Firebase.ready())
    {
      data_Firebase = arrayToString(TimeProcessed[Num], 20);
      data_Firebase += convertArrayToString(HMIDayOfWeek, NUMBER_OF_STATION*7, Num*7, 7);
      if (Firebase.setString(firebaseDataPrimary, stationStatusPath.c_str() + StationName[Num], data_Firebase.c_str()))
      {
        lcd.setCursor(0,0);
        lcd.print("PRI "); lcd.print(Num);
        lcd.print(": OK    ");
        Serial.print("Write Successfully to The Primary Firebase: ");Serial.println(Num);
        Firebase_Primary_Set[Num] = E_OK;
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print("PRI "); lcd.print(Num);
        lcd.print(": ERROR    ");
        Firebase_Primary_Set[Num] = E_NOT_OK;
        Serial.println("Failed to Write Primary Firebase: "); Serial.println(Num);
        Serial.println(firebaseDataPrimary.errorReason());
      }

      Serial.println(data_Firebase);
      // Firebase.setString(firebaseDataPrimary, stationIPPath.c_str() + StationName[Num], "192.168.1.2");
      WifiStatus = E_OK;
    }
    else
    {
      Firebase_Primary_Set[Num] = E_NOT_OK;
      Serial.println("Firebase is not ready");
      if (WiFi.status() != WL_CONNECTED)
      {
        lcd.setCursor(0,0);
        lcd.print("Wifi isn't contected"); 
        WifiStatus = E_NOT_OK;
        Serial.println("Wifi isn't contected");
        WiFi.reconnect();
      }
      delay(1000);
    }
  delay(100);
  }
}
for( int Num = 1; Num < NUMBER_OF_STATION; Num++)
{
  if((Firebase_Backup_Set[Num] == E_NOT_OK))
  {
    Firebase.begin(&firebaseConfigBackup, &firebaseAuthBackup);
    Serial.println("Begin with Backup Firebase");
    break;
  }
}
  // Lựa chọn sử dụng DataBase Backup
for( int Num = 1; Num < NUMBER_OF_STATION; Num++)
{
  if((Firebase_Backup_Set[Num] == E_NOT_OK) && (modbusStatus == E_OK))
  {
    esp_task_wdt_reset();
    if (Firebase.ready())
    {
      data_Firebase = arrayToString(TimeProcessed[Num], 20);
      data_Firebase += convertArrayToString(HMIDayOfWeek, NUMBER_OF_STATION*7, Num*7, 7);
      if (Firebase.setString(firebaseDataBackup, stationStatusPath.c_str() + StationName[Num] , data_Firebase.c_str()))
      {
        lcd.setCursor(0,0);
        lcd.print("BAK "); lcd.print(Num);
        lcd.print(": OK            ");
        Serial.print("Write Successfully to The Backup Firebase: ");
        Serial.println(Num);
        Firebase_Backup_Set[Num] = E_OK;
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print("BAK "); lcd.print(Num);
        lcd.print(": ERROR        ");
        Firebase_Backup_Set[Num] = E_NOT_OK;
        Serial.print("Failed to Write Backup Firebase: "); Serial.println(Num);
        Serial.println(firebaseDataBackup.errorReason());
      }
      Serial.println(data_Firebase);
      WifiStatus = E_OK;
    }
    else
    {
      Firebase_Backup_Set[Num] = E_NOT_OK;
      Serial.println("Firebase is not ready");
      WifiStatus = E_NOT_OK;
      if (WiFi.status() != WL_CONNECTED)
      {
        lcd.setCursor(0,0);
        lcd.print("Wifi isn't contected"); 
        Serial.println("Wifi isn't contected");
        WiFi.reconnect();
        delay(1000);
      }
    }
  delay(100);
  } 
}

Firebase.begin(&firebaseConfigPrimary, &firebaseAuthPrimary);
if (WriteIP == E_NOT_OK)
{
  for( int Num = 1; Num < NUMBER_OF_STATION; Num++)
  {
    String result = "";
    Serial.print("GetIP:"); Serial.println(Num);
    // Lấy dữ liệu từ Firebase
    if (Firebase.getString(firebaseDataPrimary, (stationIPPath + StationName[Num]).c_str())) 
    {
        result = firebaseDataPrimary.stringData();
        parseIP(result, IPNumber);
        
        for(int i = 0; i<4; i++)
        {
          IPNumberArray[Num*4+i] = IPNumber[i];
        }
    } 
    else 
    {
      for(int i = 0; i<4; i++)
      {
        IPNumberArray[Num*4+i] = 0;
      }
      Serial.println("Failed to get data from Firebase");
      Serial.println("Reason: " + firebaseDataPrimary.errorReason());
    }
  }
}

  // && (DayOfWeekToday[Num] != 0x00) 
  for (uint16_t Index = 1; Index < NUMBER_OF_STATION; Index++)
  {
      if( (true == checkArray(TimeProcessed[Index], timeValue)) && (HMIDayOfWeek[ Index*7 + dayOfweekProcessed] != 0x00))
      {
        StationStatus[Index] = 1;
      }
      else
      {
        StationStatus[Index] = 0;
      }
      if (CountNumber == 1)
      {
        IPNumberArray[Index] = StationStatus[Index];
      }
      Serial.print("-");
      Serial.print(StationStatus[Index]);
      Serial.print(" ");
  }

  Serial.println(dayOfweekProcessed);

  if (CountNumber == 0)
  {
    if(WriteIPAddressToHMI(600, NUMBER_OF_STATION*4, IPNumberArray))
    {
      Serial.println("Write IP Successfully");
    }
    else
    {
      Serial.println("Fail to Write IP");
    }
  }
  else
  {
    if(WriteIPAddressToHMI(700, NUMBER_OF_STATION, IPNumberArray))
    {
      Serial.println("Write Status Successfully");
    }
    else
    {
      Serial.println("Fail to Write IP");
    }
  }
  CountNumber++;
  if(CountNumber > 1) CountNumber = 1;
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
  result = readRegisters(9017, 7, HMITime);
  if(result == true)
  {
    timeValue = HMITime[2]*60 + HMITime[1];
    Serial.print("Time HMI: "); Serial.print(HMITime[2]); Serial.print(":"); Serial.println(HMITime[1]);
    Serial.println(timeValue);
  }
  hourGet = (int)HMITime[2];
  minGet = (int)HMITime[1];
  secGet = (int)HMITime[0];
  dayGet = (int)HMITime[3];
  monthGet = (int)HMITime[4];
  yearGet = (int)HMITime[5];
  return result;
}

String convertArrayToString(uint16_t arr[], int length, int startIndex, int numElements) {
  String result = ""; // Chuỗi kết quả

  // Kiểm tra chỉ số bắt đầu và số lượng phần tử có hợp lệ không
  if (startIndex < 0 || startIndex >= length || numElements <= 0) {
    return "Invalid input"; // Trả về thông báo lỗi nếu đầu vào không hợp lệ
  }

  // Xác định phần tử kết thúc (không vượt quá độ dài mảng)
  int endIndex = startIndex + numElements;
  if (endIndex > length) {
    endIndex = length;
  }

  for (int i = startIndex; i < endIndex; i++) {
    result += arr[i]; // Thêm ký tự từ mảng vào chuỗi
    result += "*"; // Thêm dấu '*' nếu không phải là ký tự cuối cùng
  }

  return result; // Trả về chuỗi kết quả
}

bool ReadSettingParam(uint16_t startAddress)
{
  bool result;
  result = readRegisters(startAddress, 64, SettingParametter);
  if(result == false)
  {
    Serial.println("Modbus Fail 1");
    return result;
  }
  return result;
}

bool ReadDayOfWeekFromHMI(uint16_t startAddress)
{
  bool result;
  uint16_t HMIDayOfWeekStub[60];
  result = readRegisters(startAddress, 60, HMIDayOfWeekStub);
  if(result == false)
  {
    Serial.println("Modbus Fail 1");
    return result;
  }
  for( int i = 0; i < 60; i++)
  {
    HMIDayOfWeek[i] = HMIDayOfWeekStub[i];
  }

  result = readRegisters(startAddress+60, 60, HMIDayOfWeekStub);
  if(result == false)
  {
    Serial.println("Modbus Fail 2");
    return result;
  }
  for(int i = 60; i < 120; i++)
  {
    HMIDayOfWeek[i] = HMIDayOfWeekStub[i-60];
  }
    
  result = readRegisters(startAddress + 120, 60, HMIDayOfWeekStub);
  if(result == false)
  {
    Serial.println("Modbus Fail 3");
    return result;
  }
  for(int i = 120; i < NUMBER_OF_STATION*7; i++)
  {
    HMIDayOfWeek[i] = HMIDayOfWeekStub[i-120];
  }
  /*
  for(int i = 0; i < 140; i++)
  {
    Serial.print(HMIDayOfWeek[i]);
    Serial.print(" ");
  }
  Serial.println();
  */

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
  dayGet = timeinfo.tm_mday;
  monthGet = timeinfo.tm_mon;
  yearGet = timeinfo.tm_year;
  timeValue = timeinfo.tm_hour*60 + timeinfo.tm_min;
  if(SetTimeHMI == E_NOT_OK)
  {
    uint16_t startAddress = 9017;
    // Ghi dữ liệu vào buffer truyền
    node.setTransmitBuffer(0, secGet); // Giá trị 0x1234 vào thanh ghi đầu tiên
    node.setTransmitBuffer(1, minGet); // Giá trị 0x5678 vào thanh ghi thứ hai
    node.setTransmitBuffer(2, hourGet); // Giá trị 0x9ABC vào thanh ghi thứ ba
    // node.setTransmitBuffer(3, dayGet); // Giá trị 0x1234 vào thanh ghi đầu tiên
    // node.setTransmitBuffer(4, monthGet); // Giá trị 0x5678 vào thanh ghi thứ hai
    // node.setTransmitBuffer(5, yearGet); // Giá trị 0x9ABC vào thanh ghi thứ ba
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

void screen1()
{
  lcd.setCursor(0,0);
  lcd.print("M:");
  if(modbusStatus == E_OK)
  {
    lcd.print("1");
  }
  else
  {
    lcd.print("0");
  }
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

// Hàm tách chuỗi IP thành 4 số và lưu vào mảng ipArray
void parseIP(const String& ipStr, uint8_t ipArray[4]) {
  int partIndex = 0;  // Chỉ số của phần hiện tại trong IP (0 đến 3)
  
  // Tạo bản sao của chuỗi IP để xử lý
  String ipCopy = ipStr;
  int startIndex = 0;
  int endIndex = ipCopy.indexOf('.'); // Tìm dấu chấm đầu tiên

  // Lặp qua các phần của chuỗi IP
  while (endIndex != -1 && partIndex < 4) {
    ipArray[partIndex++] = ipCopy.substring(startIndex, endIndex).toInt();  // Lấy phần và chuyển đổi thành số nguyên
    startIndex = endIndex + 1; // Cập nhật chỉ số bắt đầu cho phần tiếp theo
    endIndex = ipCopy.indexOf('.', startIndex); // Tìm dấu chấm tiếp theo
  }
  
  // Xử lý phần cuối cùng của chuỗi IP (không có dấu chấm sau nó)
  if (partIndex < 4) {
    ipArray[partIndex++] = ipCopy.substring(startIndex).toInt();
  }

  // Nếu chuỗi IP không hợp lệ (không đủ 4 phần), đặt tất cả các phần còn lại thành 0
  while (partIndex < 4) {
    ipArray[partIndex++] = 0;
  }
}

bool WriteIPAddressToHMI(uint16_t startAddress, uint16_t count, uint16_t* inputArray) {
  // Kiểm tra xem inputArray có đủ phần tử không
  if (inputArray == nullptr) {
    Serial.println("Invalid inputArray pointer");
    return false;
  }

  // Ghi nhiều thanh ghi
  for (int i = 0; i < count; i++) {
    node.setTransmitBuffer(i, inputArray[i]); // Ghi giá trị từ inputArray vào thanh ghi
  }

  // In ra các giá trị của inputArray để kiểm tra
  Serial.print("Input Array: ");
  for (int i = 0; i < count; i++) {
    Serial.print(i + startAddress);
    Serial.print("-");
    Serial.print(inputArray[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Kiểm tra xem lệnh ghi có thành công không
  uint8_t result = node.writeMultipleRegisters(startAddress, count);

  // Kiểm tra kết quả của quá trình ghi
  if (result == node.ku8MBSuccess) {
    Serial.println("Write Modbus Successfully");
    return true;
  } else {
    Serial.print("Error: ");
    Serial.println(result);
    return false;
  }
}


bool WriteStatusToHMI(uint16_t startAddress, uint16_t count) {
  // Ghi nhiều thanh ghi
  for(int Index = 0; Index < count; Index++)
  {
    node.setTransmitBuffer(Index, StationStatus[Index]); // Ghi giá trị 0x1234 vào thanh ghi 0
  }
  // for(int i = 0; i < count; i++)
  // {
  //   Serial.print(IPNumberArray[i]);Serial.print(" ");
  // }
  // Serial.println(" ");
  // Kiểm tra xem lệnh ghi có thành công không
  uint8_t result = node.writeMultipleRegisters(startAddress, count);

  // Kiểm tra kết quả của quá trình ghi
  if (result == node.ku8MBSuccess) 
  {
    return true;
  } 
  else 
  {
    Serial.println(result);
    return false;
  }
  
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
