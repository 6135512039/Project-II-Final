#define BLYNK_PRINT Serial
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <TridentTD_LineNotify.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h>
RTC_DS3231 RTC;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LINE_TOKEN "WRwJDW2chARPYHj5J7RiB4ahSQfgcfbPZisDUdXAKIy"

#define SET_PIN 0
#define MUX_A D3
#define MUX_B D4
#define MUX_C D5
#define BUZZER1 D7
#define FORCE_RTC_UPDATE 2
const int pushButton = 15;     // pushbutton pin
const int OutputPin =  12;        // LED pin

int buttonState = 0;          // The status of the pushbutton will be store in button state : HIGH or LOW

  char blynk_token[40] = "";
  bool shouldSaveConfig = false;

  const long offsetTime = 25200;
  
  int hourNow, minuteNow, secondNow;
 
  int timeUpdated;
 
  WiFiUDP ntpUDP;

  NTPClient timeClient(ntpUDP, "pool.ntp.org", offsetTime);

  WidgetLCD LCD(V0);

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);  
  Serial.println();
  pinMode(SET_PIN, INPUT_PULLUP);
  delay(3000);
  

  //read configuration from FS json
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_blynk_token);

  if (digitalRead(SET_PIN) == LOW) {
    LCD.clear();
    lcd.begin();
    lcd.clear();
    wifiManager.resetSettings();
  }

  if (!wifiManager.autoConnect("MEDICINE BOX SOS","12345678")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println("wifi connected");
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    }
    
    Serial.println();
    Serial.print("local ip : ");
    Serial.println(WiFi.localIP());
    Serial.print("Blynk Token : ");
    Serial.println(blynk_token);
    Blynk.config(blynk_token);
    EEPROM.begin(4);
    if (!RTC.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
    }
    byte addvalue = EEPROM.read(timeUpdated);
    Serial.print("EEPROM: ");
    Serial.print(addvalue);
    Serial.print(" == ");
    Serial.print(FORCE_RTC_UPDATE);
    Serial.println(" ?");
    if (addvalue != FORCE_RTC_UPDATE) 
    {
      Serial.println("Forcing Time Update");
      syncTime();
      Serial.println("Updating EEPROM..");
      EEPROM.write(timeUpdated, FORCE_RTC_UPDATE);
      EEPROM.commit();
      } 
    else 
      {
       Serial.println("Time has been updated before...EEPROM CHECKED");
       Serial.print("EEPROM: ");
       Serial.print(addvalue);
       Serial.print(" = ");
       Serial.print(FORCE_RTC_UPDATE);
       Serial.println("!");
       }
    Wire.begin(); // Start the I2C 
    pinMode(MUX_A, OUTPUT);
    pinMode(MUX_B, OUTPUT);
    pinMode(MUX_C, OUTPUT);
    pinMode(OutputPin, OUTPUT);
    pinMode(BUZZER1, OUTPUT); 
    pinMode(pushButton, INPUT);
    digitalWrite(OutputPin,LOW);
    }
    
void syncTime(void) 
{
  timeClient.begin();
  timeClient.update();

  long actualTime = timeClient.getEpochTime();
  Serial.print("Internet Epoch Time: ");
  Serial.println(actualTime);
  RTC.adjust(DateTime(actualTime));

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  }

void changeMux(int c, int b, int a) 
{
  digitalWrite(MUX_A, a);
  digitalWrite(MUX_B, b);
  digitalWrite(MUX_C, c);
  }

BLYNK_WRITE(V1) 
{
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(LOW, LOW, LOW); //0
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(LOW, LOW, LOW); //0
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }

 BLYNK_WRITE(V2) 
 {
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(LOW, LOW, HIGH); //1
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(LOW, LOW, HIGH); //1
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }

 BLYNK_WRITE(V3) 
 {
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(LOW, HIGH, LOW); //2
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(LOW, HIGH, LOW); //2
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }

 BLYNK_WRITE(V4) 
 {
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(LOW, HIGH, HIGH); //3
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(LOW, HIGH, HIGH); //3
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }

 BLYNK_WRITE(V5) 
 {
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(HIGH, LOW, LOW); //4
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(HIGH, LOW, LOW); //
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }

 BLYNK_WRITE(V6) 
 {
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(HIGH, LOW, HIGH); //5
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(HIGH, LOW, HIGH); //5
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }
 BLYNK_WRITE(V7) 
 {
  if(param.asInt() == 1)
  {
    Serial.print("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");
    changeMux(HIGH, HIGH, LOW); //6
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");
    LINE.setToken(LINE_TOKEN);
    LINE.notify("ถึงเวลากินยาแล้ว");
    delay(1000); 
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
  }
  else
  {    
    Serial.print("LED OFF\n");
    LCD.clear();
    LCD.print(0,0,"LED OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED OFF");
    changeMux(HIGH, HIGH, LOW); //6
    digitalWrite(OutputPin,LOW);
    delay(1000);
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("หมดเวลากินยาแล้ว");
    delay(1000);   
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");   
    digitalWrite(BUZZER1, HIGH);
    delay(15000); 
    Serial.print("Sound OFF\n");
    LCD.clear();
    LCD.print(0,0,"Sound OFF");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound OFF");
    digitalWrite(BUZZER1, LOW);
    delay(2000);
    lcd.clear();
  }
 }

