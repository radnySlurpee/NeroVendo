#include <Servo.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>

#define SERVOMIN 150
#define SERVOMAX 600

char key;
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {22, 24, 26, 28};
byte colPins[COLS] = {30, 32, 34, 36}; // keypad pins
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
volatile int bills = 0, checkbills = 0; const byte interruptPinBills = 2;
volatile int coins = 0, checkcoins = 0; const byte interruptPinCoins = 3;
int totalBalance=0;
LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
String lcdRow1="", lcdRow2="";
String entryCode = "";
int slot=-1;
int arrayPrice[80];


void setup()
{ // initialize Serial for testing
  Serial.begin(9600);
  pinMode(interruptPinCoins, INPUT_PULLUP);
  pinMode(interruptPinBills, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinCoins), CurrencyAcceptor, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptPinBills), CurrencyAcceptor, RISING);
  detachInterrupt(digitalPinToInterrupt(interruptPinCoins));
  detachInterrupt(digitalPinToInterrupt(interruptPinBills));
  lcd.begin(20, 4);
  lcd.init();
  lcd.backlight();
  totalBalance = EEPROM.read(0);
  for (int ctr = 0; ctr <= 80; ctr++)
  {
    arrayPrice[ctr] = EEPROM.read(ctr+1);
    EEPROM.write(ctr, 0);
  }

  for (int ctr = 0; ctr <= 81; ctr++)
  {
    EEPROM.write(ctr, 0);
  }

  pwm.begin();
  pwm.setPWMFreq(60);
  delay(2000);
  Serial.println("SYSTEM ONLINE");
}

void LCD_display_customer(){
  if (totalBalance <= 0){
    lcdRow1 = "Please insert a";
    lcdRow2 = "coin to continue";
  }else{
    lcdRow1 = "PHP: " + (String)totalBalance;
    lcdRow2 = "> " + entryCode + " <";
  }
  lcd.setCursor(0, 0);
  lcd.print(lcdRow1);
  lcd.setCursor(0, 1);
  lcd.print(lcdRow2);
}

void CurrencyAcceptor()
{
  if (digitalRead(interruptPinCoins) == LOW)
  {
    checkcoins++;
  }

  if (digitalRead(interruptPinBills) == LOW)
  {
    checkbills++;
  }
}

void CurrencyChecker(int minCount, int maxCount, int given,String currencyType)
{
  if (currencyType == "bill"){
    if ((checkbills >= minCount) && (checkbills <= maxCount))
    {
      bills = given;
      totalBalance += bills;
      bills = 0;
      checkbills = 0;
      lcd.clear();
    }
  }
  else if (currencyType == "coin"){
    if ((checkcoins >= minCount) && (checkcoins <= maxCount))
    {
      coins = given;
      totalBalance += coins;
      coins = 0;
      checkcoins = 0;
      lcd.clear();
    }
  }
}

void Keypad_control(){
  key = keypad.getKey();
    if (key) {
      Serial.print("KEY : ");
      Serial.println(key);
      lcd.clear();
      if (entryCode.length() <= 1)
      {
        entryCode += key;
      }
    }else{
      key = key;
    }
}

void control_function(){
  switch (key)
  {
    case '1': purchase_function(15);
              servoSpin();
              break;
    case '2': purchase_function(25);
              //servoSpin(1);
              break;
    case '3': totalBalance++;
              EEPROM.write(0, totalBalance);
              break;
    case '#': entryCode = ""; lcd.clear();break;
  default:
    break;
  }
  key="";
}

void purchase_function(int cost){
  int totalBalanceTemp = totalBalance;
  totalBalanceTemp -= cost;
  if (totalBalanceTemp >= 0){
    totalBalance -= cost;
    //LCD_display(1, 1, "cost: " + (String)cost);
  }else{
    totalBalance = totalBalance;
    //LCD_display(1, 1, "Not Enough BAL");
  }
}

void servoSpin(){
  slot++;
   for (int degree = 0; degree <= 545; degree++) //adjust rotation here
   {
     pwm.setPWM(slot, 0, map(90, 0, 180, SERVOMIN, SERVOMAX));
     delayMicroseconds(1000);
   }
  pwm.setPWM(slot, 0, 0); //stop
}

void RunCodeInMillis()
{
  static unsigned long timer = millis();
  static int deciSeconds1 = 0, deciSeconds2 = 0;
  
  if (millis() - timer >= 100)
  {
    timer += 50;
    CurrencyAcceptor();
    if (deciSeconds1 >= checkbills * 1.35)
    {
      CurrencyChecker(16, 21, 20, "bill");
      CurrencyChecker(46, 51, 50, "bill");
      CurrencyChecker(96, 101, 100, "bill");
      CurrencyChecker(196, 201, 200, "bill");
      CurrencyChecker(496, 501, 500, "bill");
      CurrencyChecker(996, 1001, 1000, "bill");
      deciSeconds1 = 0;
    }
    if (deciSeconds2 >= checkcoins * 3)
    {
      CurrencyChecker(1, 2, 1, "coin");
      CurrencyChecker(3, 6, 5, "coin");
      CurrencyChecker(8, 11, 10, "coin");
      deciSeconds2 = 0;
    }

    deciSeconds1++;
    deciSeconds2++;
    Keypad_control();
    control_function();
    LCD_display_customer();

  }
}

void loop()
{
  RunCodeInMillis();
}
