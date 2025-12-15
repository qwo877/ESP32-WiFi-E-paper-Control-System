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

    int16_t x1, y1;
    uint16_t w, h;
    String t = text;

    display.getTextBounds(t, 0, 0, &x1, &y1, &w, &h);

    int16_t x = (display.width()  - w) / 2;
    int16_t y = (display.height() + h) / 2;

    display.setCursor(x, y);
    display.print(t);
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
