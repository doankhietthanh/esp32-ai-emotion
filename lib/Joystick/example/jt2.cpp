#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_ADS1x15.h>

TwoWire i2cBus = TwoWire(0);
Adafruit_ADS1115 ads;

void setup(void)
{
    Serial.begin(115200);
    i2cBus.begin(14, 15, 400000);
    if (!ads.begin(0x48, &i2cBus))
    {
        Serial.println("ADS1115 not found");
        while (1)
            ;
    }

    WiFi.begin("Nha Bat On", "chochimancut");
    WiFi.setSleep(false);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
}

void loop(void)
{
    unsigned int adc0, adc1, adc2, adc3;

    adc0 = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);
    adc2 = ads.readADC_SingleEnded(2);
    adc3 = ads.readADC_SingleEnded(3);
    Serial.print("AIN0: ");
    Serial.println(adc0);
    Serial.print("AIN1: ");
    Serial.println(adc1);
    Serial.print("AIN2: ");
    Serial.println(adc2);
    Serial.print("AIN3: ");
    Serial.println(adc3);
    Serial.println(" ");

    delay(1000);
}