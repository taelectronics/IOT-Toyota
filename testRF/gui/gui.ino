#include <RH_ASK.h>
#include <SPI.h> // Không sử dụng, nhưng bắt buộc để RadioHead hoạt động

// Khởi tạo đối tượng driver với chân gửi dữ liệu là chân 4
RH_ASK driver(2000, 5, 4); // Baud rate 2000, chân nhận 12, chân gửi 4, chân PTT 2

void setup()
{
    // Khởi tạo Serial để in thông báo
    Serial.begin(9600);

    // Khởi tạo driver
    if (!driver.init())
        Serial.println("không thể khởi tạo RF");
    else
        Serial.println("Khởi tạo RF thành công");
}

void loop()
{
    const char *msg = "Hello, RF!"; // Dữ liệu cần gửi
    driver.send((uint8_t *)msg, strlen(msg)); // Gửi dữ liệu
    driver.waitPacketSent(); // Đợi cho đến khi dữ liệu được gửi xong
    Serial.println("ok");
    delay(1000); // Đợi 1 giây trước khi gửi lại
}
