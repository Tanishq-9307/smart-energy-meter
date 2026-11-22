/************************************************************
   SMART ENERGY METER WITH BLYNK
   Hardware:
   ESP32 + ACS712-5A + OLED SSD1306 + Buzzer

   Blynk Virtual Pins:
   V0 = Current (A)
   V1 = Power (W)
   V2 = Energy (kWh)
   V3 = Electricity Cost (INR)
   V4 = Budget (INR)
 ************************************************************/

//---------------- Blynk Configuration ----------------//

// Credentials live in secrets.h (git-ignored).
// Copy secrets.example.h -> secrets.h and fill in your own values.
#include "secrets.h"

#define BLYNK_PRINT Serial

//---------------- Libraries ----------------//

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

//---------------- Wi-Fi Details ----------------//

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASSWORD;

//---------------- Blynk Timer ----------------//

BlynkTimer timer;

//---------------- OLED Configuration ----------------//

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

//---------------- Pin Definitions ----------------//

const int sensorPin = 34;
const int buzzerPin = 26;

//---------------- Constants ----------------//

const float supplyVoltage = 12.0;

// ACS712-5A sensitivity
const float sensitivity = 0.185;

// Electricity cost
const float costPerkWh = 5.0;

//---------------- Variables ----------------//

int adcOffset = 0;

float current = 0;
float power = 0;
float energy = 0;
float cost = 0;
float budget = 0;

bool halfBudgetShown = false;

unsigned long previousTime = 0;

//====================================================
// SEND DATA TO BLYNK
//====================================================

void sendDataToBlynk()
{
  if (Blynk.connected())
  {
    Blynk.virtualWrite(V0, current);
    Blynk.virtualWrite(V1, power);
    Blynk.virtualWrite(V2, energy);
    Blynk.virtualWrite(V3, cost);
    Blynk.virtualWrite(V4, budget);

    Serial.println("Data sent to Blynk");
  }
}

//====================================================
// SETUP
//====================================================

void setup()
{
  Serial.begin(115200);

  //---------------- ESP32 ADC Setup ----------------//

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  //---------------- Buzzer Setup ----------------//

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  //---------------- OLED Setup ----------------//

  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      ))
  {
    Serial.println("OLED Failed");

    while (1);
  }

  display.clearDisplay();

  display.setTextColor(WHITE);

  display.setTextSize(1);

  //---------------- Welcome Screen ----------------//

  display.setCursor(15, 15);
  display.println("SMART ENERGY");

  display.setCursor(30, 30);
  display.println("METER");

  display.display();

  delay(2000);

  //---------------- Connect to Wi-Fi and Blynk ----------------//

  display.clearDisplay();

  display.setCursor(0, 15);
  display.println("Connecting WiFi...");

  display.display();

  Serial.println();
  Serial.println("Connecting to Blynk...");

  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );

  if (Blynk.connected())
  {
    Serial.println("Blynk Connected!");

    display.clearDisplay();

    display.setCursor(10, 20);
    display.println("BLYNK CONNECTED");

    display.display();

    delay(1500);
  }

  //---------------- Budget Input ----------------//

  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("Enter Budget");

  display.println("from Serial");

  display.println("Monitor");

  display.display();

  Serial.println("--------------------------------");
  Serial.println(" SMART ENERGY METER ");
  Serial.println("--------------------------------");

  Serial.print("Enter Budget (Rs): ");

  while (Serial.available() == 0)
  {
    Blynk.run();
  }

  budget = Serial.parseFloat();

  Serial.print("Budget Entered : Rs ");

  Serial.println(budget);

  delay(1000);

  //---------------- ACS712 Calibration ----------------//

  display.clearDisplay();

  display.setCursor(0, 10);

  display.println("Calibrating...");

  display.setCursor(0, 30);

  display.println("Keep LED OFF");

  display.display();

  Serial.println();

  Serial.println("Calibrating ACS712...");

  Serial.println("Keep LED OFF");

  long sum = 0;

  for (int i = 0; i < 500; i++)
  {
    sum += analogRead(sensorPin);

    delay(2);
  }

  adcOffset = sum / 500;

  Serial.print("ADC Offset = ");

  Serial.println(adcOffset);

  display.clearDisplay();

  display.setCursor(10, 20);

  display.println("Calibration");

  display.setCursor(25, 35);

  display.println("Complete");

  display.display();

  delay(2000);

  //---------------- Start Energy Timer ----------------//

  previousTime = millis();

  //---------------- Blynk Update Timer ----------------//

  // Send values every 2 seconds

  timer.setInterval(
    2000L,
    sendDataToBlynk
  );

  Serial.println();

  Serial.println(
    "Smart Energy Meter Started"
  );
}

//====================================================
// LOOP
//====================================================

