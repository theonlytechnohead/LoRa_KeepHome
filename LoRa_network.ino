#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "TimeLib.h"

DNSServer dnsServer;
IPAddress ip;

void initNetwork () {
  if (checkFile("/ap_mode.txt")) {
    String ap_mode = readFile("/ap_mode.txt");
    if (ap_mode == "1") {
      printMessage("WiFi", "Initializing AP mode");
      initAP();
    } else {
      printMessage("WiFi", "Initializing client mode");
      initWiFi();
    }
  } else {
    writeFile("/ap_mode.txt", "1");
    printMessage("WiFi", "Initializing AP mode");
    initAP();
  }
}

// Cuz Arduino IDE is a dumb-dumb
void onWiFiIP (WiFiEvent_t event, WiFiEventInfo_t info);

// Connect to WiFi part 1
void initWiFi () {
  WiFi.onEvent(onWiFiIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  String ssid = getSSID();
  String password = getPassword();
  printMessage("WiFi", "Connecting to " + ssid + "...");
  WiFi.begin(ssid.c_str(), password.c_str());
}

// Connect to WiFi part 2
void onWiFiIP (WiFiEvent_t event, WiFiEventInfo_t info) {
  ip = WiFi.localIP();
  initMDNS();
}

// Make WiFi
void initAP () {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(getSSID().c_str(), getPassword().c_str());
  ip = WiFi.softAPIP();
  dnsServer.start(53, "*", ip);
  initMDNS();
}

// Do WiFi
void initMDNS () {
  printMessage("WiFi", "Connected to " + WiFi.SSID());
  printMessage("WiFi", "IP address: " + ip.toString());
  if (MDNS.begin("KeepHome")) {
    printMessage("WiFi", "MDNS responder started: KeepHome.local");
  }
  MDNS.addService("_http", "_tcp", 80);
  initModule(initNTP, "NTP");
  
  drawUI();
  getDisplay() -> display();
  
  initWebserver();
}

// Do Webserver
void initWebserver () {
  
}


// Utility functions
String getSSID () {
  if (checkFile("/ssid.txt")) {
    String ssid = readFile("/ssid.txt");
    return ssid;
  } else {
    writeFile("/ssid.txt", "KeepHome");
    return String("KeepHome");
  }
}

String getPassword () {
  if (checkFile("/password.txt")) {
    String password = readFile("/password.txt");
    return password;
  } else {
    String password;
    for (int i = 0; i < 8; i++) {
      password += '0' + random(10);
    }
    writeFile("/password.txt", password);
    return password;
  }
}

IPAddress getIP (String server) {
  IPAddress result;
  int error = WiFi.hostByName(server.c_str(), result) ;
  if(error != 1) {
    printMessage("WiFi", "getIP() error: " + String(error));
  }
  return result;
}

void disconnectWiFi () {
  WiFi.disconnect();
}

void endMDNS () {
  MDNS.end();
}
