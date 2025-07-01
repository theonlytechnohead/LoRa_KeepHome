#include <LoRa.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include <ArduinoJson.h>
#include <Event.h>
#include <Timer.h>

#include "TinyUPnP.h"

// Global constants / variables
TaskHandle_t background_loop;

void (*state)(); // function pointer, to hold the current state (i.e. function to execute)

// Local-ish variables
Timer t;
bool button_pressed = false;
bool booted = false;
unsigned long start_millis = 0;

// ESP32 Arduino Core 3+ is incompatible with any serial baud rate when using TTGO LoRa OLED V1 boards:
// https://forum.arduino.cc/t/baudrate-changes-after-boot/1337352/18
// not tried: setting serial baud to 115200 in code and setting IDE to 74880
void setup () {
  Serial.begin(115200);
  pinMode(0, INPUT); // PRG button
  pinMode(2, OUTPUT); // Blue LED
  vTaskDelay(100);
  printMessage("system", "Booting...");

  initModule(initOLED, "Display");
  initModule(initLoRa, "LoRa");
  initModule(initLittleFS, "LittleFS");
  initModule(initNetwork, "Network");
  
  xTaskCreatePinnedToCore(
      backgroundLoop, // Function to implement the task
      "background loop", // Name of the task
      8192,  // Stack size in words, causes stack overflow if too low
      NULL,  // Task input parameter
      0,  // Priority of the task, 0 is lowest
      &background_loop,  // Task handle
      0); // Core where the task should run, code runs on core 1 by default
  
  while (!booted) {
    vTaskDelay(100);
  }
  
  printMessage("system", "Finished booting");
  drawMessage("Boot complete");

  vTaskDelay(500);

  drawUI();
  getDisplay() -> display();

  state = idle;
  const char *text = "Hello there";
//  t.every(5000, testSendLoRa, (void*) text);
  t.every(5000, testQueuePacket, (void*) text);
  t.after(60*1000, drawUICallback, (void*) NULL); // refresh UI after a minute
}

void loop () {
  handleButton();
  t.update();
  if (getWiFiMode() == 0) {
    getUPnP() -> updatePortMappings(600000);  // 10 minutes
  }
  if (digitalRead(0)) {
    state();
  }
}

void backgroundLoop (void* parameter) {
  for (;;) { // Infinite loop
    handleWebClient();
    // do delay immediately after handleWebClient because power saving
    vTaskDelay(10); // ESP32 defaults to 100Hz tick rate, so 10ms delay allows for 1 tick in order to run background tasks
  }
}
