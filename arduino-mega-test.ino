#include <Servo.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

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
int totalBalance = 0;
LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
String lcdRow1="", lcdRow2="";
String entryCode = "", thisEntryCode="";
const char *itemDataSAVED;
bool isPurchased = false;
bool caseRunOnce = false;
bool isLCD_refreshed = false;

void setup()
{ // initialize Serial for testing
  Serial.begin(9600);
  pinMode(interruptPinCoins, INPUT_PULLUP);
  pinMode(interruptPinBills, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinCoins), CurrencyAcceptor, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptPinBills), CurrencyAcceptor, RISING);
  detachInterrupt(digitalPinToInterrupt(interruptPinCoins));
  detachInterrupt(digitalPinToInterrupt(interruptPinBills));
  totalBalance = 100;
                          //Json Syntax : A1 [("price"),("quantity"),("servo")] 
  const char* itemData ="{\"A1\":[15,5,0],\"A2\":[0,0,1],\"A3\":[0,0,2],\"A4\":[0,0,3],\"A5\":[0,0,4],\"A6\":[0,0,5],\"A7\":[0,0,6],"
                         "\"B1\":[0,0,7],\"B2\":[0,0,8],\"B3\":[0,0,9],\"B4\":[0,0,10],\"B5\":[0,0,11],\"B6\":[0,0,12],\"B7\":[0,0,13],"
                         "\"C1\":[0,0,14],\"C2\":[0,0,15],\"C3\":[0,0,16],\"C4\":[0,0,17],\"C5\":[0,0,18],\"C6\":[0,0,19],\"C7\":[0,0,20],"
                         "\"D1\":[0,0,21],\"D2\":[0,0,22],\"D3\":[0,0,23],\"D4\":[0,0,24],\"D5\":[2,0,25],\"D6\":[0,0,26],\"D7\":[77,7,27]}";

  EEPROM.put(1, itemData);
  EEPROM.get(1, itemDataSAVED);
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, itemDataSAVED);
  
  if (err)
  {
    Serial.println("ERROR: ");
    Serial.println(err.c_str());
    return;
  }

  //int ItemPrice = doc["D7"][0];
  //int ItemQuantity = doc["D7"][1];
  //int ItemServo = doc["D7"][2];

  Serial.println("ITEMDATA: " + (String)itemDataSAVED);

  lcd.begin(20, 4);
  lcd.init();
  lcd.backlight();
  pwm.begin();
  pwm.setPWMFreq(60);
  delay(2000);
  Serial.println("SYSTEM ONLINE");
}

void LCD_display(){
  if (totalBalance <= 0){
    lcdRow1 = "Please insert a";
    lcdRow2 = "coin to continue";
  }else if(lcdRow1 == ""){
    lcdRow1 = "PHP: " + (String)totalBalance;
  }

  if(isLCD_refreshed == true){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdRow1);
    lcd.setCursor(0, 1);
    lcd.print(lcdRow2);
    isLCD_refreshed = false;
  }

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
    }
  }
  else if (currencyType == "coin"){
    if ((checkcoins >= minCount) && (checkcoins <= maxCount))
    {
      coins = given;
      totalBalance += coins;
      coins = 0;
      checkcoins = 0;
    }
  }
}

void Keypad_control(){
  key = keypad.getKey();
    if (key) {
      Serial.print("KEY : ");
      Serial.println(key);
      //lcd.clear();
      if (entryCode.length() <= 1 && (key !='#' || key !='*'))
      {  
        if (entryCode.length() == 2)
        {
          thisEntryCode = entryCode;
          caseRunOnce = true;
          caseSetup();
        }
        entryCode += key;
      }
      Serial.println(entryCode);
    }else{
      key = key;
    }
}

//int ItemPrice = doc["D7"][0];
//int ItemQuantity = doc["D7"][1];
//int ItemServo = doc["D7"][2];

void control_function(){
  switch (key)
  {
    case 'D': totalBalance++;break;
    case '*':Serial.println("entryCode " + thisEntryCode);
             caseRunOnce = true;
             isPurchased = true;
    case '#': entryCode = "";break;
  default:
    break;
  }
  key="";
}

void caseSetup(){

  if (thisEntryCode.length() == 2  && caseRunOnce == true)
  {
    Serial.println("caseSetup()");
    EEPROM.get(1, itemDataSAVED);
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, itemDataSAVED);
    int thisPrice = doc[thisEntryCode][0];
    int thisServo = doc[thisEntryCode][2];
    if (isPurchased == true)
    {
      purchase_function(thisPrice,thisServo);
    }
    lcdRow1 = "PHP: " + (String)totalBalance;
    lcdRow2 = ">" + thisEntryCode + "< " + "COST: " + (String)thisPrice;
    Serial.println("itemPrice: " + (String)thisPrice);
    Serial.println("ItemServo:" + (String)thisServo);
    caseRunOnce = false;
    isPurchased = false;
    isLCD_refreshed = true;
  }
  
}

void purchase_function(int cost,int thisServo){
  Serial.println("purchase_function("+ (String)cost + ", " + (String)thisServo + ")");
  int totalBalanceTemp = totalBalance;
  totalBalanceTemp -= cost;
  if (totalBalanceTemp >= 0){
    totalBalance = EEPROM.read(0);
    totalBalance = totalBalance - cost;
    Serial.println("COST: " + cost);
    EEPROM.write(0, totalBalance);
    servoSpin(thisServo);
  }else{
    totalBalance = totalBalance;
    //LCD_display(1, 1, "Not Enough BAL");
  }
}

void servoSpin(int slot){
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
    if (deciSeconds2 >= checkcoins * 1.2)
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
    caseSetup();
    LCD_display();
  }
}

void loop()
{
  RunCodeInMillis();
}
