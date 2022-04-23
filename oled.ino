#include <DHT.h>
#include <GyverOLED.h>
#include <OneButton.h>

#define BTN_PORT 7
#define DHT_PORT 8
#define PHOTO_RESISTOR A0

#define READ_INTERVAL 4000
#define CAROUSEL_INTERVAL 2000

#define CAROUSEL_LENGTH 3

DHT dht(DHT_PORT, DHT11);
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
OneButton button(BTN_PORT, true, true);

const unsigned char bmp_moon [] PROGMEM = {
  0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfc, 0xfe, 0xfe, 0xff, 0x01, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xf0, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf0, 0xc0, 0x80, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x0f, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xfe, 0xfc, 0xfc, 0xf8, 0xf8, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x70, 0x18, 
  0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x3f, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0x3f, 0x1f, 0x0f, 0x0f, 0x03, 0x01, 0x00, 0x00
  };

  const unsigned char bmp_sun [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x20, 0x70, 0xe0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 
  0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x20, 0x00, 0x00, 0x00, 0x00, 
  0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x01, 0xe0, 0xf8, 0xfc, 0x1e, 0x0e, 0x07, 0x07, 0x07, 
  0x07, 0x07, 0x07, 0x0e, 0x1e, 0xfc, 0xf8, 0xe0, 0x01, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 
  0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x80, 0x07, 0x1f, 0x3f, 0x78, 0x70, 0xe0, 0xe0, 0xe0, 
  0xe0, 0xe0, 0xe0, 0x70, 0x78, 0x3f, 0x1f, 0x07, 0x80, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 
  0x00, 0x00, 0x00, 0x00, 0x04, 0x0e, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 
  0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0e, 0x04, 0x00, 0x00, 0x00, 0x00
};

float h = -1;
float c = -1;
float f = -1;
float l = -1;

unsigned long timer_read = 0;
unsigned long timer_carousel = 0;

byte prev = CAROUSEL_LENGTH;
bool show_carousel = true;
bool temp_far = false;

void with_header(const String& title, void (*cb)()) {
   oled.rect(0, 0, 128, 11, 1);
   
   oled.setCursorXY(2, 2);
   
   oled.textMode(BUF_SUBTRACT);
   oled.print(title);
   oled.textMode(BUF_REPLACE);

   oled.setCursorXY(2, 20);
   cb();
}

void with_scaleup(int factor, void (*cb)()) {
  oled.setCursorXY(2, 29);
  oled.setScale(factor);
  cb();
  oled.setScale(1);
}

String fallback (float n, int precision = 2) {
  return isnan(n) ? "IDK" : String(n, precision);  
}

void print_hum() {
  oled.print(fallback(h, 0) + "%");
  
  oled.roundRect(107, 20, 127, 63, 1);
  oled.roundRect(109, 22, 125, 61, 0);
  if (h > 0)
    oled.roundRect(110, 60 - (60 - 23)*(h / 100), 124, 60, 1);
}

void print_temp() {
  if (temp_far)
    oled.print(fallback(f, 1) + "F");
  else
    oled.print(fallback(c, 1) + "C");
}

void print_light() {
  bool is_night = l > 450;

  String str = F("It's ");
  if (is_night)
    str += F("night");
  else
    str += F("day");

  oled.setCursorXY((127 - str.length()*6)/2, 55);
  oled.println(str);
  
  oled.drawBitmap(48, 17, is_night ? bmp_moon : bmp_sun, 32, 32, BITMAP_NORMAL, BUF_ADD);
}

void print_settings() {
  oled.invertText(temp_far);
  oled.print(F("Fahrenheit"));
  oled.invertText(false);
      
  oled.print("/");
      
  oled.invertText(!temp_far);
  oled.print(F("Celsius"));
  oled.invertText(false);
}

void display(byte screen) {
  oled.clear();
  oled.home();
  
  switch(screen) {
    case 0:
      with_header(F("Humidity"), []() { with_scaleup(3, &print_hum); });
      break;
    case 1:
      with_header(F("Temperature"), []() { with_scaleup(3, &print_temp); });
      break;
    case 2:
      with_header(F("Lightness"), &print_light);
      break;
    case CAROUSEL_LENGTH:
      with_header(F("Settings"), &print_settings);
      break;
  }
  
  oled.update();
}

void show_next() {
  timer_carousel = millis();
  prev = (prev + 1) % CAROUSEL_LENGTH;
  display(prev);
}

void every(int del, unsigned long& timer, void (*cb)()) {
  if (millis() - timer >= del) {
    cb();
    timer = millis();
  }
}

void read_data() {
  h = dht.readHumidity();
  c = dht.readTemperature();
  f = dht.readTemperature(true);

  l = analogRead(PHOTO_RESISTOR);
}

void handle_click() {
  if (show_carousel) {
    show_next();  
  } else {
    temp_far = !temp_far;
    display(CAROUSEL_LENGTH);
  }
}

void handle_lpress() {
  if (show_carousel) {
    show_carousel = false;
    display(CAROUSEL_LENGTH);
  }
  else {
    show_carousel = true;
    show_next();
  }
}

void setup() {
  Serial.begin(9600);

  dht.begin();

  read_data();
  
  oled.init();
  oled.clear();
  oled.update();

  button.attachClick(&handle_click);
  button.attachLongPressStart(&handle_lpress);
  
  timer_read = millis();
  timer_carousel = millis();

  display(0);
}

void loop() {
  button.tick();
  
  oled.clear();
  
  every(READ_INTERVAL, timer_read, &read_data);
  every(CAROUSEL_INTERVAL, timer_carousel, []() { if (show_carousel) show_next(); });
}
