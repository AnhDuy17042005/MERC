#include <Arduino.h>
#include <QTRSensors.h>
#include <PS2X_lib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
// khai báo dò line
QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
float Kp = 0.6;
float Ki = 0.0;
float Kd = 5.0;
int P;
int I;
int D;
int lastError = 0;// sai số cuối cùng
//boolean onoff = false;// bật tắt tính năng dò line 

// Thêm 2 biến toàn cục ở đầu file
unsigned long autoModeStartTime = 0;
bool delayPassed = false;
//--------

// khai báo tốc độ
const uint8_t maxspeeda = 160;
const uint8_t maxspeedb = 160;
const uint8_t basespeeda = 80;
const uint8_t basespeedb = 80;

int mode = 24;// led trạng thái dò line
int buttoncalibrate = 25;//Nút thực hiện hiệu chỉnh cảm biến

PS2X ps2x;
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo servo1;

// Chân kết nối PS2 cho Arduino Mega
#define PS2_DAT 30
#define PS2_CMD 31
#define PS2_ATT 33
#define PS2_CLK 32

// Chân BTS7960 (bên trái)
#define L_EN 6 // ENA
#define L_RPWM 8  //IN2
#define L_LPWM 5 // IN1

// Chân BTS7960 (bên phải)
#define R_EN 10 // ENB
#define R_RPWM 12 //IN4
#define R_LPWM 9 // IN3

// Chân L298N cho động cơ nâng
#define IN1 4
#define IN2 3
#define ENA 2

// Servo
#define SERVO1_PIN 22

// Chế độ hoạt động
bool autoMode = false;
bool ps2Connected = false;
// 

void setup() {
    Serial.begin(115200);
    // lcd.init();
    // lcd.backlight();
    // lcd.setCursor(0, 0);
    // lcd.print("Sat That");
    // lcd.setCursor(0, 1);
    // lcd.print("Mode: Manual");

    qtr.setTypeAnalog(); 
    qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5, A6, A7}, SensorCount);
    qtr.setEmitterPin(13);//Khi bật LED emitter, cảm biến sẽ phát hiện ánh sáng phản xạ từ bề mặt (trắng hoặc đen) để xác định đường đi.

    int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT, false, true);
    ps2Connected = (error == 0);
    Serial.println(ps2Connected ? "Kết nối PS2 thành công!" : "Lỗi kết nối PS2");
    pinMode(PS2_ATT, OUTPUT);
    digitalWrite(PS2_ATT, HIGH);

    pinMode(mode, OUTPUT);
    pinMode(buttoncalibrate, INPUT_PULLUP);// khai báo nút nhấn
    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(L_EN, OUTPUT);
    pinMode(L_RPWM, OUTPUT);
    pinMode(L_LPWM, OUTPUT);
    pinMode(R_EN, OUTPUT);
    pinMode(R_RPWM, OUTPUT);
    pinMode(R_LPWM, OUTPUT);
    // l298 nâng hạ
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);

    digitalWrite(mode, LOW);

    servo1.attach(SERVO1_PIN);
    servo1.write(40);
    stopMotors();

  boolean Ok = false;
    while (Ok == false) {
      if(digitalRead(buttoncalibrate) == LOW)// Kiểm tra xem nút nhấn buttoncalibrate (được gán với chân số 17) có được nhấn hay không.
      {
        calibration();
        Ok = true;
        }
      }
    //stopMotors();
}

