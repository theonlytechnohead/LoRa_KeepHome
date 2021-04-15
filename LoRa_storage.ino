#include <SPIFFS.h>

void initSPIFFS () {
  if (!SPIFFS.begin()) {
    getDisplay() -> clear();
    getDisplay() -> setTextAlignment(TEXT_ALIGN_CENTER);
    getDisplay() -> drawString(64, 22, "SPIFFS borked");
    getDisplay() -> display();
  }
  listFiles();
}

void formatSPIFFS () {
  SPIFFS.format();
  restart();
}

void listFiles () {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    if (String(file.name()) == "/log.txt") { file = root.openNextFile(); continue; }
    printMessage("file", String(file.name()) + ": '" + readFile(file.name()) + "'");
    file = root.openNextFile();
  }
}

// File operations
bool checkFile (const char* filename) {
  return SPIFFS.exists(filename);
}


String readFile (const char* filename) {
  if (checkFile(filename)) {
    File file = SPIFFS.open(filename, FILE_READ);
    String contents;
    while (file.available()) {
      contents += char(file.read());
    }
    file.close();
    return contents;
  } else {
    printMessage("file", "Can't find file: " + String(filename));
    return "";
  }
}


void writeFile (const char* filename, const char* contents) {
  File file = SPIFFS.open(filename, FILE_WRITE);
  file.println(contents);
  file.close();
}

void writeFile (const char* filename, String contents) {
  writeFile(filename, contents.c_str());
}


void appendFile (const char* filename, const char* contents) {
  File file = SPIFFS.open(filename, FILE_APPEND);
  file.println(contents);
  file.close();
}

void appendFile (const char* filename, String contents) {
  appendFile(filename, contents.c_str());
}

void removeFile (const char* filename) {
  SPIFFS.remove(filename);
}


// Utility functions
void incrementReboots () {
  String contents = "[" + getNow() + "] Reboooted";
  appendFile("/log.txt", contents);
}

void incrementDroppedPackets () {
  String contents = "[" + getNow() + "] Dropped packet";
  appendFile("/log.txt", contents);
}