void loop() 
{
    if(WiFi.status() != WL_CONNECTED)
    {
      DateTime nowTime = RTC.now();

      Serial.print("TIME RTC DS3231");
      Serial.print(": ");
      if(hourNow < 10)(Serial.print("0"));
        Serial.print(nowTime.hour(), DEC);
        Serial.print(":");
      if(minuteNow < 10)(Serial.print("0"));
        Serial.print(nowTime.minute(), DEC);
        Serial.print(":");
      if(secondNow < 10)(Serial.print("0"));
        Serial.print(nowTime.second(), DEC);
        Serial.println();

      LCD.clear();
      LCD.print(0,0,"MEDICINE BOX SOS");
      LCD.print(0,1,"TIME:");
      LCD.print(5,1,nowTime.hour());
      LCD.print(7,1,":");
      LCD.print(8,1,nowTime.minute());
      LCD.print(10,1,":");
      LCD.print(11,1,nowTime.second());    
  
      lcd.begin();
      lcd.setCursor(0, 0);
      lcd.print("MEDICINE BOX SOS");
      lcd.setCursor(0, 1);
      lcd.print("TIME");
      lcd.print(':');
      lcd.print(nowTime.hour(), DEC);
      lcd.print(':');
      lcd.print(nowTime.minute(), DEC);
      lcd.print(':');
      lcd.print(nowTime.second(), DEC);
      }
    else
      {
      DateTime nowTime = RTC.now();  
      timeClient.update();
      secondNow = timeClient.getSeconds();
      minuteNow = timeClient.getMinutes();
      hourNow = timeClient.getHours();

      Serial.print("TIME RTC DS3231");
      Serial.print(": ");
      if(hourNow < 10)(Serial.print("0"));
        Serial.print(nowTime.hour(), DEC);
        Serial.print(":");
      if(minuteNow < 10)(Serial.print("0"));
        Serial.print(nowTime.minute(), DEC);
        Serial.print(":");
      if(secondNow < 10)(Serial.print("0"));
        Serial.print(nowTime.second(), DEC);
        Serial.println();

      Serial.print("TIME NTP");
      Serial.print(": ");
      if(hourNow < 10)(Serial.print("0"));
        Serial.print(hourNow);
        Serial.print(":");
      if(minuteNow < 10)(Serial.print("0"));
        Serial.print(minuteNow);
        Serial.print(":");
      if(secondNow < 10)(Serial.print("0"));
        Serial.print(secondNow);
        Serial.println();

      LCD.clear();
      LCD.print(0,0,"MEDICINE BOX SOS");
      LCD.print(0,1,"TIME:");
      LCD.print(5,1,hourNow);
      LCD.print(7,1,":");
      LCD.print(8,1,minuteNow);
      LCD.print(10,1,":");
      LCD.print(11,1,secondNow);     

      lcd.begin();
      lcd.setCursor(0, 0);
      lcd.print("MEDICINE BOX SOS");
      lcd.setCursor(0, 1);
      lcd.print("TIME");
      lcd.print(":");
      if(hourNow < 10)(lcd.print("0"));
        lcd.print(hourNow);
        lcd.print(":");
      if(minuteNow < 10)(lcd.print("0"));
        lcd.print(minuteNow);
        lcd.print(":");
      if(secondNow < 10)(lcd.print("0"));
        lcd.print(secondNow);   
        }
   buttonState = digitalRead(pushButton); 
   if (buttonState == HIGH) 
   {
    Serial.print("SOS\n");
    LCD.clear();
    LCD.print(0,0,"SOS SOS SOS");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("SOS SOS SOS");
    changeMux(HIGH, HIGH, HIGH);  
    digitalWrite(OutputPin, HIGH);
    Serial.println("LED ON\n");
    LCD.clear();
    LCD.print(0,0,"LED ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LED ON");         
    Serial.print("Sound ON\n");
    LCD.clear();
    LCD.print(0,0,"Sound ON");
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Sound ON");
    Serial.print("Send Message\n");
    LCD.clear();
    LCD.print(0,0,"Send Message");    
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Send Message");   
    LINE.setToken(LINE_TOKEN);
    LINE.notify("SOS ขอความช่วยเหลือ ด่วน!"); 
    delay(10000);
    } 
   else
    {
    changeMux(HIGH, HIGH, HIGH);
    digitalWrite(OutputPin, LOW);                 
    Serial.println("LED OFF\n");
    }
    
 delay(100);
 digitalWrite(BUZZER1, LOW);
 Blynk.run();
}
