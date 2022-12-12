#include <Arduino.h>
#include <WiFi.h>
#include <Joystick.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include "time.h"
#include "Icons.h"

#define BAUD_RATE 115200

#define SDA_PIN 14
#define SCL_PIN 15

#define VRx_PIN 0
#define VRy_PIN 1
#define SW_PIN 2

#define BUTTON_PIN 16
#define LED_PIN 4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

#define TOTAL_SCREEN 5
#define TOTAL_MESSAGE 3

typedef enum
{
    FS,
    I2C,
    OLED,
    RTC,
    JOYSTICK
} ERROR_INIT;

// typedef enum screen: main, message, local time
typedef enum
{
    MAIN,
    MESSAGE,
    LOCAL_TIME,
    SET_ALARM,
    RING_ALARM,
} screenType;

const char *SSID = "Nha Bat On";
const char *PASS = "chochimancut";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 60 * 60;
const int daylightOffset_sec = 3600;

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

String ssid;
String pass;
String dataWifi;

uint8_t setDayAlarm = 0;
uint8_t setHourAlarm = 0;
uint8_t setMinuteAlarm = 0;
int8_t setHourAlarmTemp = setHourAlarm;
int8_t setMinuteAlarmTemp = setMinuteAlarm;

const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";
const char *wifiPath = "/wifi.txt";

bool initWifi();
void initI2C();
void initRTC();
void initDateTime();
void initOLed();
void initSPIFFS();
void startWifiAP();
void IRAM_ATTR setupWifiHandler();

void displayErrorInit();

void drawIconWifi(uint8_t x, uint8_t y);
void drawIconAlarm(uint8_t x, uint8_t y);
void drawIconLockScreen(uint8_t x, uint8_t y);
void drawTimeSmall(uint8_t x, uint8_t y);

void displayMain();
void displayWeather();
void displayMessage();
void displaySetAlarm();
void displayRingAlarm();

void updateScreen();
void updateTime();
void reCheckWifi();
void reCheckAlarm();

void handlerSetAlarm();
void handlerReadMessage();

String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <title>ESP Wi-Fi Manager</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style type="text/css">
        html {
            font-family: Arial, Helvetica, sans-serif;
            display: inline-block;
            text-align: center;
        }

        h1 {
            font-size: 1.8rem;
            color: white;
        }

        p {
            font-size: 1.4rem;
        }

        .topnav {
            overflow: hidden;
            background-color: #0a1128;
        }

        body {
            margin: 0;
        }

        .content {
            padding: 5%;
        }

        .card-grid {
            max-width: 800px;
            margin: 0 auto;
            display: grid;
            grid-gap: 2rem;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
        }

        .card {
            background-color: white;
            box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
        }

        .card-title {
            font-size: 1.2rem;
            font-weight: bold;
            color: #034078;
        }

        input[type="submit"] {
            border: none;
            color: #fefcfb;
            background-color: #034078;
            padding: 15px 15px;
            text-align: center;
            text-decoration: none;
            display: inline-block;
            font-size: 16px;
            width: 100px;
            margin-right: 10px;
            border-radius: 4px;
            transition-duration: 0.4s;
        }

        input[type="submit"]:hover {
            background-color: #1282a2;
        }

        input[type="text"],
        input[type="number"],
        select {
            width: 50%;
            padding: 12px 20px;
            margin: 18px;
            display: inline-block;
            border: 1px solid #ccc;
            border-radius: 4px;
            box-sizing: border-box;
        }

        label {
            font-size: 1.2rem;
        }

        .value {
            font-size: 1.2rem;
            color: #1282a2;
        }

        .state {
            font-size: 1.2rem;
            color: #1282a2;
        }

        button {
            border: none;
            color: #fefcfb;
            padding: 15px 32px;
            text-align: center;
            font-size: 16px;
            width: 100px;
            border-radius: 4px;
            transition-duration: 0.4s;
        }

        .button-on {
            background-color: #034078;
        }

        .button-on:hover {
            background-color: #1282a2;
        }

        .button-off {
            background-color: #858585;
        }

        .button-off:hover {
            background-color: #252524;
        }
    </style>
</head>

<body>
    <div class="topnav">
        <h1>ESP Wi-Fi Manager</h1>
    </div>
    <div class="content">
        <div class="card-grid">
            <div class="card">
                <form action="/" method="POST">
                    <p>
                        <label for="ssid">SSID</label>
                        <input type="text" id="ssid" name="ssid" value="Nha Bat On"><br>
                        <label for="pass">Password</label>
                        <input type="text" id="pass" name="pass" value="chochimancut"><br>
                        <input type="submit" value="Submit">
                    </p>
                </form>
            </div>
        </div>
    </div>
</body>

</html>
)rawliteral";