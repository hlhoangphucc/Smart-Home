#include <ESP32Servo.h>
#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SocketIOclient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 0
#define SS_PIN 5

int UID[4], i;

int ID1[4] = { 78, 36, 00, 125 };  //Thẻ bật tắt đèn

MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2);

SocketIOclient socketIO;

#define USE_SERIAL Serial

const char* ssid = "OPPO A3s";
const char* pass = "12345678900";

//////////////////// Khai Báo Các Chân Của Thiết Bị //////////////////////////
int buttonBedroom = 25;
int buttonKitroom = 26;
int buttonToiletroom = 27;
int buttonGarage = 12;
int buttonGate = 13;
int buttonMainDoor = 14;
int led = 32;
int DoorLeft = 2;
int DoorRight = 4;
// int buttonFan = 15;
// 0986641241
#define LaserPin 32

#define RainPin 39

#define LDRPin 34

#define THERMISTORPIN 36
// resistance of termistor at 25 degrees C
#define THERMISTORNOMINAL 10000
#define TEMPERATURENOMINAL 25
// Accuracy - Higher number is bigger
#define NUMSAMPLES 10
// Beta coefficient from datasheet
#define BCOEFFICIENT 3950
// the value of the R1 resistor
#define SERIESRESISTOR 10000
//prepare pole
uint16_t samples[NUMSAMPLES];

Servo ServoLeft;
Servo ServoRight;
//////////////////// Khai Báo Trạng Thái Của Thiết Bị //////////////////////////
const String token = "ExponentPushToken[osKFkDKC2aGcyjxlSdNYnG]";
const String title = "JiDuy Smart Home";
const String body = "A thief broke into your house";

int buttonBedstate = 0;
int buttonKitstate = 0;
int buttonToiletstate = 0;
int buttonGatestate = 0;
int buttonGaragestate = 0;
int buttonMaindoorstate = 0;

int lastButtonBedstate = 0;
int lastButtonKitstate = 0;
int lastButtonToiletstate = 0;
int lastButtonGatestate = 0;
int lastButtonGaragestate = 0;
int lastbuttonMaindoorstate = 0;

bool statusBedLed = false;
bool statusKitLed = false;
bool statusToiletLed = false;
int statusGate = 0;
int statusGarage = 0;
int statusMaindoor = 0;

int stop1 = 0;
int stop2 = 0;

int temp = 0;

unsigned long previousMillis = 0;
const long interval = 5000;

int ValueSoil, Soilanalog;

String homeId = "64a3de2814b0c81928b21d97";
///////////////////// Xứ Lý Sự Kiện Được Lấy Về Trên Server ///////////////////
void messageHandler(uint8_t* payload) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  String EventName = doc[0];

  Serial.println(EventName);

  if (EventName == "buttonState") {
    int pinEsp = doc[1]["pinEsp"];
    bool status = doc[1]["status"];
    if (pinEsp == 2) {
      statusBedLed = status;
    }
    if (pinEsp == 3) {
      statusKitLed = status;
    }
    if (pinEsp == 12) {
      statusToiletLed = status;
    }
    if (pinEsp == 7) {
      statusGarage = status;
    }
    if (pinEsp == 8) {
      statusGate = status;
    }
  }
}
//////////////// Xử Lý Trạng Thái Của SocketIO //////////////////////

void socketIOEvent(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      digitalWrite(led, LOW);
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
      digitalWrite(led, HIGH);

      break;
    case sIOtype_EVENT:
      // USE_SERIAL.printf("[IOc] Get Event: %s\n", payload);
      messageHandler(payload);
      // controlled_RGB(payload);
      break;
  }
}
bool listOnOff[] = {
  false,  //BEDROOM
  false,  //KITCHEN
  false,  //TOILET
  false,  // GATE
  false,  // GARAGE
};

void setup() {
  Wire.begin(21, 22);
  SPI.begin();
  mfrc522.PCD_Init();
  USE_SERIAL.begin(115200);

  searchWiFi();
  connectWiFi();

  SensorNTC();
  // server address, port and URL
  socketIO.beginSSL("server-smart-home.onrender.com", 443, "/socket.io/?EIO=4");
  // socketIO.begin("192.168.43.142", 3001, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);

  pinMode(buttonKitroom, INPUT);
  pinMode(buttonBedroom, INPUT);
  pinMode(buttonToiletroom, INPUT);
  pinMode(buttonGate, INPUT);
  pinMode(buttonGarage, INPUT);
  pinMode(RainPin, INPUT);
  pinMode(LaserPin, INPUT);
  pinMode(buttonMainDoor, INPUT);

  ServoLeft.attach(DoorLeft);
  ServoRight.attach(DoorRight);

  ServoLeft.write(180);
  ServoRight.write(90);

  lcd.init();                     //Khởi tạo LCD
  lcd.clear();                    //Xóa màn hình
  lcd.backlight();                //Bật đèn nền
  lcd.setCursor(2, 0);            //Đặt vị trí ở ô thứ 3 trên dòng 1
  lcd.print("Welcom to");         //Ghi đoạn text "Welcom to"
  lcd.setCursor(0, 1);            //Đặt vị trí ở ô thứ 1 trên dòng 2
  lcd.print("E-Smart Channel!");  //Ghi. đoạn text "E-smart Channel!"
}

