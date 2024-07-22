// ESP8266 433MHz transmitter test 1
#include <RH_ASK.h>  
#include <SPI.h>     

// 21 nhận 22 gửi

RH_ASK rf_driver(2000, 21, 22);  


void setup() {
  Serial.begin(115200);
  delay(4000);
  Serial.println("ESP8266 433MHz transmitter");
  if (!rf_driver.init()) {
    Serial.println("init failed");
    while (1) delay(10000);
  }
  Serial.println("Transmitter: rf_driver initialised");
}

void loop() {
  Serial.println("Transmitting packet");
  const char *msg = "Location:X=10.0,Y=20.0";
  rf_driver.send((uint8_t *)msg, strlen(msg)+1);
  rf_driver.waitPacketSent();
  delay(1000);
}
