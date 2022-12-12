#include "main.h"

AsyncWebServer server(80);
TwoWire i2cBus = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2cBus);
RTC_DS1307 rtc;

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

struct tm timeinfo;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
const long timeAutoUpdateScreen = 15000;

bool listHasError[5] = {0};

uint8_t storeMessageChecked[TOTAL_MESSAGE] = {0};
char bufferMsg[TOTAL_MESSAGE][SCREEN_WIDTH] = {
    "Hello world",
    "This is a message",
    "This is a message 2"};

unsigned long lastTime = 0;
unsigned long lastTimeDotClock = 0;
unsigned long lastTimeWifi = 0;

bool dotClockState = false;
bool setupWifiState = false;

bool alarmCheck = false;
bool wifiCheck = false;
bool lockScreenCheck = false;

int8_t screenIndex = 0;
int8_t setAlarmIndex = 0;
int8_t messageIndex = 0;

void setup()
{
    Serial.begin(BAUD_RATE);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    initSPIFFS();
    initI2C();
    initOLed();
    if (!initWifi() && setupWifiState)
    {
        startWifiAP();
    }
    initRTC();
    initDateTime();
    joystickSetup(VRx_PIN, VRy_PIN, SW_PIN);
    displayErrorInit();
}

void loop()
{
    updateTime();
    joystickLoop();
    updateScreen();
    reCheckWifi();
    reCheckAlarm();
}

void initSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An error has occurred while mounting SPIFFS");
        listHasError[FS] = true;
    }
    Serial.println("SPIFFS mounted successfully");
}

void initI2C()
{
    if (!i2cBus.begin(SDA_PIN, SCL_PIN, 400000))
        listHasError[I2C] = true;
}

void initOLed()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        listHasError[OLED] = true;
    }
    else
    {
        Serial.println("SSD1306 OLed Found");
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();
}

bool initWifi()
{
    ssid = SSID;
    pass = PASS;
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);

    if (ssid == "" || pass == "")
    {
        dataWifi = readFile(SPIFFS, "/wifi.txt");
        Serial.println(dataWifi);
        int countBreakData = 0;
        for (int i = 0; i < dataWifi.length(); i++)
        {
            if (dataWifi[i] == '|')
            {
                countBreakData++;
                continue;
            }

            if (countBreakData == 0)
            {
                ssid += dataWifi[i];
            }
            else if (countBreakData == 1)
            {
                pass += dataWifi[i];
            }
        }
    }

    // ssid = readFile(SPIFFS, ssidPath);
    // pass = readFile(SPIFFS, passPath);
    Serial.println(ssid);
    Serial.println(pass);

    if (ssid == "")
    {
        setupWifiState = true;
        Serial.println("Undefined SSID or IP address.");
        display.setCursor(0, 0);
        display.print("Undefined SSID or IP address.");
        display.display();
        delay(1000);
        return false;
    }

    if (!WiFi.config(localIP, localGateway, subnet))
    {
        Serial.println("STA Failed to configure");
        return false;
    }

    Serial.print("Connecting to ");
    Serial.println(ssid.c_str());

    display.setCursor(0, 0);
    display.print("Connecting to ");
    display.setCursor(0, 10);
    display.print(ssid.c_str());
    display.display();
    delay(1500); // Pause for 2 seconds

    WiFi.begin(ssid.c_str(), pass.c_str());

    int waitConnectWifi = 0;
    wifiCheck = true;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        display.print(".");
        display.display();

        waitConnectWifi++;
        if (waitConnectWifi > 10)
        {
            wifiCheck = false;
            break;
        }
    }

    if (wifiCheck)
    {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());

        display.setCursor(0, 30);
        display.print("WiFi connected");
        display.setCursor(0, 40);
        display.print("IP address: ");
        display.setCursor(0, 50);
        display.print(WiFi.localIP());
        display.display();
        delay(2000); // Pause for 2 seconds
        return true;
    }
    else
    {
        display.setCursor(0, 30);
        display.print("WiFi connect failed");
        display.display();
        delay(2000); // Pause for 2 seconds
        int countDown = 5;
        attachInterrupt(BUTTON_PIN, setupWifiHandler, RISING);
        while (countDown > 0)
        {
            display.clearDisplay();
            display.setTextSize(1);
            display.setCursor(0, 0);
            display.print("Can you setup new wifi? ");
            display.setTextSize(2);
            display.setCursor(50, 30);
            display.print(countDown);
            display.display();
            delay(1000);
            countDown--;
        }
        Serial.println("Out of while loop");
        return false;
    }
}
void startWifiAP()
{
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Web Server Root URL
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    //           { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

    // server.serveStatic("/", SPIFFS, "/");

    dataWifi = "";

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", index_html); });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            dataWifi += ssid;
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            dataWifi += pass;
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
        }

        dataWifi += "|";
      }
      Serial.println("Data Wifi: " + dataWifi);
      writeFile(SPIFFS, wifiPath, dataWifi.c_str());
      request->send(200, "text/plain", "Done. ESP will restart, please waiting esp reconnect to new wifi.");
      delay(3000);
      ESP.restart(); });
    server.begin();

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Setup New Wifi");
    display.setCursor(0, 20);
    display.println("Try to submit at");
    display.setCursor(0, 40);
    display.print("http://");
    display.print(IP);
    display.display();
    delay(3000);
}

