// https://github.com/adafruit/RTClib
// https://github.com/marcoschwartz/LiquidCrystal_I2C
// https://github.com/earlephilhower/ESP8266Audio

//#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <RTClib.h>

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#include "viola.h"
#include "viola2.h"

#include <LiquidCrystal_I2C.h>

#include <Wire.h>
// #include <NTPClient.h>
#include <WiFiUdp.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_Millis rtc;
char datetime[48];

DateTime _now;
uint32_t _ts;
uint32_t _todaySeconds;

//定義wifi
WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, 28800);
//const char *ssid     = "Yueverlasting";
//const char *password = "10241228";
//const char *ssid     = "TVTC-MOA";
//const char *password = "22592566";
// const char *ssid     = "71216457";
// const char *password = "22611182";
const char *ssid[] = {"MOA-TC", "TVTC-MOA", "TT49"};
const char *password[] = {"22927399", "22592566", "22592566"};
uint8_t currentNetwork = 0;
const uint8_t NETCFG_LEN = 3;
//定義PIR
const int PIN_PIRout = D5;

void takeCurrentTime() {
  _now = rtc.now();
  _ts = _now.unixtime();
  _todaySeconds = _ts % 86400;
}

char *getDateTimeString() {
  takeCurrentTime();
  sprintf(datetime, "%04d/%02d/%02d %02d:%02d:%02d",
          _now.year(), _now.month(), _now.day(),
          _now.hour(), _now.minute(), _now.second());
  return datetime;
}


void printDateTimeInfo() {
  getDateTimeString();
  lcd.setCursor ( 0, 0 ); lcd.print( datetime);
}


bool startWifi(uint8_t networkNo, uint32_t timeout) {
  WiFi.begin(ssid[networkNo], password[networkNo]);  //wifi開始連接
  timeout = millis() + (timeout * 1000);

  Serial.print("start wifi");
  Serial.print(":"); Serial.print(ssid[networkNo]);
  //  Serial.print("/"); Serial.print(password[networkNo]);
  Serial.print(";"); Serial.print(timeout);
  Serial.print(";"); Serial.print(millis());

  while (timeout > millis() && WiFi.status() != WL_CONNECTED ) {
    printDateTimeInfo();
    delay ( 1000 ); Serial.print ( "." );
    // lcd.print('.');
  }
  Serial.print(";"); Serial.print(WiFi.status() != WL_CONNECTED);
  Serial.println();
  return (WiFi.status() == WL_CONNECTED);
}

bool endWifi() {
  if (WiFi.status() == WL_CONNECTED ) {
    WiFi.mode(WIFI_OFF);
  }
}

WiFiUDP Udp;

const int NTP_PACKET_SIZE = 48;

byte packetBuffer[ NTP_PACKET_SIZE];
//char timeServer[] = "time.nist.gov";
const unsigned int localPort = 2390;
// IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServer(103, 18, 128, 60); // tw.pool.ntp.org NTP server

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  //Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  //Serial.println("5");
  Udp.endPacket();
  //Serial.println("6");
}


uint32_t retriveNtp(uint32_t timeout) {
  bool success;

  sendNTPpacket(timeServer);
  success = false;
  timeout = millis() + timeout * 1000;
  Udp.begin(localPort);
  while (timeout > millis()) {
    if ((success = Udp.parsePacket())) {
      break;
    }
    Serial.print('.');
    delay(1000);
  }
  if (success) {
    Serial.println(" packet received; ");
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    Serial.print("Seconds since Jan 1 1900 = ");    Serial.print(secsSince1900);
    Serial.print(" Unix time = "); Serial.println(epoch);

    return epoch;
  }
  return 0;
}

bool syncNtpDate() {
  lcd.clear();
  lcd.setCursor(0, 1); lcd.print("WIFI:");  lcd.print( ssid[currentNetwork] );
  bool success ;

  success = false;
  if (success == false) success = startWifi(currentNetwork, 10);
  if (success == false) success = startWifi(currentNetwork, 10);
  if (success == false) success = startWifi(currentNetwork, 10);

  lcd.setCursor(5, 1);
  lcd.print( (success) ? "OK      " : "fail    ");
  if (success == false) {
    currentNetwork = (currentNetwork + 1) % NETCFG_LEN;
    return false;
  }

  Serial.println("Sync NTP: ");
  lcd.setCursor(10, 1); lcd.print("NTP:");
  uint32_t epoch = 0;
  if (epoch == 0) epoch = retriveNtp(10);
  if (epoch == 0) epoch = retriveNtp(10);
  if (epoch == 0) epoch = retriveNtp(10);

  lcd.setCursor(15, 1);
  lcd.print( (epoch == 0) ? "fail" : "OK");

  if (epoch > 0) {
    rtc.adjust(epoch + (8 * 60 * 60));
    // rtc.adjust(epoch + (0 * 60 * 60));
    printDateTimeInfo();
    lcd.setCursor(0, 2);
    lcd.print(datetime);
  }
  endWifi();
  return success;
}