void loop() {

// // Đọc giá trị từ cảm biến QTR
//   qtr.read(sensorValues); 

//   // In giá trị từng cảm biến ra Serial Monitor
//   for (uint8_t i = 0; i < SensorCount; i++) {
//     Serial.print("Sensor ");
//     Serial.print(i);
//     Serial.print(": ");
//     Serial.println(sensorValues[i]); // In giá trị từng cảm biến
//   }
//   // Ngăn cách giữa các lần in
//   Serial.println("-----------------------");
//   delay(700); // Delay để dễ theo dõi
    
    if (ps2Connected == true) {
        if (!ps2x.read_gamepad(false, false)) {
            ps2Connected = false;
            Serial.println("Mất kết nối PS2!");
        } else {
        if (ps2x.ButtonPressed(PSB_START)) {
        autoMode = !autoMode;
        //....
            delayPassed = false; // Reset cờ delay khi chuyển chế độ
        if (autoMode) {
        autoModeStartTime = millis(); // Lưu thời điểm bắt đầu chế độ auto
        }
        //.....
        //lcd.setCursor(0, 1);
        //lcd.print(autoMode ? "Mode: Auto   " : "Mode: Manual");
        Serial.println(autoMode ? "Chế độ: Tự động" : "Chế độ: Điều khiển");
        //delay(500);
    }

    int joyLX = ps2x.Analog(PSS_LX) - 128;
    int joyLY = 128 - ps2x.Analog(PSS_LY);
    int joyRY = 128 - ps2x.Analog(PSS_RY);
    int joyRX = ps2x.Analog(PSS_RX) - 128;  // Trục X của joystick phải
    Serial.print("LX:"); Serial.print(joyLX);
    Serial.print(" LY:"); Serial.print(joyLY);
    Serial.print(" RX:"); Serial.print(joyRX);
    Serial.print(" RY:"); Serial.println(joyRY);

    if (!autoMode) {
        digitalWrite(mode, LOW);      
        controlMotors(joyLX, joyLY);
        controlLiftingMotor(joyRY);
        if (ps2x.Button(PSB_CIRCLE)) servo1.write(0);
        if (ps2x.Button(PSB_CROSS)) servo1.write(40);
        if (ps2x.Button(PSB_SQUARE)) servo1.write(20);

    } else {
        digitalWrite(mode, HIGH);
        // Kiểm tra thời gian delay
        if (!delayPassed) {
        // Dừng động cơ trong 3 giây đầu
            stopMotors();
        // Kiểm tra đã đủ 3 giây chưa
            if (millis() - autoModeStartTime >= 3000) {
                delayPassed = true;
        }
    } 
        else {
        // Chạy PID sau khi đã qua 3 giây
            PID_control();
        }
}
        }
    } else {
        static unsigned long lastAttempt = 0;
        if (millis() - lastAttempt > 1500) {
            ps2Connected = reconnect();
            lastAttempt = millis();
        }
    }
}
bool reconnect() {
    digitalWrite(PS2_ATT, LOW);
    delay(50);
    digitalWrite(PS2_ATT, HIGH);
    delay(50);
    return ps2x.read_gamepad(false, false);
}

void controlMotors(int lx, int ly) {
    // int speed = map(abs(ly), 0, 128, 0, 200);
    // int turnSpeed = map(abs(lx), 0, 128, 0, 160);

    // if (ly > 10) {
    //     moveForward(speed);
    // } else if (ly < -10) {
    //     moveBackward(speed);
    // } else if (lx > 10) {
    //     turnRight(turnSpeed);
    // } else if (lx < -10) {
    //     turnLeft(turnSpeed);
    // } else {
    //     stopMotors();
    // }
    const int moveSpeed = 200;    // Tốc độ tiến/lùi
    const int turnSpeed = 150;    // Tốc độ rẽ
    
    // Đọc trạng thái các nút
    bool up = ps2x.Button(PSB_PAD_UP);
    bool down = ps2x.Button(PSB_PAD_DOWN);
    bool left = ps2x.Button(PSB_PAD_LEFT);
    bool right = ps2x.Button(PSB_PAD_RIGHT);

    // Xử lý tổ hợp phím
    if (up && !down) {
        if (left)       turnLeft(turnSpeed);   // Tiến + Rẽ trái
        else if (right) turnRight(turnSpeed);  // Tiến + Rẽ phải
        else            moveForward(moveSpeed); // Tiến thẳng
    } 
    else if (down && !up) {
        if (left)       turnLeft(turnSpeed);   // Lùi + Rẽ trái
        else if (right) turnRight(turnSpeed);  // Lùi + Rẽ phải
        else            moveBackward(moveSpeed); // Lùi thẳng
            } 
    else if (left)      turnLeft(turnSpeed);   // Rẽ trái tại chỗ
    else if (right)     turnRight(turnSpeed);  // Rẽ phải tại chỗ
    else                stopMotors();          // Dừng
}

void moveForward(int speed) {
    analogWrite(L_EN, speed);
    digitalWrite(L_RPWM, LOW);// IN2
    digitalWrite(L_LPWM, HIGH);//IN1
    analogWrite(R_EN, speed);
    digitalWrite(R_RPWM, LOW);// IN4
    digitalWrite(R_LPWM, HIGH);//IN3
    //Serial.println("Xe tiến lên");
}

void moveBackward(int speed) {
    analogWrite(L_EN, speed);
    digitalWrite(L_RPWM, HIGH);
    digitalWrite(L_LPWM, LOW);
    analogWrite(R_EN, speed);
    digitalWrite(R_RPWM, HIGH);
    digitalWrite(R_LPWM, LOW);
    //Serial.println("Xe lùi");
}

