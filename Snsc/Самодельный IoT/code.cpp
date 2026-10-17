#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <FastLED.h>

const char* ssid = "Ninebot_MaxPlus_X"; 
const char* pass_to_ride = "1"; //СЮДА ВАШ ПАРОЛЬ

#define RELAY_IGNITION_PIN 18  
#define RELAY_DECK_PIN     19  
#define LED_HANDLEBAR_PIN  12  
#define LED_TAIL_PIN       13  

#define NUM_LEDS_HANDLEBAR 24  
#define NUM_LEDS_TAIL      32  

CRGB leds_handlebar[NUM_LEDS_HANDLEBAR];
CRGB leds_tail[NUM_LEDS_TAIL];

bool is_unlocked = false;
bool is_scooter_on = false;
uint8_t main_r = 150, main_g = 0, main_b = 255; 
uint8_t led_brightness = 140;
uint8_t effect_speed = 25;
uint8_t current_effect = 0; 

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);

void hexToRGB(String hex, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (hex.startsWith("#")) hex.remove(0, 1);
  long num = strtol(hex.c_str(), NULL, 16);
  r = num >> 16; g = num >> 8 & 0xFF; b = num & 0xFF;
}

String getPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  html += "<style>";
  html += "body{background:#050508;color:#e2e2e9;font-family:sans-serif;text-align:center;padding:15px;margin:0;}";
  html += ".card{background:#100f16;padding:25px 20px;border-radius:24px;max-width:340px;margin:20px auto;border:1px solid #3d2f5c;box-shadow:0 15px 50px rgba(138,43,226,0.2);}";
  html += "h1{color:#b392ff;font-size:24px;margin:0;font-weight:900;text-shadow:0 0 15px rgba(161,124,255,0.4);}";
  html += ".p-sub{color:#6c6c7a;margin:0 0 20px 0;font-size:11px;text-transform:uppercase;letter-spacing:2px;}";
  html += ".status{font-weight:bold;padding:15px;border-radius:18px;margin:20px 0;font-size:14px;letter-spacing:1px;text-transform:uppercase;}";
  html += ".locked{background:#261318;color:#ff5577;border:1px solid #592432;}";
  html += ".unlocked{background:#13261b;color:#34d399;border:1px solid #245937;}";
  html += "input[type='password']{width:80%;padding:15px;margin:15px 0;border:1px solid #3d355c;border-radius:14px;background:#060509;color:#fff;font-size:18px;text-align:center;outline:none;}";
  html += "button{width:92%;padding:16px;margin:10px 0;border:none;border-radius:16px;font-size:15px;font-weight:bold;text-transform:uppercase;cursor:pointer;transition:0.2s ease;}";
  html += ".btn-main{background:#8a2be2;color:#fff;}";
  html += ".btn-unl{background:#10b981;color:#fff;}";
  html += ".btn-loc{background:#d93856;color:#fff;}";
  html += ".btn-deck{background:#ff7b00;color:#fff;}";
  html += ".btn-action{background:#16141f;color:#a0a0ab;border:1px solid #2d293d;margin-top:20px;}";
  html += ".btn-fx{background:#15131c;color:#a0a0ab;border:1px solid #2d293d;}";
  html += ".btn-active{background:#a17cff!important;color:#050508!important;box-shadow:0 0 15px rgba(161,124,255,0.5)!important;}";
  html += "label{display:block;text-align:left;margin:15px 0 5px 15px;font-size:11px;color:#8f8aae;font-weight:700;text-transform:uppercase;}";
  html += ".rgb-box{background:#09080d;padding:15px;border-radius:18px;border:1px solid #231f33;margin-top:10px;}";
  html += "input[type='range']{width:92%;margin-bottom:12px;accent-color:#a17cff;background:#15131c;height:6px;border-radius:3px;display:block;margin:10px auto;}";
  html += "#r-slip{accent-color:#ff4545;}#g-slip{accent-color:#34d399;}#b-slip{accent-color:#00bfff;}";
  html += ".grid{display:flex;justify-content:space-between;width:92%;margin:0 auto;}";
  html += ".grid button{width:31%;font-size:11px;padding:12px 2px;border-radius:12px;}";
  html += "hr{border:0;border-top:1px solid #2b2640;margin:25px 0;}";
  html += "button:active{transform:scale(0.96);}</style></head><body>";
  
  html += "<div class='card'><h1>🔮 Max Plus Custom(7j6j7)</h1><div class='p-sub'>IoT System 4.3</div>";
  
  if (!is_unlocked) {
    html += "<div class='status locked'>🔒 ДОСТУП ОГРАНИЧЕН</div><form action='/auth' method='POST'>";
    html += "<input type='password' name='secret' placeholder='ВВЕДИТЕ ПИН-КОД' required><br>";
    html += "<button type='submit' class='btn-main'>🔑 ВОЙТИ</button></form>";
  } else {
    if (is_scooter_on) html += "<div class='status unlocked'>🔓 САМОКАТ ГОТОВ К ПОЕЗДКЕ</div>";
    else html += "<div class='status locked'>🔒 САМОКАТ ЗАБЛОКИРОВАН</div>";
    
    html += "<form action='/on' method='POST'><button type='submit' class='btn-unl'>🔓 РАЗБЛОКИРОВАТЬ</button></form>";
    html += "<form action='/off' method='POST'><button type='submit' class='btn-loc'>🔒 ЗАБЛОКИРОВАТЬ</button></form>";
    html += "<form action='/open_deck' method='POST'><button type='submit' class='btn-deck'>🧲 ОТКРЫТЬ ДЕКУ</button></form>";
    html += "<form action='/logout' method='POST'><button type='submit' class='btn-action'>🚪 ВЫЙТИ Из ПАНЕЛИ</button></form><hr>";
    
    html += "<h3 style='color:#a17cff;font-size:14px;margin:0;'>НАСТРОЙКА НЕОНА</h3><label>Палитра:</label>";
    html += "<form action='/set_color' method='POST'>";
    char hb[8]; sprintf(hb, "#%02X%02X%02X", main_r, main_g, main_b);
    html += "<input type='color' name='favcolor' value='" + String(hb) + "' onchange='this.form.submit()'></form>";
    html += "<label>HEX-код:</label><form action='/set_hex' method='POST'>";
    html += "<input type='text' name='hexval' placeholder='#8A2BE2' maxlength='7'>";
    html += "<button type='submit' class='btn-action' style='padding:8px;width:40%;font-size:11px;margin-top:5px;'>ОК</button></form>";
    html += "<form action='/set_sliders' method='POST'><label>Яркость:</label>";
    html += "<input type='range' name='brightness' min='10' max='255' value='" + String(led_brightness) + "' onchange='this.form.submit()'>";
    html += "<label>Скорость:</label>";
    html += "<input type='range' name='speed' min='5' max='100' value='" + String(effect_speed) + "' onchange='this.form.submit()'></form>";
    html += "<label>Режим:</label><div class='grid'>";
    html += "<button class='btn-fx " + String(current_effect == 0 ? "btn-active" : "") + "' onclick=\"location.href='/fx?id=0'\">Статик</button>";
    html += "<button class='btn-fx " + String(current_effect == 1 ? "btn-active" : "") + "' onclick=\"location.href='/fx?id=1'\">Радуга</button>";
    html += "<button class='btn-fx " + String(current_effect == 2 ? "btn-active" : "") + "' onclick=\"location.href='/fx?id=2'\">Волна</button></div>";
  }
  html += "</div></body></html>"; return html;
}
void setup() {
  pinMode(RELAY_IGNITION_PIN, OUTPUT); pinMode(RELAY_DECK_PIN, OUTPUT);
  digitalWrite(RELAY_IGNITION_PIN, HIGH); digitalWrite(RELAY_DECK_PIN, HIGH);     
  FastLED.addLeds<WS2812B, LED_HANDLEBAR_PIN, GRB>(leds_handlebar, NUM_LEDS_HANDLEBAR);
  FastLED.addLeds<WS2812B, LED_TAIL_PIN, GRB>(leds_tail, NUM_LEDS_TAIL);
  FastLED.setBrightness(led_brightness);
  WiFi.mode(WIFI_AP); WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); WiFi.softAP(ssid);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", getPage()); });
  server.on("/auth", HTTP_POST, []() {
    if (server.hasArg("secret") && server.arg("secret") == pass_to_ride) { is_unlocked = true; }
    server.send(200, "text/html", getPage());
  });
  server.on("/on", HTTP_POST, []() {
    if (is_unlocked) { is_scooter_on = true; digitalWrite(RELAY_IGNITION_PIN, LOW); }
    server.send(200, "text/html", getPage());
  });
  server.on("/off", HTTP_POST, []() {
    if (is_unlocked) { is_scooter_on = false; digitalWrite(RELAY_IGNITION_PIN, HIGH); }
    server.send(200, "text/html", getPage());
  });
  server.on("/open_deck", HTTP_POST, []() {
    if (is_unlocked) { digitalWrite(RELAY_DECK_PIN, LOW); delay(2500); digitalWrite(RELAY_DECK_PIN, HIGH); }
    server.send(200, "text/html", getPage());
  });
  server.on("/logout", HTTP_POST, []() {
    is_unlocked = false; is_scooter_on = false; digitalWrite(RELAY_IGNITION_PIN, HIGH);
    server.send(200, "text/html", getPage());
  });
  server.on("/set_color", HTTP_POST, []() {
    if (is_unlocked && server.hasArg("favcolor")) { hexToRGB(server.arg("favcolor"), main_r, main_g, main_b); current_effect = 0; }
    server.send(200, "text/html", getPage());
  });
  server.on("/set_hex", HTTP_POST, []() {
    if (is_unlocked && server.hasArg("hexval") && server.arg("hexval").length() >= 6) { hexToRGB(server.arg("hexval"), main_r, main_g, main_b); current_effect = 0; }
    server.send(200, "text/html", getPage());
  });
  server.on("/set_sliders", HTTP_POST, []() {
    if (is_unlocked) {
      if (server.hasArg("brightness")) { led_brightness = server.arg("brightness").toInt(); FastLED.setBrightness(led_brightness); }
      if (server.hasArg("speed")) effect_speed = server.arg("speed").toInt();
    }
    server.send(200, "text/html", getPage());
  });
  server.on("/fx", HTTP_GET, []() { if (is_unlocked && server.hasArg("id")) current_effect = server.arg("id").toInt(); server.send(200, "text/html", getPage()); });
  server.onNotFound([]() { server.sendHeader("Location", String("http://") + apIP.toString() + "/", true); server.send(302, "text/plain", ""); });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest(); server.handleClient();
  if (!is_scooter_on) {
    uint8_t b = beat8(12); uint8_t p = map(b, 0, 255, 15, 150); 
    fill_solid(leds_handlebar, NUM_LEDS_HANDLEBAR, CRGB(p * 0.4, 0, p));
    fill_solid(leds_tail, NUM_LEDS_TAIL, CRGB(p * 0.4, 0, p));
  } else {
    if (current_effect == 0) {
      fill_solid(leds_handlebar, NUM_LEDS_HANDLEBAR, CRGB(main_r, main_g, main_b));
      fill_solid(leds_tail, NUM_LEDS_TAIL, CRGB(main_r, main_g, main_b));
    } else if (current_effect == 1) {
      uint8_t h = beat8(effect_speed); 
      fill_rainbow(leds_handlebar, NUM_LEDS_HANDLEBAR, h, 8); fill_rainbow(leds_tail, NUM_LEDS_TAIL, h, 8);
    } else if (current_effect == 2) {
      uint8_t w = beat8(effect_speed);
      for (int i = 0; i < NUM_LEDS_HANDLEBAR; i++) {
        uint8_t bm = quadwave8(w + (i * 15)); 
        leds_handlebar[i] = CRGB(map(bm,0,255,0,main_r),map(bm,0,255,0,main_g),map(bm,0,255,0,main_b));
      }
      for (int i = 0; i < NUM_LEDS_TAIL; i++) {
        uint8_t bm = quadwave8(w + (i * 12)); 
        leds_tail[i] = CRGB(map(bm,0,255,0,main_r),map(bm,0,255,0,main_g),map(bm,0,255,0,main_b));
      }
    }
  }
  FastLED.show();
}
