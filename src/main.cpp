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
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

char bufferMsg[TOTAL_MESSAGE][SCREEN_WIDTH] = {
    "Hello world",
    "This is a message",
    "This is a message 2"};

unsigned long lastTime = 0;
unsigned long lastTimeDotClock = 0;
unsigned long lastTimeWifi = 0;

uint8_t dotClockState = false;
uint8_t screenIndex = 0;

uint8_t alarmCheck = true;
uint8_t wifiCheck = false;
uint8_t lockScreenCheck = false;
;

void setup()
{
  Serial.begin(BAUD_RATE);
  initSPIFFS();
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  Serial.println(ssid);
  Serial.println(pass);
  initI2C();
  initOLed();
  if (!initWifi())
  {
    startWifiAP();
  }
  initRTC();
  initDateTime();
}

void loop()
{
  updateTime();
  updateScreen();
  reCheckWifi();
}

void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void initI2C()
{
  i2cBus.begin(SDA_PIN, SCL_PIN, 400000);
}

void initOLed()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
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

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  if (ssid == "")
  {
    Serial.println("Undefined SSID or IP address.");
    display.setCursor(0, 0);
    display.print("Undefined SSID or IP address.");
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
    display.setCursor(0, 45);
    display.print("Try: http://" + WiFi.softAPIP());
    display.display();
    delay(2000); // Pause for 2 seconds
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
      writeFile(SPIFFS, wifiPath, dataWifi.c_str());
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart(); });
  server.begin();
}

void initRTC()
{
  if (!rtc.begin(&i2cBus))
  {
    Serial.println("Couldn't find RTC");
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

  display.drawBitmap(20, 40, IconWeatherRain, 20, 20, BLACK, WHITE);
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

  display.setTextColor(WHITE);
  display.setTextSize(1);

  for (uint8_t i = 0; i < TOTAL_MESSAGE; i++)
  {
    uint8_t positionAt = (i + 1) * 16;
    display.drawBitmap(0, positionAt, IconMailChecked, 7, 10, BLACK, WHITE);
    display.setCursor(10, positionAt);
    display.print(bufferMsg[i]);
  }

  // display.startscrollright(5, 7);

  display.display();
}

void updateScreen()
{
  if (millis() - lastTime > 15000 && !lockScreenCheck)
  {
    lastTime = millis();
    screenIndex++;
    if (screenIndex >= TOTAL_SCREEN)
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
  default:
    displayMain();
    break;
  }
}

void updateTime()
{
  // if (!getLocalTime(&timeinfo))
  // {
  //   Serial.println("Failed to obtain time");
  // }

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