void loop() {
  socketIO.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    lcd.clear();
    SensorNTC();
    RainSersor();
    SoilSensor();
  }
  // LazerSensor();
  RFID();
  ControlBedRoom();
  ControlKitRoom();
  ControlToiletRoom();
  ControlGate();
  ControlGarage();
  ControlMainDoor();
  controllArduinoTwo();
}
////////////////////////// Xử Lý Gửi Dữ Liệu Lên Server ////////////////////////
void senddata(String homeid, bool status, int pin) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  array.add("buttonState");

  JsonObject param = array.createNestedObject();
  param["homeId"] = homeid;
  param["status"] = status;
  param["pinEsp"] = pin;

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);
}

void ControlBedRoom() {
  int buttonBedstate = digitalRead(buttonBedroom);

  if (buttonBedstate != lastButtonBedstate && buttonBedstate) {
    statusBedLed = !statusBedLed;
    senddata(homeId, statusBedLed, 12);
    Serial.println(statusBedLed);
  }
  lastButtonBedstate = buttonBedstate;
  listOnOff[0] = statusBedLed;
}

void ControlKitRoom() {
  buttonKitstate = digitalRead(buttonKitroom);

  if (buttonKitstate != lastButtonKitstate && buttonKitstate) {
    statusKitLed = !statusKitLed;
    senddata(homeId, statusKitLed, buttonKitroom);
    Serial.println(statusKitLed);
  }
  lastButtonKitstate = buttonKitstate;
  listOnOff[1] = statusKitLed;
}
void ControlToiletRoom() {
  buttonToiletstate = digitalRead(buttonToiletroom);

  if (buttonToiletstate != lastButtonToiletstate && buttonToiletstate) {
    statusToiletLed = !statusToiletLed;
    senddata("789", statusToiletLed, buttonToiletroom);
    Serial.println(statusToiletLed);
  }
  lastButtonToiletstate = buttonToiletstate;
  listOnOff[2] = statusToiletLed;
}


void ControlGate() {
  buttonGatestate = digitalRead(buttonGate);

  if (buttonGatestate != lastButtonGatestate && buttonGatestate) {
    statusGate = !statusGate;
    senddata("Gate", statusGate, buttonGate);
    Serial.println(statusGate);
  }
  lastButtonGatestate = buttonGatestate;
  listOnOff[3] = statusGate;
}

void ControlGarage() {
  buttonGaragestate = digitalRead(buttonGarage);

  if (buttonGaragestate != lastButtonGaragestate && buttonGaragestate) {
    statusGarage = !statusGarage;
    senddata("Garage", statusGarage, buttonGarage);
    Serial.println(statusGarage);
  }
  lastButtonGaragestate = buttonGaragestate;
  listOnOff[4] = statusGarage;
}

void ControlMainDoor() {
  buttonMaindoorstate = digitalRead(buttonMainDoor);

  if (buttonMaindoorstate != lastbuttonMaindoorstate && buttonMaindoorstate) {
    statusMaindoor = !statusMaindoor;
    if (statusMaindoor == true) {
      ServoLeft.write(90);
      ServoRight.write(180);
      // Serial.println(ServoRight.read());
    } else {
      ServoLeft.write(180);
      ServoRight.write(90);
      // Serial.println(ServoRight.read());
    }
    senddata("MainDoor", statusMaindoor, buttonMainDoor);
    Serial.println(statusMaindoor);
  }
  lastbuttonMaindoorstate = buttonMaindoorstate;
}

