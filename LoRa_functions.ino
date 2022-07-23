//LoRa pins
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_FREQ 915E6
#define LORA_SYNCWORD 232 //HEX: 0xE8 // ranges from 0x00-0xFF, default 0x12, see API docs

// Global stuff
TaskHandle_t send_task;

void initLoRa () {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) {
    getDisplay() -> clear();
    getDisplay() -> setTextAlignment(TEXT_ALIGN_CENTER);
    getDisplay() -> drawString(64, 22, "LoRa borked");
    getDisplay() -> display();
    while (1) {
      rtc_wdt_feed(); // satiate the WDT
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  
  // Magic LoRa settings
  LoRa.sleep();
  LoRa.setSyncWord(LORA_SYNCWORD);
  LoRa.setTxPower(20); // (dB) max legally allowed and using PA_BOOST pin
  LoRa.setSpreadingFactor(10);
  LoRa.setCodingRate4(6);
  LoRa.setFrequency(LORA_FREQ);
  LoRa.setSignalBandwidth(125E3); // this SF, CR, and BW make for 433 bps (no, not even Kbps)
  LoRa.enableCrc();
  LoRa.idle();
}

// unused/deprecated
long setFrequencyCorrection () {
  // uint8_t temperature = LoRa.temperature();
  uint8_t temperature = 0;
  String temp_s = String(temperature) + "°C";
  long frequencyOffset = 150 * temperature;
  LoRa.setFrequency(LORA_FREQ + frequencyOffset);
  Serial.println(temp_s + " : " + String(frequencyOffset) + "MHz");
  return frequencyOffset;
}

// deprecated
void sendJSON (DynamicJsonDocument doc) {
  // Prepare for sending and turn on LED
  //setFrequencyCorrection();
  LoRa.idle();

  // Make packet
  char output[measureJson(doc) + 1];
  serializeJson(doc, output, sizeof(output));
  // TEMP?: Display packet
  drawPacket(doc);

  digitalWrite(2, HIGH);
  LoRa.beginPacket();
  LoRa.print(output);
  LoRa.endPacket();
  LoRa.receive();
  digitalWrite(2, LOW);
  
  // send packet
//  xTaskCreatePinnedToCore(
//      sendTask, // Function to implement the task
//      "LoRa send", // Name of the task
//      8192,  // Stack size in words, causes stack overflow if too low
//      output,  // Task input parameter
//      1,  // Priority of the task, 0 is lowest
//      &send_task,  // Task handle
//      0); // Core where the task should run, code runs on core 1 by default
}

// deprecated
void sendTask (void* parameter) {
  const char *output = (const char*) parameter;
  vTaskDelete(NULL);
}

// deprecated
void testSendLoRa (void* context) {
  const char *text = (const char*) context;
  sendJSON(createJSON(text));
}

StaticJsonDocument<256> getDoc (String text);
void testQueuePacket (void* context) {
  const char *text = (const char*) context;
  send_queue.push(getDoc(text));
}
