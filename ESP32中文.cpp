#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// WiFi 設定
const char* WIFI_SSID     = "此網路不符合WPA2協議";
const char* WIFI_PASSWORD = "QooQooQoo";

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
    
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    int16_t textWidth = u8g2Fonts.getUTF8Width(text.c_str());
    int16_t x = (display.width() - textWidth) / 2;
    int16_t y = display.height() / 2;
    
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.print(text);
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
