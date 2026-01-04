#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Adafruit_GFX.h>
const char* WIFI_SSID     = "WiFi name";
const char* WIFI_PASSWORD = "WiFi key";

//#define USE_STATIC_IP 1
#if USE_STATIC_IP
IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

const char* DEVICE_NAME = "EPD-Node-2";

static const uint8_t EPD_CS   = 3;
static const uint8_t EPD_DC   = 14;
static const uint8_t EPD_RST  = 2;
static const uint8_t EPD_BUSY = 17;

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(
  GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);


WebServer server(80);
void epdClear()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
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
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold24pt7b);
    display.setTextSize(1);

    int lineHeight = 35;
    int yStart = 50;
    int currentY = yStart;
    int maxWidth = display.width() - 20;
    
    String remaining = text;
    
    while (remaining.length() > 0 && currentY < display.height() - lineHeight)
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
      while (line.length() > 0 && currentY < display.height() - lineHeight)
      {
        String displayLine = line;
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(displayLine.c_str(), 0, 0, &x1, &y1, &w, &h);
        if (w > maxWidth)
        {
          int lastSpace = -1;
          int testPos = line.length();
          
          while (testPos > 0)
          {
            testPos--;
            if (line.charAt(testPos) == ' ')
            {
              String testLine = line.substring(0, testPos);
              display.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w, &h);
              
              if (w <= maxWidth)
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
            display.getTextBounds(displayLine.c_str(), 0, 0, &x1, &y1, &w, &h);
          }
          else
          {
            while (w > maxWidth && displayLine.length() > 0)
            {
              displayLine = displayLine.substring(0, displayLine.length() - 1);
              display.getTextBounds(displayLine.c_str(), 0, 0, &x1, &y1, &w, &h);
            }
            
            if (displayLine.length() == 0) break;
            line = line.substring(displayLine.length());
          }
        }
        else
        {
          line = "";
        }
        display.getTextBounds(displayLine.c_str(), 0, 0, &x1, &y1, &w, &h);
        int16_t x = (display.width() - w) / 2;
        
        display.setCursor(x, currentY);
        display.print(displayLine);

        currentY += lineHeight;
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
  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");
  Serial.print("[HTTP] /update body = ");
  Serial.println(body);

  epdShowText(body);

  server.send(200, "text/plain", "Updated: " + body);
}



void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("===== EPD Node Boot =====");
  Serial.print("Device: ");
  Serial.println(DEVICE_NAME);

  SPI.begin(15,-1 ,4 ,EPD_CS);

  display.init(115200);
  display.setRotation(1);
  display.setFullWindow();
  display.clearScreen(); 
  epdShowText("Booting...");

  // 連 WiFi
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

#if USE_STATIC_IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("WiFi.config failed, using DHCP instead");
  }
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  String bootMsg = "IP:\n" + WiFi.localIP().toString();
  epdShowText(bootMsg);

  // 設定 HTTP 路由
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop()
{
  server.handleClient();

}
