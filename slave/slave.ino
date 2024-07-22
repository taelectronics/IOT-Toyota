#include <ModbusMaster.h>
#include <HardwareSerial.h>

// Khai báo các chân UART
#define RS485_RXD2 16  // GPIO16, UART2 RXD
#define RS485_TXD2 17  // GPIO17, UART2 TXD
#define RS485_TX_ENABLE 18  // GPIO18, UART2 TX enable

// Khai báo đối tượng ModbusMaster
ModbusMaster node;

HardwareSerial SerialModbus(2);

// ID của thiết bị Modbus Slave
#define SLAVE_ID 1

void setup() {
  // Khởi động Serial monitor
  Serial.begin(115200);

  // Khởi động UART2 cho Modbus
  SerialModbus.begin(9600, SERIAL_8N1, RS485_RXD2, RS485_TXD2);
  pinMode(RS485_TX_ENABLE, OUTPUT);

  // Khởi động Modbus master
  node.begin(SLAVE_ID, SerialModbus);

  Serial.println("Modbus Master Initialized");
}

void loop() {
  uint16_t result;
  uint16_t data[10];

  for (uint16_t reg = 0; reg <= 500; reg += 10) {
    // Đọc các thanh ghi
    result = node.readHoldingRegisters(reg, 10);

    if (result == node.ku8MBSuccess) {
      Serial.print("Register ");
      Serial.print(reg);
      Serial.print(" to ");
      Serial.print(reg + 9);
      Serial.print(": ");

      // In giá trị của các thanh ghi ra màn hình
      for (uint8_t i = 0; i < 10; i++) {
        data[i] = node.getResponseBuffer(i);
        Serial.print(data[i]);
        if (i < 9) Serial.print(", ");
      }
      Serial.println();
    } else {
      Serial.print("Failed to read registers at ");
      Serial.println(reg);
    }

    delay(1000); // Đợi 1 giây trước khi đọc tiếp
  }

  delay(5000); // Đợi 5 giây trước khi lặp lại toàn bộ quá trình
}