void initRTC()
{
    if (!rtc.begin(&i2cBus))
    {
        Serial.println("Couldn't find RTC");
        listHasError[RTC] = true;
    }

    if (!rtc.isrunning())
    {
        Serial.println("RTC is NOT running, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void initDateTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    if (getLocalTime(&timeinfo))
    {
        rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    }
}

void displayErrorInit()
{
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ERROR: ");

    uint8_t positionAtY = 10;
    if (listHasError[FS])
    {
        display.setCursor(0, positionAtY);
        display.print("SPIFFS mounted failed");
        positionAtY += 10;
    }
    if (listHasError[I2C])
    {
        display.setCursor(0, positionAtY);
        display.print("I2C not found");
        positionAtY += 10;
    }
    if (listHasError[OLED])
    {
        display.setCursor(0, positionAtY);
        display.print("SSD1306 not found");
        positionAtY += 10;
    }
    if (listHasError[RTC])
    {
        display.setCursor(0, positionAtY);
        display.print("RTC not found");
        positionAtY += 10;
    }
    if (listHasError[JOYSTICK])
    {
        display.setCursor(0, positionAtY);
        display.print("ADS1115 not found");
        positionAtY += 10;
    }

    display.display();
    delay(5000);
}

void drawIconWifi(uint8_t x, uint8_t y)
{
    if (wifiCheck == true)
    {
        display.drawBitmap(x, y, IconWifi, 13, 10, WHITE);
    }
}

void drawIconAlarm(uint8_t x, uint8_t y)
{
    if (alarmCheck == true)
    {
        display.drawBitmap(x, y, IconClock, 13, 10, WHITE);
    }
}

void drawIconLockScreen(uint8_t x, uint8_t y)
{
    if (lockScreenCheck == true)
    {
        display.drawBitmap(x, y, IconLock, 7, 10, BLACK, WHITE);
    }
}

void drawTimeSmall(uint8_t x, uint8_t y)
{
    // toggle state dot clock when millis() - lastTimeDotClock > 500
    if (millis() - lastTimeDotClock > 500)
    {
        lastTimeDotClock = millis();
        if (dotClockState == 0)
        {
            dotClockState = 1;
        }
        else
        {
            dotClockState = 0;
        }
    }

    // GET hour and minute
    char hour[4];
    strftime(hour, sizeof(hour), "%H", &timeinfo);
    char minute[4];
    strftime(minute, sizeof(minute), "%M", &timeinfo);

    // Display Date and Time on OLED display
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(x, y);
    display.print(hour);
    if (dotClockState)
    {
        display.print(":");
    }
    else
    {
        display.print(" ");
    }
    display.print(minute);
}

void displayWeather()
{
    // GET DATE
    // Get full weekday name
    char weekDay[10];
    strftime(weekDay, sizeof(weekDay), "%a", &timeinfo);
    // Get day of month
    char dayMonth[4];
    strftime(dayMonth, sizeof(dayMonth), "%d", &timeinfo);
    // Get abbreviated month name
    char monthName[5];
    strftime(monthName, sizeof(monthName), "%b", &timeinfo);
    // Get year
    char year[6];
    strftime(year, sizeof(year), "%Y", &timeinfo);

    // GET TIME
    // Get hour (24 hour format)
    char hour[4];
    strftime(hour, sizeof(hour), "%H", &timeinfo);
    // Get minute
    char minute[4];
    strftime(minute, sizeof(minute), "%M", &timeinfo);
    // Get second
    char second[4];
    strftime(second, sizeof(second), "%S", &timeinfo);

    // Display Date and Time on OLED display
    display.clearDisplay();
    drawIconLockScreen(0, 0);
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(18, 0);
    display.print(hour);
    display.print(":");
    display.print(minute);
    display.print(":");
    display.print(second);
    display.setTextSize(1);
    display.setCursor(16, 20);
    display.print(weekDay);
    display.print(", ");
    display.print(dayMonth);
    display.print(" ");
    display.print(monthName);
    display.print(" ");
    display.print(year);

    display.drawBitmap(20, 40, IconWeatherSnow, 20, 20, BLACK, WHITE);
    display.setTextSize(2);
    display.setCursor(50, 40);
    display.print("25oC");

    display.display();
}

void displayMain()
{
    display.clearDisplay();

    drawIconWifi(0, 0);
    drawIconAlarm(15, 0);
    drawIconLockScreen(60, 0);
    drawTimeSmall(95, 0);

    display.display();
}

void displayMessage()
{
    display.clearDisplay();

    drawIconWifi(0, 0);
    drawIconAlarm(15, 0);
    drawIconLockScreen(60, 0);
    drawTimeSmall(95, 0);

    display.setTextSize(1);
    display.setTextWrap(true);
    display.setTextColor(WHITE);

    for (uint8_t i = 0; i < TOTAL_MESSAGE; i++)
    {
        uint8_t positionAt = (i + 1) * 16;
        if (storeMessageChecked[i] == 1)
        {
            display.drawBitmap(0, positionAt, IconMailChecked, 7, 10, BLACK, WHITE);
        }
        else
        {
            display.drawBitmap(0, positionAt, IconMailUnChecked, 7, 10, BLACK, WHITE);
        }
        display.setCursor(10, positionAt);
        display.print(bufferMsg[i]);
    }

    handlerReadMessage();

    display.display();
}

void displaySetAlarm()
{
    char bufferHour[10] = {'\0'};
    char bufferMin[10] = {'\0'};

    if (setMinuteAlarmTemp < 10)
        sprintf(bufferMin, "0%d", setMinuteAlarmTemp);
    else
        sprintf(bufferMin, "%d", setMinuteAlarmTemp);

    if (setHourAlarmTemp < 10)
        sprintf(bufferHour, "0%d", setHourAlarmTemp);
    else
        sprintf(bufferHour, "%d", setHourAlarmTemp);

    display.clearDisplay();
    drawIconLockScreen(60, 0);
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(20, 16);
    display.print(bufferHour);
    display.print(":");
    display.print(bufferMin);

    drawIconAlarm(25, 52);
    display.setCursor(40, 55);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.print(setHourAlarm);
    display.print(":");
    display.print(setMinuteAlarm);
    display.print(" ");

    setDayAlarm = setDayAlarm == 0 ? timeinfo.tm_mday : setDayAlarm;
    display.print(setDayAlarm);

    display.print("/");
    display.print(timeinfo.tm_mon + 1);

    display.display();

    handlerSetAlarm();
}

void displayRingAlarm()
{
    for (int i = 0; i < 20; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            display.clearDisplay();
            display.drawBitmap(0, 0, AnimationClock[j], 128, 64, BLACK, WHITE);
            display.display();
            delay(100);
        }
    }
}

void updateScreen()
{
    ButtonState joystickButtonState = buttonReadState();
    if (joystickButtonState == PRESSED_LONG)
    {
        lockScreenCheck = !lockScreenCheck;
        Serial.print("Press long, lockScreenCheck = ");
        Serial.println(lockScreenCheck);
        lastTime = millis();
    }

    if (!lockScreenCheck)
    {
        JoystickAxisState joystickAxisState = joystickAxisReadState();
        switch (joystickAxisState)
        {
        case AXIS_LEFT:
            screenIndex--;
            if (screenIndex < 0)
            {
                screenIndex = (TOTAL_SCREEN - 1) - 1;
            }
            lastTime = millis();
            delay(200);
            break;

        case AXIS_RIGHT:
            screenIndex++;
            if (screenIndex >= (TOTAL_SCREEN - 1))
            {
                screenIndex = 0;
            }
            lastTime = millis();
            delay(200);
            break;

        case AXIS_CENTER:
        default:
            break;
        }
    }

    if (millis() - lastTime > timeAutoUpdateScreen && !lockScreenCheck)
    {
        lastTime = millis();
        screenIndex++;
        if (screenIndex >= (TOTAL_SCREEN - 1)) // -1 because last screen is ring alarm
        {
            screenIndex = 0;
        }
    }

    switch (screenIndex)
    {
    case MAIN:
        displayMain();
        break;
    case LOCAL_TIME:
        displayWeather();
        break;
    case MESSAGE:
        displayMessage();
        break;
    case SET_ALARM:
        displaySetAlarm();
        break;
    case RING_ALARM:
        // displayRingAlarm();
        break;
    default:
        displayMain();
        break;
    }
}

void updateTime()
{
    if (!getLocalTime(&timeinfo, 500U))
    {
        DateTime now = rtc.now();
        timeinfo.tm_hour = now.hour();
        timeinfo.tm_min = now.minute();
        timeinfo.tm_sec = now.second();

        timeinfo.tm_year = now.year() - 1900;
        timeinfo.tm_mon = now.month() - 1;
        timeinfo.tm_mday = now.day();

        timeinfo.tm_wday = now.dayOfTheWeek();
    }
}

void reCheckWifi()
{
    if (millis() - lastTimeWifi > 600000)
    {
        lastTimeWifi = millis();
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Reconnect Wifi");
            WiFi.begin(SSID, PASS);
            ESP.restart();
        }
    }
}

