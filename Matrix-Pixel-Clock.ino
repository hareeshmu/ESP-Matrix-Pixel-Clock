#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"
FASTLED_USING_NAMESPACE
#include "Arduino.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

String weatherMain = "";
String weatherDescription = "";
String weatherLocation = "";
String country;
int humidity;
int pressure;
float temp;
float tempMin, tempMax;
int clouds;
float windSpeed;
String date;

String weatherString;

#define NUM_MAX 4

#define DIN_PIN 13  // D7
#define CS_PIN  0  // D3
#define CLK_PIN 14  // D5

#define DATA_PIN      12     // D6
#define LED_TYPE      WS2812
#define COLOR_ORDER   GRB
#define NUM_LEDS      24

#define MILLI_AMPS         2000
#define FRAMES_PER_SECOND  120

#include "max7219_hr.h"
#include "fonts.h"

WiFiClient client;

// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
const char* ssid     = "SSID";     // SSID of local network
const char* password = "PASSWORD";   // Password on network
String weatherKey = "KEY";
String weatherLang = "&lang=en";
String cityID = "CITYID"; //Weather City ID
// read OpenWeather api description for more info
// =======================================================================

CRGB leds[NUM_LEDS];

const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
int brightnessIndex = 4;
uint8_t brightness = brightnessMap[brightnessIndex];

byte hourval, minuteval, secondval, oldsec;
int cyclesPerSec;
long newSecTime;
int subSeconds;

void digitalClockDisplay();
void printDigits(int digits);

int mode = 1; // Variable of the display mode of the clock
int modeMax = 6; // Change this when new modes are added. This is so selecting modes can go back beyond.
int LEDOffset = 0;


void setup() 
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN,1);
  sendCmdAll(CMD_INTENSITY,5);
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift(" WIFI  ",15);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);         // for WS2812 (Neopixel)
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 255)); //CRGB::Blue
  FastLED.show();

  FastLED.setBrightness(brightness);
  delay(1000);
  clearLEDs();
}


// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS]={0};
byte digold[MAX_DIGITS]={0};
byte digtrans[MAX_DIGITS]={0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx=0;
int dy=0;
byte del=0;
int h,m,s;

// =======================================================================

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
  if(updCnt<=0) { // every 10 scrolls, ~450s=7.5m
    updCnt = 20;
    Serial.println("Getting data ...");
    printStringWithShift("   Sync...",15);
    //getWeatherData();
    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }
 
  if(millis()-clkTime > 30000 && !del && dots) { // clock for 15s, then scrolls for about 30s
    printStringWithShift("    Welcome      ",20);
    printStringWithShift(date.c_str(),20);
    printStringWithShift("   Time:       ",20);
    //printStringWithShift(weatherString.c_str(),40);

    updCnt--;
    clkTime = millis();
  }
  
  if(millis()-dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
  
  updateTime();
  showAnimClock();


  //if (now() != prevDisplay) { //update the display only if time has changed
    //prevDisplay = now();  
    digitalClockDisplay(); 
    secondval = s;
    minuteval = m;
    hourval = h;

  //}

  // clear LED array
  memset(leds, 0, NUM_LEDS * 3);
  timeDisplay();
  
  // Update LEDs
  LEDS.show();
 
}

// =======================================================================

void showSimpleClock()
{
  dx=dy=0;
  clr();
  showDigit(h/10,  0, dig6x8);
  showDigit(h%10,  8, dig6x8);
  showDigit(m/10, 17, dig6x8);
  showDigit(m%10, 25, dig6x8);
  showDigit(s/10, 34, dig6x8);
  showDigit(s%10, 42, dig6x8);
  setCol(15,dots ? B00100100 : 0);
  setCol(32,dots ? B00100100 : 0);
  refreshAll();
}

// =======================================================================

void showAnimClock()
{
  byte digPos[6]={0,8,17,25,34,42};
  int digHt = 12;
  int num = 6; 
  int i;
  int hr;
  
  if(del==0) {
    del = digHt;
    for(i=0; i<num; i++) digold[i] = dig[i];

    if(h>12) {
      hr=h-12;
    }
    else {
      hr=h;
    }

    if(h=0) {
      hr=12;
    }

    dig[0] = hr/10 ? hr/10 : 10;
    dig[1] = hr%10;
    
    dig[2] = m/10;
    dig[3] = m%10;
    dig[4] = s/10;
    dig[5] = s%10;
    for(i=0; i<num; i++)  digtrans[i] = (dig[i]==digold[i]) ? 0 : digHt;
  } else
    del--;
  
  clr();
  for(i=0; i<num; i++) {
    if(digtrans[i]==0) {
      dy=0;
      showDigit(dig[i], digPos[i], dig6x8);
    } else {
      dy = digHt-digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }
  dy=0;
  setCol(15,dots ? B00100100 : 0);
  setCol(32,dots ? B00100100 : 0);
  refreshAll();
  delay(30);
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data)
{
  if(dy<-8 | dy>8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if(col+i>=0 && col+i<8*NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if(!dy) scr[col + i] = v; else scr[col + i] |= dy>0 ? v>>dy : v<<-dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if(dy<-8 | dy>8) return;
  col += dx;
  if(col>=0 && col<8*NUM_MAX)
    if(!dy) scr[col] = v; else scr[col] |= dy>0 ? v>>dy : v<<-dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX*8 + i] = 0;
  return w;
}


// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {
  if (c < ' ' || c > '~'+25) return;
  c -= 32;
  int w = showChar(c, font);
  for (int i=0; i<w+1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char* s, int shiftDelay){
  while (*s) {
    printCharWithShift(*s, shiftDelay);
    s++;
  }
}

// =======================================================================

const char *weatherHost = "api.openweathermap.org";

void getWeatherData()
{
  Serial.print("connecting to "); Serial.println(weatherHost);
  if (client.connect(weatherHost, 80)) {
    client.println(String("GET /data/2.5/weather?id=") + cityID + "&units=metric&appid=" + weatherKey + weatherLang + "\r\n" +
                "Host: " + weatherHost + "\r\nUser-Agent: ArduinoWiFi/1.1\r\n" +
                "Connection: close\r\n\r\n");
    Serial.println(String("GET /data/2.5/weather?id=") + cityID + "&units=metric&appid=" + weatherKey + weatherLang + "\r\n" +
                "Host: " + weatherHost + "\r\nUser-Agent: ArduinoWiFi/1.1\r\n" +
                "Connection: close\r\n\r\n");
  } else {
    Serial.println("connection failed");
    return;
  }
  String line;
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    Serial.println("w.");
    repeatCounter++;
  }
  while (client.connected() && client.available()) {
    char c = client.read(); 
    if (c == '[' || c == ']') c = ' ';
    line += c;
  }

  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(line);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  //weatherMain = root["weather"]["main"].as<String>();
  weatherDescription = root["weather"]["description"].as<String>();
  weatherDescription.toLowerCase();
  //  weatherLocation = root["name"].as<String>();
  //  country = root["sys"]["country"].as<String>();
  temp = root["main"]["temp"];
  humidity = root["main"]["humidity"];
  pressure = root["main"]["pressure"];
  tempMin = root["main"]["temp_min"];
  tempMax = root["main"]["temp_max"];
  windSpeed = root["wind"]["speed"];
  clouds = root["clouds"]["all"];
  String deg = String(char('~'+25));
  weatherString = "         Temp: " + String(temp,1) + deg + "C (" + String(tempMin,1) + deg + "-" + String(tempMax,1) + deg + ")  ";
  weatherString += weatherDescription;
  weatherString += "  Humidity: " + String(humidity) + "%  ";
  weatherString += "  Pressure: " + String(pressure) + "hPa  ";
  weatherString += "  Clouds: " + String(clouds) + "%  ";
  weatherString += "  Wind: " + String(windSpeed,1) + "m/s                 ";
}

// =======================================================================

float utcOffset = 5.5;
long localEpoc = 0;
long localMillisAtUpdate = 0;

void getTime()
{
  WiFiClient client;
  if (!client.connect("www.google.co.in", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.co.in\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    //Serial.println(".");
    repeatCounter++;
  }

  String line;
  client.setNoDelay(false);
  while(client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    Serial.println(line);
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      date = "     "+line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();
}

// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L) % 86400L;
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================
//========================================================================

void clearLEDs()
{      
  for (int i = 0; i < NUM_LEDS; i++) // Set all the LEDs to off
    {
      leds[i].r = 0;
      leds[i].g = 0;
      leds[i].b = 0;
    }
}

void timeDisplay()
{ 
  switch (mode)
    {
      case 0:
        minimalClock();
        break;
      case 1:
        basicClock();
        break;
      case 2:
        //smoothSecond(now);
        break;
      case 3:
        //outlineClock(now);
        break;
      case 4:
        //minimalMilliSec(now);
        break;
      case 5:
        //simplePendulum(now);
        break;
      case 6:
        //breathingClock(now);
        break;
      default: // Keep this here and add more timeDisplay modes as defined cases.
        {
          mode = 1;
        }
    }
}

void minimalClock()
{
  unsigned char hourPos = (hourval%12)*5;
  Serial.print(hourPos);
  leds[(hourPos+LEDOffset)%60].r = 255;
  leds[(minuteval+LEDOffset)%60].g = 255;
  leds[(secondval+LEDOffset)%60].b = 255;
}

void basicClock()
{


  if (secondval!=oldsec)
    {
      oldsec = secondval;
      cyclesPerSec = (millis() - newSecTime);
      newSecTime = millis();
    } 

  subSeconds = (((millis() - newSecTime)*60)/cyclesPerSec)%60;  // This divides by 733, but should be 1000 and not sure why???
  // Millisec lights are set first, so hour/min/sec lights override and don't flicker as millisec passes

  //Serial.println(subSeconds);
  
  int subsecval = (subSeconds+LEDOffset)/2.5;

  leds[subsecval].r = 50;
  leds[subsecval].g = 50;
  leds[subsecval].b = 50;
  
  unsigned char hourPos = ((hourval%12)*2 + (minuteval+6)/30);
  //Serial.println(hourPos);
//  leds[(hourPos+LEDOffset+59)%NUM_LEDS].r = 255;
//  leds[(hourPos+LEDOffset+59)%NUM_LEDS].g = 0;
//  leds[(hourPos+LEDOffset+59)%NUM_LEDS].b = 0; 
 
  leds[(hourPos+LEDOffset)%NUM_LEDS].r = 255;
  leds[(hourPos+LEDOffset)%NUM_LEDS].g = 0;
  leds[(hourPos+LEDOffset)%NUM_LEDS].b = 0;

  //Serial.println((hourPos+LEDOffset)%NUM_LEDS);
  
//  leds[(hourPos+LEDOffset+1)%NUM_LEDS].r = 255;
//  leds[(hourPos+LEDOffset+1)%NUM_LEDS].g = 0;
//  leds[(hourPos+LEDOffset+1)%NUM_LEDS].b = 0;
  
  int minval = (minuteval+LEDOffset)/2.5;
  leds[minval].r = 0;
  leds[minval].g = 255;
  leds[minval].b = 0;  

  //Serial.println(minval);

  int secval = (secondval+LEDOffset)/2.5;
  
  leds[secval].r = 0;
  leds[secval].g = 0;
  leds[secval].b = 255;

  //Serial.println(secval);
  
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(h);
  printDigits(m);
  printDigits(s);
  Serial.print(" ");
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
