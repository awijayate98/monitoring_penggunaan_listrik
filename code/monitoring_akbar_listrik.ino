/* PIN CONFIGURATION 
 *  PZEM004T RX  -> D5
 *           TX  -> D6
 *  LCD I2C  SCL -> D2
 *           SDA -> D1
 *  LED_K_D  C   -> D3
 *  LED_K_P  C   -> D4
 *  RELAY    IN  -> D0
 *  TMBL_R   IN  -> D7
 *  
 *  
 */

#define BLYNK_PRINT Serial
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define relay D3
#define led_d D0
#define led_p D4
#define tombol_r D7
#define ditekan 0
#define l_hidup 1
#define l_mati 0
#define simpan_wifi 1
#define simpan_data 2

char blynk_token[34] = "VTup7n_iy1rxwsavXQSYUMno0Qilel66";
char blynk_host[20] = "blynk.iot-cm.com";
char blynk_port[7] = "8080";
char g_listrik[15] = "1.444.70";
int portblynk= 8080;
bool shouldSaveConfig = false;

float kal;
int golongan_listrik;
float b_gl;
float voltage, current, power, energy,frequency;
float cost_kwh;
float Wattage;
int mVperAmp = 66; // use 185 for 5A, 100 for 20A Module and 66 for 30A Module
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
int flag;
int a,b;
boolean cutoff = false;
String kalimat, statusalat;
String statusl;
int waktu_sebelumnya = 0;
String api_key = "tPmAT5Ab3j7F9";
byte ring = 0;
int model;
boolean kondisi_reset = false;

BlynkTimer timer;
WidgetTerminal terminal(V7);
SoftwareSerial pzemSWSerial(D5, D6);
PZEM004Tv30 pzem_f(pzemSWSerial);
LiquidCrystal_I2C lcd(0x3F, 16, 2);
WiFiManager wifiManager;


void setup() {  //fungsi utama setup yannge berjalan hanya sekali ketika alat di hidupkan pada arduino
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(tombol_r, INPUT_PULLUP);
  pinMode(led_d, OUTPUT);
  pinMode(led_p, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(led_p, LOW);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("MONITORING LISTRIK");
  delay(1000);
  lcd.clear();
  WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
  if (!wifiManager.autoConnect("Air Quality", "12345678")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.reset();
    delay(3000);
  }
  inisial_ota();
  byte kondisi_tombol = digitalRead(tombol_r);
  if(kondisi_tombol == ditekan && kondisi_reset == false)
  {
    kondisi_reset = true;
    delay(200);
  }
    while(kondisi_reset == true)
    {
      ArduinoOTA.handle();
      lcd.setCursor(0,0);
      lcd.print("MODE PROGRAM");
    Serial.println("MODE PROGRAM");
  }
  initialize();
  kondisi(l_hidup);
  Blynk.begin(blynk_token, WiFi.SSID().c_str(), WiFi.psk().c_str(),blynk_host,portblynk);
  timer.setInterval(1000L, kirimdata);
  lcd.clear();
  Blynk.run();
  Blynk.virtualWrite(V8, model);
  //delay(2000);
  kondisi_reset = false;
}  

void loop() {  //fungsi utama looping pada arduino
  ArduinoOTA.handle();
  Blynk.run();
  timer.run();
  bacasensor();
  tampil_lcd();
  tampilserial();
  kirimdata();
  if(digitalRead(tombol_r) == ditekan && kondisi_reset == false)
  {
    ESP.reset();
  }
}

void kirimdata(){ //fungsi untuk mengirim data ke blynk
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, energy);
  Blynk.virtualWrite(V4, frequency);
  Blynk.virtualWrite(V6, cost_kwh);
  terminal.println(kalimat);
  digitalWrite(led_d, HIGH);
  delay(100);
  digitalWrite(led_d, LOW);
  delay(100);
}

  BLYNK_WRITE(V8)
{   
  kondisi(param.asInt());
}


  BLYNK_WRITE(V10)
{   
 lcd.setCursor(0,0);
 lcd.print("MEMFORMAT DATA WIFI ");
 wifiManager.resetSettings();
 delay(500);
 SPIFFS.format();
 delay(500);
 delay(500);
 ESP.reset();
}

  BLYNK_WRITE(V9)
{   
 lcd.setCursor(0,0);
 lcd.print("MEMFORMAT DATA ENERGY ");
  delay(3000);
  pzem_f.resetEnergy();
 delay(500);
 ESP.reset();
}


void kondisi(byte i) //fungsi untuk menghidupkan dan mematikan listrik perintah dari blynk
{
  
  if (i == l_hidup)
  {
    statusl = "LISTRIK HIDUP";
    digitalWrite(relay, LOW);
    model = 1;
  }else if(i == l_mati)
  {
    statusl = "LISTRIK MATI";
    digitalWrite(relay, HIGH);
    model = 0;
  }
}

void bacasensor() //fungsi membaca sensor
{
  if(isnan(pzem_f.voltage())){
    Serial.println("BACA ");
  }else{
    voltage = pzem_f.voltage();
    current = pzem_f.current();
    power = pzem_f.power();
    energy = pzem_f.energy(), 3;
    frequency = pzem_f.frequency();
    cost_kwh = energy * atof(g_listrik); 
  }
}

void tampilserial() { //fungsi untuk menampilkan serial
    kalimat = "volt=";
    kalimat += voltage;
    kalimat += "&";
    kalimat += "frek=";
    kalimat += frequency;
    kalimat += "&";
    kalimat += "amper=";
    kalimat += current;
    kalimat += "&";
    kalimat += "watt=";
    kalimat += power;
    kalimat += "&";
    kalimat += "status=";
    kalimat += statusl;
    kalimat += "&";
    kalimat += "kwh=";
    kalimat += energy;
    kalimat += "&";
    kalimat += "biaya=";
    kalimat += "RP. ";
    kalimat += cost_kwh;
    Serial.println(kalimat);

}

void tampil_lcd() { //fungsi untuk menampilkan pada lcd

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(voltage);

  lcd.setCursor(9,0);
  lcd.print("P:");
  lcd.print(power);

  //baris 2
  lcd.setCursor(0, 1);
  lcd.print("A:");
  lcd.print(current);
  
  lcd.setCursor(9, 1);
  lcd.print("E:");
  lcd.print(energy);
}
