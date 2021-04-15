#include <LoRa.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include "soc/rtc_wdt.h" // FreeRTOS WDT control for ESP32
#include <ArduinoJson.h>
#include "Timer.h"

// Global constants / variables
TaskHandle_t background_loop;

enum LORA_COMMANDS {
  ONE, TWO, THREE
};

// Local-ish variables
Timer t;
bool button_pressed = false;
bool booted = false;
unsigned long start_millis = 0;

void setup () {
  Serial.begin(115200);
  pinMode(0, INPUT); // PRG button
  pinMode(2, OUTPUT); // Blue LED
  vTaskDelay(100);
  printMessage("system", "Booting...");

  initModule(initOLED, "Display");
  initModule(initLoRa, "LoRa");
  initModule(initSPIFFS, "SPIFFS");
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
  
  const char *text = "Hello there";
  t.every(5000, testSendLoRa, (void*) text);
  t.after(60*1000, drawUICallback, (void*) NULL); // refresh UI every minute
}

void loop () {
  handleButton();
  t.update();
}

void backgroundLoop (void* parameter) {
  for (;;) { // Infinite loop
    vTaskDelay(10); // ESP32 defaults to 100Hz tick rate, so 10ms delay allows for 1 tick in order to run background tasks
    handleWebClient();
  }
}
