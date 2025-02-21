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

const int ADL5902_VOUT = A0;
const int BUTTON_PIN = 2;
const int ATTEN_BUTTON_PIN = 3;
int buttonState = HIGH;
int lastButtonState = HIGH;
int attenButtonState = HIGH;
int lastAttenButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int menuIndex = 0;
float attenuationOffset = 30.0;
const float ATTENUATION_STEP = 1.0;

// Frequency settings
struct FrequencySetting {
    float intercept;
    float slope;
};

//https://www.analog.com/media/en/technical-documentation/data-sheets/adl5902.pdf
FrequencySetting frequencySettings[] = {
    {-54.1, 42.7}, // 5.8 GHz
    {-62.1, 51.0}, // 2.6 GHz
    {-62.7, 53.7}  // 900 MHz
};

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    u8g2.setFont(u8g2_font_5x7_tf);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ATTEN_BUTTON_PIN, INPUT_PULLUP);
    analogReference(INTERNAL4V096);
    analogReadResolution(10);
}

void loop() {
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) {
                menuIndex = (menuIndex + 1) % 3;
            }
        }
    }
    lastButtonState = reading;

    int attenReading = digitalRead(ATTEN_BUTTON_PIN);
    if (attenReading != lastAttenButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (attenReading != attenButtonState) {
            attenButtonState = attenReading;
            if (attenButtonState == LOW) {
                attenuationOffset += ATTENUATION_STEP;
                if (attenuationOffset > 50.0) {
                    attenuationOffset = 50.0;
                }
            }
        }
    }
    lastAttenButtonState = attenReading;

    float peakVoltage = 0;
    unsigned long startTime = millis();
    while (millis() - startTime < 1000) {
        float voltage = readRFVoltage();
        if (voltage > peakVoltage) {
            peakVoltage = voltage;
        }
    }

    float intercept = frequencySettings[menuIndex].intercept;
    float slope = frequencySettings[menuIndex].slope;
    float powerBeforeAttenuator = (peakVoltage + (intercept * slope)) / slope;
    float powerAfterAttenuator = powerBeforeAttenuator + attenuationOffset;
    float powerBeforeAttenuator_mW = pow(10, powerBeforeAttenuator / 10);
    float powerAfterAttenuator_mW = powerBeforeAttenuator_mW * pow(10, attenuationOffset / 10);

    displayValues(peakVoltage, powerBeforeAttenuator, powerAfterAttenuator, powerBeforeAttenuator_mW, powerAfterAttenuator_mW);
    SerialOutput(peakVoltage, powerBeforeAttenuator, powerAfterAttenuator, powerBeforeAttenuator_mW, powerAfterAttenuator_mW);
}

float readRFVoltage() {
    int rawValue = analogRead(ADL5902_VOUT);
    return rawValue * (4.096 / 1023.0) * 1000;
}

void displayValues(float voltage, float powerBefore, float powerAfter, float powerBefore_mW, float powerAfter_mW) {
    u8g2.firstPage();
    do {
        u8g2.clearBuffer();
        u8g2.setCursor(0, 10);
        u8g2.print("Freq: ");
        if (menuIndex == 0) u8g2.print("5.8 GHz");
        else if (menuIndex == 1) u8g2.print("2.6 GHz");
        else if (menuIndex == 2) u8g2.print("900 MHz");

        u8g2.setCursor(0, 20);
        u8g2.print("Volt: ");
        u8g2.print(voltage, 0);
        u8g2.print("mV");
        u8g2.setCursor(0, 32);
        u8g2.print("Atten: ");
        u8g2.print(attenuationOffset);
        u8g2.print("dB");
        u8g2.setCursor(0, 42);
        u8g2.print("M: ");
        u8g2.print(powerBefore, 2);
        u8g2.print("dBm ");
        u8g2.print(powerBefore_mW, 2);
        u8g2.print("mW ");
        u8g2.setCursor(0, 54);
        u8g2.print("C: ");
        u8g2.print(powerAfter, 2);
        u8g2.print("dBm ");
        u8g2.print(powerAfter_mW, 2);
        u8g2.print("mW ");
        u8g2.print("  <-");
        u8g2.sendBuffer();
    } while (u8g2.nextPage());
}

void SerialOutput(float voltage, float powerBefore, float powerAfter, float powerBefore_mW, float powerAfter_mW) {
    Serial.print("Freq: ");
    if (menuIndex == 0) Serial.print("5.8 GHz");
    else if (menuIndex == 1) Serial.print("2.6 GHz");
    else if (menuIndex == 2) Serial.print("900 MHz");
    Serial.print(", Voltage: ");
    Serial.print(voltage, 0);
    Serial.print("mV, Power Before: ");
    Serial.print(powerBefore, 2);
    Serial.print(" dBm (");
    Serial.print(powerBefore_mW, 4);
    Serial.print(" mW), Power After: ");
    Serial.print(powerAfter, 2);
    Serial.print(" dBm (");
    Serial.print(powerAfter_mW, 4);
    Serial.println(" mW)");
}

