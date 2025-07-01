#include <LittleFS.h>

void initLittleFS () {
  if (!LittleFS.begin()) {
    getDisplay() -> clear();
    getDisplay() -> setTextAlignment(TEXT_ALIGN_CENTER);
    getDisplay() -> drawString(64, 22, "LittleFS borked!");
    getDisplay() -> display();
  }
  listFiles();
}

void formatLittleFS () {
  LittleFS.format();
  restart();
}

void listFiles () {
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    if (String(file.name()) == "log.txt") { file = root.openNextFile(); continue; }
    if (String(file.name()) == "web") { file = root.openNextFile(); continue; }
    if (String(file.name()) == "password.txt") {
      printMessage("file", String(file.name()) + ": <REDACTED>");
      file = root.openNextFile();
      continue;
    }
    printMessage("file", String(file.path()) + ": '" + readFile(file.path()) + "'");
    file = root.openNextFile();
  }
}

// File operations
bool checkFile (const char* filename) {
  return LittleFS.exists(filename);
}


String readFile (const char* filename) {
  if (checkFile(filename)) {
    File file = LittleFS.open(filename, FILE_READ);
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
  File file = LittleFS.open(filename, FILE_WRITE);
  file.print(contents);
  file.close();
}

void writeFile (const char* filename, String contents) {
  writeFile(filename, contents.c_str());
}


void appendFile (const char* filename, const char* contents) {
  File file = LittleFS.open(filename, FILE_APPEND);
  file.println(contents);
  file.close();
}

void appendFile (const char* filename, String contents) {
  appendFile(filename, contents.c_str());
}

void removeFile (const char* filename) {
  LittleFS.remove(filename);
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