void SensorNTC() {
  // Serial.println(analogRead(THERMISTORPIN));
  uint8_t i;
  float average;

  // lưu giá trị input
  for (i = 0; i < NUMSAMPLES; i++) {
    samples[i] = analogRead(THERMISTORPIN);
    delay(10);
  }
  //tính giá trị trung bình input
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  //đổi sang điện trở
  average = 4095 / average - 1;
  average = SERIESRESISTOR / average;

  //đổi từ điện trở sang nhiệt độ
  float temperature;
  temperature = average / THERMISTORNOMINAL;
  temperature = log(temperature);
  temperature /= BCOEFFICIENT;
  temperature += 1.0 / (TEMPERATURENOMINAL + 273.15);
  temperature = 1.0 / temperature;
  temperature -= 273.15;  // đổi sang nhiệt độ
  temp = temperature + 14;
  // Hiện thị nhiệt độ lên lcd
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  lcd.print(temp);
}

void SoilSensor() {

  Soilanalog = analogRead(LDRPin);
  ValueSoil = (100 - ((Soilanalog / 4095.00) * 100));
  lcd.setCursor(6, 0);
  lcd.print("||");
  lcd.setCursor(9, 0);
  lcd.print("M: ");
  lcd.print(ValueSoil);
  lcd.print("%");
}

// void LazerSensor() {
//   bool value = digitalRead(LaserPin);
//   if (!value) {
//     sendPushNotification(title, body, token);
//     senddata(homeId, title, body);
//   }
// }

void RFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("UID của thẻ: ");

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    UID[i] = mfrc522.uid.uidByte[i];
    Serial.print(UID[i]);
  }

  Serial.println("   ");

  if (UID[i] == ID1[i]) {
    statusMaindoor = true;
    ServoLeft.write(90);
    ServoRight.write(180);
  }

  else {
    statusMaindoor = false;
    ServoLeft.write(180);
    ServoRight.write(90);
    Serial.println("SAI THẺ........");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
void RainSersor() {
  bool value = digitalRead(RainPin);
  String status = "";
  // Serial.println(value);
  if (value == false) {
    // Serial.println("MUA ");
    status = "Rain";
  } else {
    // Serial.println("NANG");
    status = "Sun";
  }
  lcd.setCursor(0, 1);
  lcd.print("W: ");
  lcd.print(status);
}
// void sendPushNotification(String title, String message, String token) {
//   HTTPClient http;

//   // Tạo URL API gửi thông báo
//   String url = "https://api.expo.dev/v2/push/send";

//   // Tạo JSON data cho thông báo
//   String jsonData = "{";
//   jsonData += "\"to\": \"" + token + "\",";
//   jsonData += "\"title\": \"" + title + "\",";
//   jsonData += "\"body\": \"" + message + "\",";
//   jsonData += "\"priority\": \"high\",";
//   jsonData += "\"sound\": \"default\"";
//   jsonData += "}";

//   // Gửi yêu cầu POST đến API của ứng dụng của bạn
//   http.begin(url);
//   http.addHeader("Content-Type", "application/json");
//   int httpResponseCode = http.POST(jsonData);

//   // Xử lý kết quả trả về từ API
//   if (httpResponseCode > 0) {
//     Serial.printf("Notification sent (HTTP status code: %d)\n", httpResponseCode);
//     String response = http.getString();
//     Serial.println(response);
//   } else {
//     Serial.printf("Error sending notification (HTTP status code: %d)\n", httpResponseCode);
//   }

//   http.end();
// }
void controllArduinoTwo() {
  int t = 0;
  t = listOnOff[0] * 10000 + listOnOff[1] * 1000 + listOnOff[2] * 100 + listOnOff[3] * 10 + listOnOff[4];
  int ControllAduinoTwo = t < 10000 ? t + 20000 : t;
  // Serial.println(ControllAduinoTwo);
  Wire.beginTransmission(4);
  char num[5];
  itoa(ControllAduinoTwo, num, 10);
  Wire.write((const uint8_t*)num, 5);  // ép kiểu con trỏ num về kiểu const uint8_t*
  Wire.endTransmission();
}



/////////////////////////// Tìm Kiếm Và Kết Nối WIFI ////////////////////////
void searchWiFi() {
  int numberOfNetwork = WiFi.scanNetworks();
  USE_SERIAL.println("----");

  for (int i = 0; i < numberOfNetwork; i++) {
    USE_SERIAL.print("Network name: ");
    USE_SERIAL.println(WiFi.SSID(i));
    USE_SERIAL.print("Signal strength: ");
    USE_SERIAL.println(WiFi.RSSI(i));
    USE_SERIAL.println("--------------");
  }
}
void connectWiFi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    USE_SERIAL.print(".");
    delay(1000);
  }

  USE_SERIAL.print("");
  USE_SERIAL.println("WiFi connected");
  USE_SERIAL.print("IP Address : ");
  USE_SERIAL.println(WiFi.localIP());
}