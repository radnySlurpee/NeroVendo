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
String userMode = "";
int arrayPrice[80];
int arrayQuantity[80];
int address1 = sizeof(arrayPrice);
bool defaultSetup = false;

void setup()
{ // initialize Serial for testing
  Serial.begin(9600);
  pinMode(interruptPinCoins, INPUT_PULLUP);
  pinMode(interruptPinBills, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinCoins), CurrencyAcceptor, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptPinBills), CurrencyAcceptor, RISING);
  detachInterrupt(digitalPinToInterrupt(interruptPinCoins));
  detachInterrupt(digitalPinToInterrupt(interruptPinBills));

  totalBalance = EEPROM.read(0);
  EEPROM.write(0,0); 
  EEPROM.get(1,arrayPrice);
  EEPROM.put(1, arrayPrice);
  EEPROM.get(address1,arrayQuantity);
  EEPROM.put(address1, arrayQuantity);
if (defaultSetup == false)
{
  ItemDataEntry("11", "11", 0, 10, 11);
  ItemDataEntry("22", "22", 1, 20, 2);
  ItemDataEntry("33", "33", 2, 30, 3);
  ItemDataEntry("44", "44", 3, 40, 4);
  ItemDataEntry("55", "55", 4, 50, 5);
  ItemDataEntry("66", "66", 5, 60, 6);
  ItemDataEntry("77", "77", 6, 70, 7);
  ItemDataEntry("88", "88", 7, 80, 8);
  ItemDataEntry("99", "99", 8, 90, 9);
  ItemDataEntry("AA", "AA", 9, 100, 10);
  ItemDataEntry("BB", "BB", 10, 110, 11);
  defaultSetup = true;
}

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
      Serial.println(entryCode);
    }else{
      key = key;
    }
}

void control_function_customer(){
  switch (key)
  {
    case '1': EEPROM.get(1, arrayPrice);
              purchase_function(arrayPrice[0]);
              EEPROM.get(address1, arrayQuantity);
              arrayQuantity[0] = arrayQuantity[0] - 1;
              EEPROM.put(address1, arrayQuantity);
              EEPROM.get(address1, arrayQuantity);
              Serial.println((String)arrayPrice[0]);
              Serial.println((String)arrayQuantity[0]);
              servoSpin(0);
              break;
    case '2': purchase_function(25);
              servoSpin(1);
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

void control_function_admin(){

  if (entryCode != "")
  {
    ItemDataEntry(entryCode, "11", 0, 10, 1);
    ItemDataEntry(entryCode, "22", 1, 20, 2);
    ItemDataEntry(entryCode, "33", 2, 30, 3);
    ItemDataEntry(entryCode, "44", 3, 40, 4);
    ItemDataEntry(entryCode, "55", 4, 50, 5);
    ItemDataEntry(entryCode, "66", 5, 60, 6);
    ItemDataEntry(entryCode, "77", 6, 70, 7);
    ItemDataEntry(entryCode, "88", 7, 80, 8);
    ItemDataEntry(entryCode, "99", 8, 90, 9);
    ItemDataEntry(entryCode, "AA", 9, 100, 10);
    ItemDataEntry(entryCode, "BB", 10, 110, 11);
    }
    if(key == '#'){
      entryCode = "";
      key="";
      lcd.clear();
    }
    
    
}
void ItemDataEntry(String thisEntry, String staticEntry, int arrayIndex, int price, int qty)
{
  bool itemValid = false;
  String lcdRow1_admin = "", lcdRow2_admin = "";
  if (thisEntry == staticEntry)
  {
    arrayPrice[arrayIndex] = price;
    EEPROM.put(1, arrayPrice[arrayIndex]);
    arrayQuantity[arrayIndex] = qty;
    EEPROM.put(address1, arrayQuantity[arrayIndex]);
    lcd.clear();
    lcdRow1_admin = "CODE: " + (String)staticEntry;
    lcdRow2_admin = "PHP:" + (String)EEPROM.get(1, arrayPrice[arrayIndex]) + ".00 " +
                    "QTY:" + (String)EEPROM.get(address1, arrayQuantity[arrayIndex]);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdRow1_admin);
    lcd.setCursor(0, 1);
    lcd.print(lcdRow2_admin);
    Serial.println(lcdRow2_admin);
    entryCode = "";
      itemValid = true;
    }
    
    
}

void purchase_function(int cost){
  int totalBalanceTemp = totalBalance;
  totalBalanceTemp -= cost;
  if (totalBalanceTemp >= 0){
    totalBalance = EEPROM.read(0);
    totalBalance = totalBalance - cost;
    Serial.println("COST: "+cost);
    EEPROM.write(0, totalBalance);
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
    if (userMode == "ADMIN_mode")
    { 
      control_function_admin();
    }else{
      control_function_customer();
      LCD_display();
    }
  }
}

void loop()
{
  RunCodeInMillis();
}