AudioGeneratorMP3 *wav;
AudioFileSourcePROGMEM *file;
AudioOutputI2SNoDAC *out;
void playSound(int sound) {
  //  Serial.print(";sound="); Serial.print(sound);
  //   return;
  switch (sound) {
    case 0:
      file = new AudioFileSourcePROGMEM( viola, sizeof(viola) );
      break;
    case 1:
      file = new AudioFileSourcePROGMEM( viola2, sizeof(viola2) );
      break;
    default: return;
  }
  Serial.print(";file="); Serial.print(sound);

  wav = new AudioGeneratorMP3();
  out = new AudioOutputI2SNoDAC();
  Serial.print(";start; ");
  wav -> begin(file, out);
  // delay(3000);
  while ( wav->isRunning() ) {
    if (!wav->loop()) {
      wav->stop();
      delete out;
      delete file;
    }
  }
  delete wav;
}

void setup() {
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  {
    Serial.begin(57600);
    Serial.setTimeout(5000);
    pinMode(PIN_PIRout, INPUT);
    digitalWrite(PIN_PIRout, LOW);
    Serial.println();
  }

  {
    //  const uint8_t scl = D6; // defalut I2c
    //  const uint8_t sda = D7;// defalut I2c
    const uint8_t scl = D1;
    const uint8_t sda = D2;
    Wire.begin(sda, scl);
  }
  {
    lcd.begin(20, 4);
    //  lcd.display();
    lcd.clear();
    printDateTimeInfo();
    Serial.println(datetime);
  }

}


bool checkTimeThenPlay() {
  const bool PLAY_ANY = false;
  const uint32_t PI_START = (7 * 60 + 30) * 60;
  const uint32_t PI_END = (9 * 60 + 30) * 60;
  const uint32_t PO_START = (16 * 60 + 30) * 60;
  const uint32_t PO_END = (22 * 60 + 0) * 60;

  if (PLAY_ANY || _todaySeconds >= PI_START && _todaySeconds <= PI_END) {
    getDateTimeString();
    lcd.backlight();    
    lcd.setCursor(10, 3); lcd.print(datetime + 11);
    lcd.print('+');
    playSound(0);
  } else if (PLAY_ANY || _todaySeconds >= PO_START && _todaySeconds <= PO_END) {
    getDateTimeString();
    lcd.backlight();
    lcd.setCursor(11, 3); lcd.print(datetime + 11);
    lcd.print('-');
    playSound(1);
  } else {
    return false;
  }
  if(PLAY_ANY) {
    lcd.setCursor(19, 3);lcd.print('*');
  }
  return true;
}


void loop() {
  static uint32_t nextNtp = 0;
  static bool ntpInited = false;

  static uint32_t stopBacklight = 0;

  delay(1000);
  printDateTimeInfo();
  Serial.print(datetime);
  Serial.print(";ts="); Serial.print(_ts);
  Serial.print(";sec="); Serial.print(_todaySeconds);

  const uint32_t NTP_START = (1 * 60) * 60;
  const uint32_t NTP_END = (1 * 60 + 30) * 60;
  const uint32_t NTP_ONEDAYSECONDS = 86400L;
  if (!ntpInited) {
    lcd.backlight();
    if (syncNtpDate()) {
      ntpInited = true;
      nextNtp = (_ts / NTP_ONEDAYSECONDS + 1) * NTP_ONEDAYSECONDS + NTP_START;
    }
    stopBacklight = _ts + 10;
  }
  if (!ntpInited) return;

  if (_todaySeconds >= NTP_START && _todaySeconds <= NTP_END && nextNtp <= _todaySeconds) {
    lcd.backlight();
    if (syncNtpDate()) {
      ntpInited = true;
      nextNtp = (_ts / NTP_ONEDAYSECONDS + 1) * NTP_ONEDAYSECONDS + NTP_START;
    }
    stopBacklight = _ts + 10;
  }
  Serial.print(";ntp="); Serial.print(nextNtp);

  static uint8_t currentValue;
  {
    digitalWrite(PIN_PIRout, LOW);
    currentValue = digitalRead(PIN_PIRout);
    lcd.setCursor(3, 0);
    lcd.print(currentValue);
    Serial.print(currentValue); Serial.print(' ');
  }

  if (stopBacklight > _ts) lcd.backlight();
  else if (currentValue) lcd.backlight();
  else lcd.noBacklight();

  static uint8_t prevValue = LOW;
  static uint32_t startHi = 0;
  bool played ;
  played = false;
  if (currentValue != prevValue && currentValue) {
    startHi=0;
    lcd.print(startHi);
    played = checkTimeThenPlay();
    stopBacklight = _ts + 10;
  } else if (currentValue != prevValue && currentValue) {
  } else if (currentValue) {
    startHi++;
    stopBacklight = _ts + 10;
    // if(startHi>=30) currentValue=LOW;
  }
  prevValue = currentValue;

  {
    char buf[40];
    sprintf(buf,"%d %3d",currentValue,startHi);
    lcd.setCursor(0,3);
    lcd.print(buf);
  }
  if (!played)  Serial.println();
}
