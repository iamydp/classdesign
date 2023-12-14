#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD初始化
LiquidCrystal_I2C lcd(0x27, 20, 4);

// 定义GPIO引脚
const int startButtonPin = 0;  // 开始抢答/复位按钮
const int judgeButtonPin1 = 13; // "正确"按钮
const int judgeButtonPin2 = 14; // "错误"按钮
const int buzzerPin = 12;       // 蜂鸣器引脚
const int groupButtons[6] = {15, 16, 17, 18, 19, 4}; // 小组按钮引脚

// 定义参数
const int countdownTime = 30; // 倒计时时间（秒）
bool countdownActive = false;
bool waitingForJudge = false; // 标记是否等待主持人判断
int currentGroup = -1;
unsigned long countdownStartTime = 0;

// 小组信息
struct Group {
  String name;
  int score;
};

// 定义小组数组
Group groups[6] = {{"A", 100}, {"B", 100}, {"C", 100}, {"D", 100}, {"E", 100}, {"F", 100}};

void setup() {
  // 初始化LCD
  lcd.begin(20, 4);
  lcd.init();
  lcd.backlight();
  
  // 初始化按钮引脚
  pinMode(startButtonPin, INPUT_PULLUP); // 使用内部上拉电阻
  pinMode(judgeButtonPin1, INPUT_PULLUP);
  pinMode(judgeButtonPin2, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  for (int i = 0; i < 6; i++) {
    pinMode(groupButtons[i], INPUT_PULLUP);
  }
}

void loop() {
  // 检测开始抢答/复位按钮
  if (digitalRead(startButtonPin) == LOW) {
    resetSystem();
    startCountdown();
  }

  // 检测小组抢答按钮
  for (int i = 0; i < 6; i++) {
    if (digitalRead(groupButtons[i]) == LOW && !waitingForJudge) {
      handleGroupButtonPress(i);
    }
  }

  // 检测判断按钮
  if (digitalRead(judgeButtonPin1) == LOW) {
    handleJudgeButtonPress(true);  // 传入true表示"正确"按钮按下
  }

  if (digitalRead(judgeButtonPin2) == LOW) {
    handleJudgeButtonPress(false); // 传入false表示"错误"按钮按下
  }

  // 更新LCD显示
  updateLCD();
}

void resetSystem() {
  countdownActive = false;
  waitingForJudge = false;
  currentGroup = -1;
}

void startCountdown() {
  countdownActive = true;
  waitingForJudge = false;
  countdownStartTime = millis();
}

void handleGroupButtonPress(int group) {
  if (currentGroup == -1) {
    currentGroup = group;
  } else {
    // 处理犯规
    flashGroupOnLCD(currentGroup);
    currentGroup = -1;
    countdownActive = false;
    // 触发蜂鸣器
    digitalWrite(buzzerPin, HIGH);
    delay(500); // 蜂鸣器响一段时间
    digitalWrite(buzzerPin, LOW);
  }
}

void handleJudgeButtonPress(bool correct) {
  if (countdownActive && currentGroup != -1) {
    // 判断按钮按下，更新小组分数
    if (correct) {
      groups[currentGroup].score += 10; // "正确"按钮按下，加十分
    } else {
      groups[currentGroup].score -= 10; // "错误"按钮按下，减十分
      groups[currentGroup].score = max(0, groups[currentGroup].score); // 分数不能小于零
    }
    // 处理完成后，重置状态
    countdownActive = false;
    waitingForJudge = false;
    currentGroup = -1;
  }
}

void updateLCD() {
  lcd.clear();
  
  // 显示倒计时
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  int elapsedTime = (millis() - countdownStartTime) / 1000;
  int remainingTime = countdownActive ? max(0, countdownTime - elapsedTime) : 0;
  lcd.print(remainingTime);
  lcd.print("s");

  // 显示小组信息
  for (int i = 0; i < min(4, 6); i++) {
    lcd.setCursor(0, i + 1);
    lcd.print("GP");
    lcd.print(groups[i].name);
    lcd.print(": ");
    lcd.print(groups[i].score);
  }

  delay(1000); // 更新间隔，可以根据需要调整
}

void flashGroupOnLCD(int group) {
  // 在LCD上闪烁显示犯规的小组名
  for (int i = 0; i < 5; i++) {
    lcd.setCursor(3, min(group, 3) + 1);
    lcd.print("        "); // 清除小组信息
    delay(200);
    lcd.setCursor(3, min(group, 3) + 1);
    lcd.print("GP");
    lcd.print(groups[group].name);
    lcd.print(": ");
    lcd.print(groups[group].score);
    delay(200);
  }
}
