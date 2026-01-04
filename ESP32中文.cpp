#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Fonts/FreeMonoBold24pt7b.h>
const char* WIFI_SSID     = "qwo124";
const char* WIFI_PASSWORD = "qwoqwoqwo";

//#define USE_STATIC_IP 1
#if USE_STATIC_IP
IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

const char* DEVICE_NAME = "EPD-Node-1";

static const uint8_t EPD_CS   = 3;
static const uint8_t EPD_DC   = 14;
static const uint8_t EPD_RST  = 2;
static const uint8_t EPD_BUSY = 17;

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(
  GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

WebServer server(80);

void epdClear()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_BLACK);
  }
  while (display.nextPage());
  display.hibernate();
}

void epdShowText(const String& text)
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    
    int lineHeight = 20;
    int yStart = 50;
    int currentY = yStart;
    int maxWidth = display.width() - 20;
    
    String remaining = text;
    while (remaining.length() > 0)
    {
      int newlinePos = remaining.indexOf('\n');
      String line;
      
      if (newlinePos >= 0) {
        line = remaining.substring(0, newlinePos);
        remaining = remaining.substring(newlinePos + 1);
      } else {
        line = remaining;
        remaining = "";
      }
      while (line.length() > 0)
      {
        String displayLine = line;
        int16_t textWidth = u8g2Fonts.getUTF8Width(displayLine.c_str());
        if (textWidth > maxWidth)
        {
          int lastSpace = -1;
          int testPos = line.length();
          
          while (testPos > 0)
          {
            testPos--;
            if (line.charAt(testPos) == ' ')
            {
              String testLine = line.substring(0, testPos);
              int16_t testWidth = u8g2Fonts.getUTF8Width(testLine.c_str());
              
              if (testWidth <= maxWidth)
              {
                lastSpace = testPos;
                break;
              }
            }
          }
          
          if (lastSpace > 0)
          {
            displayLine = line.substring(0, lastSpace);
            line = line.substring(lastSpace + 1);
          }
          else
          {
            while (textWidth > maxWidth && displayLine.length() > 0)
            {
              displayLine = displayLine.substring(0, displayLine.length() - 1);
              textWidth = u8g2Fonts.getUTF8Width(displayLine.c_str());
            }
            line = line.substring(displayLine.length());
          }
        }
        else
        {
          line = "";
        }
        
        textWidth = u8g2Fonts.getUTF8Width(displayLine.c_str());
        int16_t x = (display.width() - textWidth) / 2;
        u8g2Fonts.setCursor(x, currentY);
        u8g2Fonts.print(displayLine);
        currentY += lineHeight;
        if (currentY > display.height() - lineHeight)
        {
          remaining = "";
          break;
        }
      }
    }
  }
  while (display.nextPage());
  display.hibernate();
}

void handleRoot()
{
  String msg = "OK - ";
  msg += DEVICE_NAME;
  msg += " is running.";
  server.send(200, "text/plain", msg);
}

void handleUpdate()
{
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");
  Serial.print("[UPDATE] ");
  Serial.println(body);

  epdShowText(body);

  server.send(200, "text/plain; charset=utf-8", "OK");
}

void handleClear()
{
  Serial.println("[CLEAR]");
  epdClear();
  server.send(200, "text/plain", "OK");
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("===== EPD Node Boot =====");
  Serial.print("Device: ");
  Serial.println(DEVICE_NAME);

  SPI.begin(15, -1, 4, EPD_CS);
  display.init(115200);
  display.setRotation(1);
  u8g2Fonts.begin(display);
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  
  display.setFullWindow();
  display.clearScreen();
  
  epdShowText("啟動中...");

  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

#if USE_STATIC_IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Static IP failed, using DHCP");
  }
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  String ipMsg = "IP:\n" + WiFi.localIP().toString();
  epdShowText(ipMsg);

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.on("/clear", handleClear);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}
