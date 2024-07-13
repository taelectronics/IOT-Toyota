#include <Arduino.h>
#include <HardwareSerial.h>
HardwareSerial RFTX(1);

void setup() {
    // Initialization code
    Serial.begin(115200);
    RFTX.begin(2400,SERIAL_8N1,12,13);
}

void loop() {
    RFTX.flush();
    RFTX.println("OKOK");
    delay(10);
}