void turnRight(int speed) {
    analogWrite(L_EN, speed);
    digitalWrite(L_RPWM, LOW);
    digitalWrite(L_LPWM, HIGH);
    analogWrite(R_EN, speed);
   digitalWrite(R_RPWM, HIGH);
    digitalWrite(R_LPWM, LOW);
    //Serial.println("Xe rẽ phải");
}

void turnLeft(int speed) {
    analogWrite(L_EN, speed);
    digitalWrite(L_RPWM, HIGH);
    digitalWrite(L_LPWM, LOW);
    analogWrite(R_EN, speed);
    digitalWrite(R_RPWM, LOW);
    digitalWrite(R_LPWM, HIGH);
    //Serial.println("Xe rẽ trái");
}

void stopMotors() {
    analogWrite(L_EN, 0);
    analogWrite(R_EN, 0);
    digitalWrite(L_RPWM, LOW);
    digitalWrite(L_LPWM, LOW);
    digitalWrite(R_RPWM, LOW);
    digitalWrite(R_LPWM, LOW);
    //Serial.println("Xe dừng");
}

void controlLiftingMotor(int ry) {
    int speed = map(abs(ry), 0, 128, 0, 255);
    if (ry > 20) {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        analogWrite(ENA, speed);
        //Serial.println("Nâng tay gắp");
    } else if (ry < -20) {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, speed);
        //Serial.println("Hạ tay gắp");
    } else {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
    }
}

void PID_control() {
    qtr.read(sensorValues);;
    int input_black = 910;
    int input_white = 909;
    int count_black = 0;
    int count_white = 0;
    for (uint8_t i = 0; i < SensorCount; i++){
      if (sensorValues[i] >= input_black){
        count_black++;
      }
      if (sensorValues[i] <= input_white){
        count_white++;
      }
    }
    if (count_black >= 4){
      PID();
    }
    else if(count_white == 8){
          emergencyStop(); 
          //Serial.println("Lost line, stopping...");
          return;
    }
    else{
      PID();
    }
}
int filteredPosition() {
    static int prevPosition = 3500;
    uint16_t position = qtr.readLineBlack(sensorValues);
    int filtered = (prevPosition * 0.75) + (position * 0.25);
    prevPosition = filtered;
    return filtered;
}

void PID() {
    int position = filteredPosition(); // Sử dụng giá trị đã lọc
    int error =  3500 - position;
    P = error;
    I = I + error;
    I = constrain(I, -1000, 1000);
    D = error - lastError;
    lastError = error;
    int motorspeed = P*Kp + I*Ki + D*Kd;
 
    int motorspeeda = constrain((basespeeda + motorspeed), 0, maxspeeda);
    int motorspeedb = constrain((basespeedb - motorspeed), 0, maxspeedb);
    Forward(motorspeeda, motorspeedb);

    // Serial.print("Error: "); Serial.println(error);
    // Serial.print("P: "); Serial.println(P*Kp);
    // Serial.print("I: "); Serial.println(I*Ki);
    // Serial.print("D: "); Serial.println(D*Kd);
    // // In giá trị debug
    // Serial.print("Motorspeed: "); Serial.println(motorspeed);
    // Serial.print("Error: "); Serial.println(error);
    // Serial.print("Motor A Speed: "); Serial.println(motorspeeda);
    // Serial.print("Motor B Speed: "); Serial.println(motorspeedb);
    // Serial.println("--------------------------------");
    // delay(10);
}

void Forward(int L_speed, int R_speed) {
    analogWrite(L_EN, L_speed);
    digitalWrite(L_RPWM, HIGH);// IN2
    digitalWrite(L_LPWM, LOW);//IN1
    analogWrite(R_EN, R_speed);
    digitalWrite(R_RPWM, HIGH);// IN4
    digitalWrite(R_LPWM, LOW);//IN3
}

// Hàm đọc cảm biến đầu vào
void calibration() {
 digitalWrite(LED_BUILTIN, HIGH);
 for (uint16_t i = 0; i < 200; i++)
 {
  qtr.calibrate();// Gọi hàm hiệu chỉnh của thư viện QTR
 }
 digitalWrite(LED_BUILTIN, LOW);
}

void emergencyStop() {
    analogWrite(L_EN, 255);
    digitalWrite(L_RPWM, HIGH);
    digitalWrite(L_LPWM, HIGH);
    
    analogWrite(R_EN, 255);
    digitalWrite(R_RPWM, HIGH);
    digitalWrite(R_LPWM, HIGH);
    
    delay(30);
    stopMotors();
}
