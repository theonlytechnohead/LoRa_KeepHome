void initModule (void (*function)(), String module_name) {
  (*function)();
  printMessage("init", module_name + " initialized");
  drawMessage(module_name + " init");
  vTaskDelay(100);
}

void handleButton () {
  if (!digitalRead(0) && !button_pressed) {
    if (isShowingResetUI()) {
      start_millis = millis();
    } else {
      cycleUI();
    }
    button_pressed = true;
    vTaskDelay(10);
  }
  if (!digitalRead(0) && button_pressed && start_millis != 0) {
    if (millis() - 1000 >= start_millis) {
      Serial.println("Long press!");
      formatSPIFFS();
      start_millis = 0;
    }
  }
  if (digitalRead(0) && button_pressed) {
    button_pressed = false;
    if (start_millis != 0) {
      start_millis = 0;
      cycleUI();
    }
  }
}

String getFormattedMillis () {
  unsigned long current = millis();
  unsigned int seconds = current / 1000;
  unsigned int decimal = current % 1000;
  String seconds_s = String(seconds);
  while (seconds_s.length() < 5) { seconds_s = " " + seconds_s; }
  String decimal_f;
  if (decimal < 100) { decimal_f += "0"; }
  if (decimal < 10) { decimal_f += "0"; }
  decimal_f += String(decimal);
  String output = "[" + seconds_s + "." + String(decimal_f) + "]";
  return output;
}


void printMessage (const char* tag, String message) {
  printMessage(tag, message.c_str());
}
void printMessage (String tag, const char* message) {
  printMessage(tag.c_str(), message);
}
void printMessage (String tag, String message) {
  printMessage(tag.c_str(), message.c_str());
}
void printMessage (const char* tag, const char* message) {
  String tag_s = String(tag) + ":";
  while (tag_s.length() < 8) { tag_s += " "; }
  Serial.println(getFormattedMillis() + " " + tag_s + String(message));
}


void restart () {
  endMDNS();
  disconnectWiFi();
  getDisplay() -> clear();
  getDisplay() -> drawString(0, 0, "Restarting now...");
  getDisplay() -> display();
  vTaskDelay(1500);
  getDisplay() -> clear();
  vTaskDelay(100);
  ESP.restart();
}
