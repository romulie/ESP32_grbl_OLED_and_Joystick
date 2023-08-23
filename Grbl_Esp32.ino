/*
  Grbl_ESP32.ino - Header for system level commands and real-time processes
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

	2018 -	Bart Dring This file was modified for use on the ESP32
					CPU. Do not use this with Grbl for atMega328P

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

// This version also contains OLED display (shows current coordinates and delta step value)
// and x-y joystick w\pushbutton (controls 2d-cnc table)

#include "src/Grbl.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define TASK_UPD_POS_DELAY_MS 3000
#define TASK_JOYSTICK_DELAY_MS 250
#define RXD2 16 // Serial2 to debug grbl_esp32
#define TXD2 17 // Serial2 to debug grbl_esp32
#define SCREEN_ADDRESS 0x3C // OLED display address connected to ESP32 default I2C GPIO 21(SDA) and GPIO 22 (SCL)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define J_X 34 // Joystick x input
#define J_Y 35 // Joystick y input
#define J_BTN 15 // Joystick button input


#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)

float x, y, z;
uint16_t j_x, j_y;
String cmd;
float delta = 100.0;

void show(float x, float y, float z, float delta, const String& cmd){
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print(F("X = ")); display.println(x);
  display.print(F("Y = ")); display.println(y);
  display.print(F("Z = ")); display.println(z);
  display.println();
  display.print(F("Delta = ")); display.println(delta);
  display.println();
  display.print(F("CMD = ")); display.println(cmd);
  display.display();
}

void Update_Joystick_Task(void *arg){
  while(1){
    j_x = analogRead(J_X);
    j_y = analogRead(J_Y);
    bool j_btn = digitalRead(J_BTN);

    if (j_btn == LOW) {
      delta /= 10.0;
      if (delta < 0.1)
        delta = 100.0;
    }

    String delta_str(delta);// TO DO: remove String manipulations, do the same with char*
    cmd = "G91 ";
    
    if      (j_x < 1000)    { cmd += ("X-" + delta_str); }
    else if (3000 < j_x)    { cmd += ("X" + delta_str); }
    
    if      (j_y < 1000)    { cmd +=  (" Y-" + delta_str); }
    else if (3000 < j_y)    { cmd += (" Y" + delta_str); }

    if (cmd.length() > 4) {
      cmd +=  "\n";
      for (size_t i = 0; i < cmd.length(); ++i){
        WebUI::inputBuffer.write((uint8_t)cmd[i]); // send cmd to commands inputBuffer
      }
    }

    show(x, y, z, delta, cmd);
    
    vTaskDelay(TASK_JOYSTICK_DELAY_MS / portTICK_RATE_MS);
  }
}

void Serial_DysplayXYZ_Task(void *arg){
  while(1){
    x = system_convert_axis_steps_to_mpos(sys_position, 0);//steps[0]??
    y = system_convert_axis_steps_to_mpos(sys_position, 1);
    z = system_convert_axis_steps_to_mpos(sys_position, 2);
    
    Serial2.print(F("******* X = ")); Serial2.print(x);
    Serial2.print(F(" ; Y = ")); Serial2.print(y);
    Serial2.print(F(" ; Z = ")); Serial2.print(z);
    Serial2.print(F(" ; state = ")); Serial2.print((uint8_t)sys.state);

    show(x, y, z, delta, cmd);
    
    vTaskDelay(TASK_UPD_POS_DELAY_MS / portTICK_RATE_MS);
  }
}


void setup() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }
    display.display(); // boot screen
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    grbl_init();
    xTaskCreate(Serial_DysplayXYZ_Task, "Serial_DysplayXYZ_Task", 4000, NULL, 1, NULL);
    
    pinMode(J_X, INPUT);
    pinMode(J_Y, INPUT);
    pinMode(J_BTN, INPUT_PULLUP);
    xTaskCreate(Update_Joystick_Task, "Update_Joystick_Task", 10000, NULL, 2, NULL);
}

void loop() {
    run_once();
}
