#include <DHT.h>
#include <GyverOLED.h>
#include <OneButton.h>

#define BTN_PORT 7
#define DHT_PORT 8

#define CAROUSEL_LENGTH 2

DHT dht(DHT_PORT, DHT11);
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
OneButton button(BTN_PORT, true, true);

float h = -1;
float c = -1;
float f = -1;

unsigned long timer_read = 0;
unsigned long timer_print = 0;

int prev = CAROUSEL_LENGTH;
bool show_carousel = true;
bool temp_far = false;

void with_header(String title, void (*cb)()) {
   oled.rect(0, 0, 128, 11, 1);
   
   oled.setCursorXY(2, 2);
   
   oled.textMode(BUF_SUBTRACT);
   oled.print(title);
   oled.textMode(BUF_REPLACE);

   oled.setCursorXY(0, 20);
   cb();
}

void with_scaleup(int factor, void (*cb)()) {
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
  oled.roundRect(110, 60 - (60 - 23)*(h / 100), 124, 60, 1);
}

void print_temp() {
  if (temp_far)
    oled.print(fallback(f, 1) + "F");
  else
    oled.print(fallback(c, 1) + "C");
}

void print_settings() {
  oled.invertText(temp_far);
  oled.print("Fahrenheit");
  oled.invertText(false);
      
  oled.print("/");
      
  oled.invertText(!temp_far);
  oled.print("Celsius");
  oled.invertText(false);
}

void display(int screen) {
  oled.clear();
  oled.home();
  
  switch(screen) {
    case 0:
      with_header("Humidity", []() { with_scaleup(3, &print_hum); });
      break;
    case 1:
      with_header("Temperature", []() { with_scaleup(3, &print_temp); });
      break;
    case CAROUSEL_LENGTH:
      with_header("Settings", &print_settings);
      break;
  }
  
  oled.update();
}

void show_next() {
  timer_print = millis();
  prev = (prev + 1) % CAROUSEL_LENGTH;
  Serial.println("Next is " + String(prev));
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
  timer_print = millis();

  display(0);
}

void loop() {
  button.tick();
  
  oled.clear();
  
  every(4000, timer_read, &read_data);
  every(2000, timer_print, []() { if (show_carousel) show_next(); });
}
