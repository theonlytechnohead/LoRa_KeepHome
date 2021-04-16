//OLED pins
#define OLED_ADDRESS 0x3C
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define OLED_WIDTH 128 // OLED display width, in pixels
#define OLED_HEIGHT 64 // OLED display height, in pixels
#define OLED_FONT ArialMT_Plain_10

// Hardware variables
SSD1306 display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
OLEDDisplayUi ui(&display);

// Constants
enum UIs {
  UI_DEFAULT,
  UI_DETAILED,
  UI_EVERYTHING,
  UI_NOTHING,
  UI_RESET,
  LAST
};

// Local variables
enum UIs current_ui = UI_DEFAULT;
uint8_t ui_height = 14;
uint8_t timer_id;
DynamicJsonDocument lastDoc(2048);


void initOLED () {
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  vTaskDelay(20);
  digitalWrite(OLED_RST, HIGH);

  vTaskDelay(20);
  display.init();
  vTaskDelay(10);
  display.flipScreenVertically();
  vTaskDelay(10);
  display.setFont(OLED_FONT);
  vTaskDelay(10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  vTaskDelay(10);
  display.clear();
  vTaskDelay(10);
  display.display();
  vTaskDelay(50);
}

void drawUI () {
  //display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  switch (current_ui) {
    case UI_DEFAULT:
      ui_height = 14;
      display.setColor(BLACK);
      display.fillRect(0, 0, OLED_WIDTH, ui_height);
      display.setColor(WHITE);
      display.drawString(0, 0, "KeepHome");
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 0, String(LoRa.temperature()) + "°C");
      break;
    case UI_DETAILED:
      ui_height = 24;
      display.setColor(BLACK);
      display.fillRect(0, 0, OLED_WIDTH, ui_height);
      display.setColor(WHITE);
      display.drawString(0, 0, "Detailed UI");
      display.drawString(0, 10, getLocalIP());
      break;
    case UI_EVERYTHING:
      ui_height = OLED_HEIGHT;
      display.setColor(BLACK);
      display.fillRect(0, 0, OLED_WIDTH, ui_height);
      display.setColor(WHITE);
      display.drawString(0, 0, "EVERYTHING");
      display.drawString(0, 10, getLocalIP());
      break;
    case UI_NOTHING:
      ui_height = 1;
      display.setColor(BLACK);
      display.fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
      display.setColor(WHITE);
      break;
    case UI_RESET:
      ui_height = 14;
      display.setColor(BLACK);
      display.fillRect(0, 0, OLED_WIDTH, ui_height);
      display.setColor(WHITE);
      display.drawString(0, 0, "Hold PRG to reset");
      break;
  }
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawMessage (String message) {
  //drawUI();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setColor(BLACK);
  display.fillRect(0, 0, OLED_WIDTH, 14);
  display.setColor(WHITE);
  display.drawString(64, 0, message);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.display();
}

void drawPacket (DynamicJsonDocument doc) {
  //drawUI();
  display.setColor(BLACK);
  display.fillRect(0, ui_height, OLED_WIDTH, OLED_HEIGHT);
  display.setColor(WHITE);
  uint8_t y = ui_height;
  for (JsonPair pair : doc.as<JsonObject>()) {
    String output = pair.key().c_str();
    output += ": ";
    output += pair.value().as<String>();
    display.drawString(0, y, output);
    y += 10;
  }
  lastDoc = doc;
  display.display();
}

// -=- Getters and setters -=-
SSD1306* getDisplay () {
  return &display;
}

int getCurrentUI () {
  return current_ui;
}

uint8_t getUIheight () {
  return ui_height;
}

bool isShowingResetUI () {
  return current_ui == UI_RESET;
}

void setCurrentUI (UIs new_ui);
void setCurrentUI (UIs new_ui) {
  current_ui = new_ui;
  drawUI();
  display.display();
}

void cycleUI () {
  if (timer_id > 0)
    t.stop(timer_id);
  current_ui = (UIs)((int)current_ui + 1);
  if (current_ui == LAST) {
    current_ui = UI_DEFAULT;
  }
  drawUI();
  display.drawRect(0, 0, OLED_WIDTH, ui_height);
  drawPacket(lastDoc);
  timer_id = t.after(500, drawUICallback, (void*) NULL);
}

void drawUICallback (void* context) {
  if (getCurrentUI() != UI_NOTHING) {
    drawUI();
  } else {
    // Handle the special case of UI_NOTHING
    display.setColor(BLACK);
    display.fillRect(0, 0, OLED_WIDTH, 1);
    display.setColor(WHITE);
  }
  display.display();
}
