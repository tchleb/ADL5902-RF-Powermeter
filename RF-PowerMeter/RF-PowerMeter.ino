#include <U8g2lib.h>
#include <Wire.h>
#include <math.h> 

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

//#define Intercept -54.1 //-54,1dBm @5.8Ghz
//#define Slope 42.7 //42,7mv/db @5.8Ghz
#define Intercept -62.1 //-62.1Bm @2.6Ghz
#define Slope 51 //51mv/db @2.6Ghz
 
const int ADL5902_VOUT = A0;      // Analog pin to read ADL5902 output voltage
float attenuationOffset = 30.0;    // Initial attenuation offset in dB
const float ATTENUATION_STEP = 1.0; // Step size for adjusting attenuation
int rawValue;

const int BUTTON_PIN = 2; // Change to your button pin
int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;


void setup() {
  Serial.begin(9600);
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x7_tf);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogReference(INTERNAL4V096);
  /*
    EXTERNAL        External Reference (REF)
    INTERNAL1V024   Internal 1.024 volts
    INTERNAL2V048   Internal 2.048 volts
    INTERNAL4V096   Internal 4.096 volts
   */
  analogReadResolution(10); // Resolution = 10, 11 or 12 Bit
  //10bit = 1023.0
  //12bit = 4095
}

void loop() {
  // Read the button state with debouncing
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        // Button is pressed, change the attenuation offset
        attenuationOffset += ATTENUATION_STEP;
        if (attenuationOffset > 50.0) {
          attenuationOffset = 50.0; // Set a maximum limit for the offset
        }
      }
    }
  }
  lastButtonState = reading;

  // Measure RF voltage
  float voltage = readRFVoltage();

  // Calculate RF power before the attenuator
  float powerBeforeAttenuator = (voltage + (Intercept * Slope)) / Slope; //power in dbm

  // Calculate RF power after the adjusted attenuation offset
  float powerAfterAttenuator = powerBeforeAttenuator +  attenuationOffset; //power in dbm
  //float powerAfterAttenuator = 10 * log10(powerAfterAttenuator_mW );
  
  // Calculate the power values in mW
  float powerBeforeAttenuator_mW = pow(10, powerBeforeAttenuator / 10);
  
  //float powerAfterAttenuator_mW = powerBeforeAttenuator_mW * 1000;
  float powerAfterAttenuator_mW = powerBeforeAttenuator_mW * pow(10, attenuationOffset / 10); //30db == 1000

  // Print all values on the OLED display
  displayValues(voltage, powerBeforeAttenuator, powerAfterAttenuator, powerBeforeAttenuator_mW, powerAfterAttenuator_mW);

  // Print the values, attenuation offset, and mW values to the Serial Monitor
  Serial.print("RF Voltage (mV): ");
  Serial.print(voltage, 0); // Display voltage with 4 decimal places in V
  Serial.print(", RF Power Before (dBm, mW): ");
  Serial.print(powerBeforeAttenuator, 4); // Display power with 4 decimal places in dBm
  Serial.print(", ");
  Serial.print(powerBeforeAttenuator_mW, 4); // Display power with 4 decimal places in mW
  Serial.print(", RF Power After (dBm, mW, ");
  Serial.print("Attenuation " + String(attenuationOffset) + " dB): ");
  Serial.print(powerAfterAttenuator, 4); // Display power with 4 decimal places in dBm
  Serial.print(", ");
  Serial.println(powerAfterAttenuator_mW, 4); // Display power with 4 decimal places in mW

  delay(1000); // Delay for one second before taking the next measurement
}

float readRFVoltage() {
  // Read the voltage from the ADL5902 output pin
  int rawValue = analogRead(ADL5902_VOUT);
  // Convert the analog value to a voltage (0-5V)
  int voltage_mV = rawValue * (4.096 / 1023.0) * 1000;
  return voltage_mV;
}

void displayValues(float voltage, float powerBefore, float powerAfter, float powerBefore_mW, float powerAfter_mW) {
    u8g2.firstPage();
 do {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 10);
  u8g2.print(voltage, 0); 
  //u8g2.print("RF Voltage (mV): ");
  u8g2.print("mV -> RF Voltage");
  u8g2.setCursor(0, 32);
  u8g2.print("with " + String(attenuationOffset) + "dB Attenuator");
  u8g2.setCursor(0, 42);
  u8g2.print(powerBefore, 4); // Display power with 4 decimal places in dBm
  u8g2.print(" dBm (");
  u8g2.print(powerBefore_mW, 4); // Display power with 4 decimal places in mW
  u8g2.print(" mW)");
  u8g2.setCursor(0, 54);
  u8g2.print("without " + String(attenuationOffset) + "dB Attenuator");
  u8g2.setCursor(0, 64);
  u8g2.print(powerAfter, 4); // Display power with 4 decimal places in dBm
  u8g2.print(" dBm (");
  u8g2.print(powerAfter_mW, 4); // Display power with 4 decimal places in mW
  u8g2.print(" mW)");
  u8g2.sendBuffer();
  } while ( u8g2.nextPage() );
}

