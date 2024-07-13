// ask_transmitter.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to transmit messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) transmitter with an TX-C1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12
// GỬI
#include <RH_ASK.h>
#include <Arduino.h>
RH_ASK driver(2000, 9, 8, 0); // tốc độ - nhận - gửi
// RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85), 
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
    {
      Serial.println("init failed");
    }
}

void loop()
{
    const char *msg = "hello";

    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
    delay(200);
}