void reCheckAlarm()
{
    if (alarmCheck && setDayAlarm == timeinfo.tm_mday && setHourAlarm == timeinfo.tm_hour && setMinuteAlarm == timeinfo.tm_min)
    {
        digitalWrite(LED_PIN, HIGH);
        displayRingAlarm();
        digitalWrite(LED_PIN, LOW);

        alarmCheck = false;
    }
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n');
        break;
    }
    return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- frite failed");
    }
}

void IRAM_ATTR setupWifiHandler()
{
    if (digitalRead(BUTTON_PIN) == 1)
    {
        setupWifiState = wifiCheck ? false : true;
    }
}

void handlerSetAlarm()
{
    if (!lockScreenCheck)
        return;

    ButtonState joystickButtonState = buttonReadState();
    JoystickAxisState joystickAxisSate = joystickAxisReadState();

    if (joystickButtonState == PRESSED_SHORT)
    {
        setDayAlarm = timeinfo.tm_mday;
        setHourAlarm = timeinfo.tm_hour + setHourAlarmTemp;
        setMinuteAlarm = timeinfo.tm_min + setMinuteAlarmTemp;
        if (setMinuteAlarm >= 60)
        {
            setMinuteAlarm -= 60;
            setHourAlarm++;
        }
        if (setHourAlarm >= 24)
        {
            setHourAlarm -= 24;
            setDayAlarm++;
        }

        Serial.println("Set Alarm");
        Serial.print("Day: ");
        Serial.println(setDayAlarm);
        Serial.print("Hour: ");
        Serial.println(setHourAlarm);
        Serial.print("Minute: ");
        Serial.println(setMinuteAlarm);

        alarmCheck = true;

        display.setTextSize(1);
        display.setCursor(100, 0);
        display.print("OK");
        display.display();
    }

    switch (joystickAxisSate)
    {
    case AXIS_LEFT:
        setAlarmIndex++;
        if (setAlarmIndex > 1)
        {
            setAlarmIndex = 0;
        }

        lastTime = millis();
        delay(200);
        break;

    case AXIS_RIGHT:
        setAlarmIndex--;
        if (setAlarmIndex < 0)
        {
            setAlarmIndex = 1;
        }

        lastTime = millis();
        delay(200);
        break;

    default:
        break;
    }

    if (setAlarmIndex == 0)
    {
        switch (joystickAxisSate)
        {
        case AXIS_UP:
            setHourAlarmTemp++;
            if (setHourAlarmTemp > 23)
            {
                setHourAlarmTemp = 0;
            }
            lastTime = millis();
            delay(50);
            break;

        case AXIS_DOWN:
            setHourAlarmTemp--;
            if (setHourAlarmTemp < 0)
            {
                setHourAlarmTemp = 23;
            }
            lastTime = millis();
            delay(50);
            break;

        default:
            break;
        }

        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(20, 37);
        display.print("--");
        display.setCursor(73, 37);
        display.print("  ");
        display.display();
        delay(50);
        display.setCursor(25, 37);
        display.print("  ");
    }
    else if (setAlarmIndex == 1)
    {
        switch (joystickAxisSate)
        {
        case AXIS_UP:
            setMinuteAlarmTemp++;
            if (setMinuteAlarmTemp > 59)
            {
                setMinuteAlarmTemp = 0;
            }
            lastTime = millis();
            delay(50);
            break;

        case AXIS_DOWN:
            setMinuteAlarmTemp--;
            if (setMinuteAlarmTemp < 0)
            {
                setMinuteAlarmTemp = 59;
            }
            lastTime = millis();
            delay(50);
            break;

        default:
            break;
        }

        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(20, 37);
        display.print("   ");
        display.setCursor(73, 37);
        display.print("--");
        display.display();
        delay(50);
        display.setCursor(73, 37);
        display.print("  ");
    }
}

void handlerReadMessage()
{
    if (!lockScreenCheck)
        return;

    JoystickAxisState joystickAxisSate = joystickAxisReadState();

    switch (joystickAxisSate)
    {
    case AXIS_DOWN:
        messageIndex++;
        storeMessageChecked[messageIndex] = 1;
        if (messageIndex > TOTAL_MESSAGE - 1)
        {
            messageIndex = 0;
        }
        delay(200);
        break;

    case AXIS_UP:
        messageIndex--;
        storeMessageChecked[messageIndex] = 1;
        if (messageIndex < 0)
        {
            messageIndex = TOTAL_MESSAGE - 1;
        }
        delay(200);
        break;
    case AXIS_CENTER:
    default:
        storeMessageChecked[messageIndex] = 1;
        break;
    }

    uint8_t positionAt = (messageIndex + 1) * 16;
    display.drawBitmap(0, positionAt, IconMailChecked, 7, 10, BLACK, WHITE);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(10, positionAt);
    display.print(bufferMsg[messageIndex]);
}
