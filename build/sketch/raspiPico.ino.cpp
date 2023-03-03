#include <Arduino.h>
#line 1 "D:\\projects\\ardPorjs\\raspiPico\\raspiPico.ino"
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <iomanip>
#include <sstream>

// #define RED 6
// #define GREEN 7
/*
   ! perhitungan I paling akurat = nilai sensor V dibagi resistansi (dicoba pakai 10 ohm)
*/

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;

int fqbase = 1000; // initial frequency
int fqmax = 130000;
int fq = fqbase;
int fqnext = fq;
int fqInterval = 1000; // interval between frequency

float adc_voltage = 0.0;
int adc_value = 0;
float R1 = 30000.0;
float R2 = 7500.0;
float I, V;
float pi = 3.14159;
float R = 2.00; // load resistor

float avgV, Vtot, Vcount, Vrms, Vmax, Vdc; // V and I rms based on https://www.electricaltechnology.org/2019/05/rms-voltage-calculator.html
float avgI, Itot, Icount, Irms, Imax;
bool sendFlag = false;

/*
  ! pins to raspberry pi pico
  pins for fixes and tweaks
  gp23 for better adc
  gp25 for onboard led

  pins for button
  gp6 for red
  gp7 for green

  pins for h bridge
  gp19 for 100% duty cycle
  gp18 for direction pwm (square wave input to h bridge)

  pins for ssd1306 OLED and INA219 using I2C
  gp4 (SDA) to SDA (default SDA I2C)
  gp5 (SCL) to SCK (default SCL I2C)
*/

Adafruit_INA219 ina219;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

#line 58 "D:\\projects\\ardPorjs\\raspiPico\\raspiPico.ino"
void setup();
#line 78 "D:\\projects\\ardPorjs\\raspiPico\\raspiPico.ino"
void setup1();
#line 115 "D:\\projects\\ardPorjs\\raspiPico\\raspiPico.ino"
void loop();
#line 151 "D:\\projects\\ardPorjs\\raspiPico\\raspiPico.ino"
void loop1();
#line 58 "D:\\projects\\ardPorjs\\raspiPico\\raspiPico.ino"
void setup()
{

  // initial setup and necessary fixes
  pinMode(23, OUTPUT); // for adc accuracy fix
  digitalWrite(23, HIGH);
  pinMode(25, OUTPUT); // to sign its on
  digitalWrite(25, HIGH);

  // setup for output and input pins
  pinMode(18, OUTPUT);

  analogWriteFreq(fq);
  analogWriteResolution(8);
  analogWrite(18, 127);

  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
}

void setup1()
{
  // init for serial monitor, V, and I variables
  Serial.begin(9600);
  ina219.begin();
  ina219.setCalibration_16V_400mA();

  avgV = 0;
  Vtot = 0;
  Vcount = 0;
  Vrms = 0;
  Vmax = 0;
  V = 0;
  Vdc = 0;
  avgI = 0;
  Itot = 0;
  Icount = 0;
  Irms = 0;
  Imax = 0;
  I = 0;
  shuntvoltage = 0;
  busvoltage = 0;
  current_mA = 0;
  loadvoltage = 0;

  // ssd 1306 setup
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // hbridge PWM as 100% duty cycle for power
  pinMode(19, OUTPUT);
  digitalWrite(19, HIGH);

  analogReadResolution(12);
}

void loop()
{
  // base frequency generator
  // commented code below not used on current version
  bool REDlogic = digitalRead(6);
  bool GREENlogic = digitalRead(7);
  if (REDlogic == false && GREENlogic == false)
  {
  }
  else if (REDlogic == false)
  {
    if (fqnext >= fqbase)
    {
      fqnext -= fqInterval;
    }
  }
  else if (GREENlogic == false)
  {
    if (fqnext <= fqmax)
    {
      fqnext += fqInterval;
    }
  }
  else if (REDlogic == true && GREENlogic == true)
  {
    if ((fqnext != fq))
    {
      fq = fqnext;
      analogWriteFreq(fq);
      analogWriteResolution(8);
      analogWrite(18, 127);
    }
  }
  delay(500);
}

void loop1()
{
  u8g2.clearBuffer();
  // get data through serial monitor
  // template in serial "<frequency>,<Voltage in V>,<Current in mA>,<Power in W>"
  // Frequency-to-serial-monitor
  // for (int j; j = 20; j++)
  //{
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  loadvoltage = busvoltage + shuntvoltage / 1000;
  V = loadvoltage;
  current_mA = ina219.getCurrent_mA();
  if (current_mA >= 0)
  {
    I = current_mA;
  }
  u8g2.drawStr(0, 20, ("Frequency:" + std::to_string(fqnext) + "Hz").c_str()); // expected frequency
  if (fq != fqnext)
  {
    u8g2.drawStr(0, 30, "LOADING");
  }
  else
  {
    u8g2.drawStr(0, 30, "RUNNING");
  }

  std::stringstream streamI, streamV;
  // current,voltage-to-display needed tobe contant char
  streamV << std::fixed << std::setprecision(4) << V;
  u8g2.drawStr(0, 40, ("Voltage:" + streamV.str() + "V").c_str());
  streamI << std::fixed << std::setprecision(4) << I;
  u8g2.drawStr(0, 50, ("Current:" + streamI.str() + "mA").c_str());

  u8g2.sendBuffer();

  // to serial monitor
  Serial.print(fq);
  Serial.print(",");
  Serial.print(V);
  Serial.print(",");
  Serial.print(I);
  Serial.print(",");
  Serial.println(V * (I / 1000), 4);

  // reset vars
  avgV = 0;
  Vtot = 0;
  Vcount = 0;
  Vrms = 0;
  Vmax = 0;
  V = 0;
  Vdc = 0;
  avgI = 0;
  Itot = 0;
  Icount = 0;
  Irms = 0;
  Imax = 0;
  I = 0;
  shuntvoltage = 0;
  busvoltage = 0;
  current_mA = 0;
  loadvoltage = 0;
  delay(200);
}

