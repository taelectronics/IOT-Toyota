

void setup()
{
    // Khởi tạo Serial để in thông báo
    Serial.begin(9600);
        Serial.println("không thể khởi tạo RF");
        Serial.println("Khởi tạo RF thành công");
}

void loop()
{
  Serial.println("ok");
}