void loop()
{
  //---------------- Run Blynk ----------------//

  Blynk.run();

  timer.run();

  //---------------- Read ACS712 ----------------//

  long sum = 0;

  for (int i = 0; i < 100; i++)
  {
    sum += analogRead(sensorPin);

    delay(2);
  }

  int adc = sum / 100;

  //---------------- ADC to Voltage ----------------//

  float deltaVoltage =
    ((adc - adcOffset) * 3.3)
    / 4095.0;

  //---------------- Calculate Current ----------------//

  current =
    deltaVoltage
    / sensitivity;

  //---------------- Make Current Positive ----------------//

  if (current < 0)
  {
    current = -current;
  }

  //---------------- Remove Small Noise ----------------//

  if (current < 0.02)
  {
    current = 0;
  }

  //---------------- Calculate Power ----------------//

  power =
    supplyVoltage
    * current;

  //---------------- Calculate Energy ----------------//

  // Simulation:
  // 1 real second = 10 simulated hours

  unsigned long currentTime =
    millis();

  float simulatedHours =
    (
      (currentTime - previousTime)
      / 1000.0
    )
    * 10.0;

  // Convert watts to kWh

  energy +=
    (
      power
      / 1000.0
    )
    * simulatedHours;

  previousTime =
    currentTime;

  //---------------- Calculate Cost ----------------//

  cost =
    energy
    * costPerkWh;

  //---------------- OLED Display ----------------//

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println(
    "SMART ENERGY METER"
  );

  //---------------- Voltage ----------------//

  display.setCursor(0, 12);

  display.print("V:");

  display.print(
    supplyVoltage,
    1
  );

  display.print("V");

  //---------------- Current ----------------//

  display.setCursor(64, 12);

  display.print("I:");

  display.print(
    current,
    2
  );

  display.print("A");

  //---------------- Power ----------------//

  display.setCursor(0, 24);

  display.print("P:");

  display.print(
    power,
    2
  );

  display.print("W");

  //---------------- Energy ----------------//

  display.setCursor(0, 36);

  display.print("E:");

  display.print(
    energy,
    3
  );

  display.print("kWh");

  //---------------- Cost ----------------//

  display.setCursor(0, 48);

  display.print("Cost:");

  display.print(
    cost,
    2
  );

  display.print("Rs");

  //---------------- Budget ----------------//

  display.setCursor(0, 56);

  display.print("Bud:");

  display.print(
    budget,
    0
  );

  display.print("Rs");

  display.display();

  //---------------- 50% Budget Alert ----------------//

  if (
    (cost >= budget * 0.5)
    &&
    !halfBudgetShown
  )
  {
    halfBudgetShown = true;

    display.clearDisplay();

    display.setTextSize(2);

    display.setCursor(20, 5);

    display.println("50%");

    display.setTextSize(1);

    display.setCursor(0, 40);

    display.println(
      "BUDGET REACHED!"
    );

    display.display();

    Serial.println();

    Serial.println(
      "************************"
    );

    Serial.println(
      "*** 50% BUDGET HIT! ***"
    );

    Serial.println(
      "************************"
    );

    //---------------- Beep 3 Times ----------------//

    for (
      int i = 0;
      i < 3;
      i++
    )
    {
      digitalWrite(
        buzzerPin,
        HIGH
      );

      delay(300);

      digitalWrite(
        buzzerPin,
        LOW
      );

      delay(300);
    }

    delay(2000);
  }

  //---------------- 100% Budget Alert ----------------//

  if (
    budget > 0
    &&
    cost >= budget
  )
  {
    display.clearDisplay();

    display.setTextSize(2);

    display.setCursor(0, 10);

    display.println("BUDGET");

    display.setCursor(0, 35);

    display.println("REACHED!");

    display.display();

    Serial.println();

    Serial.println(
      "************************"
    );

    Serial.println(
      "*** BUDGET REACHED! ***"
    );

    Serial.println(
      "************************"
    );

    //---------------- Continuous Buzzer ----------------//

    while (1)
    {
      digitalWrite(
        buzzerPin,
        HIGH
      );

      delay(500);

      digitalWrite(
        buzzerPin,
        LOW
      );

      delay(500);
    }
  }

  //---------------- Serial Monitor ----------------//

  Serial.println(
    "--------------------------------"
  );

  Serial.print(
    "ADC Offset : "
  );

  Serial.println(
    adcOffset
  );

  Serial.print(
    "Current    : "
  );

  Serial.print(
    current,
    3
  );

  Serial.println(
    " A"
  );

  Serial.print(
    "Power      : "
  );

  Serial.print(
    power,
    3
  );

  Serial.println(
    " W"
  );

  Serial.print(
    "Energy     : "
  );

  Serial.print(
    energy,
    4
  );

  Serial.println(
    " kWh"
  );

  Serial.print(
    "Cost       : Rs "
  );

  Serial.println(
    cost,
    2
  );

  Serial.print(
    "Budget     : Rs "
  );

  Serial.println(
    budget,
    2
  );

  //---------------- Main Loop Delay ----------------//

  delay(1000);
}
