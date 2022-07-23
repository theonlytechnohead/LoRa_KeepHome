WiFiUDP ntp_udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
static const char ntpServerName[] = "nz.pool.ntp.org";
const int timeZone = 12; // UTC+12 (NZST)
//const int timeZone = 13; // UTC+13 (NZDT)


const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte ntpPacketBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets


void initNTP () {
  ntp_udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300); // Seconds between re-sync
  //incrementReboots();
}

/*-------- NTP code ----------*/
time_t getNtpTime () {
  IPAddress ntpServerIP; // NTP server's ip address

  while (ntp_udp.parsePacket() > 0) ; // discard any previously received packets
  // get a random server from the pool
  ntpServerIP = getIP(String(ntpServerName));
  printMessage("ntp", "Transmit NTP request to " + String(ntpServerName) + " (" + ntpServerIP.toString() + ")");
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = ntp_udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      printMessage("ntp", "Received NTP response (time sync'd)");
      ntp_udp.read(ntpPacketBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)ntpPacketBuffer[40] << 24;
      secsSince1900 |= (unsigned long)ntpPacketBuffer[41] << 16;
      secsSince1900 |= (unsigned long)ntpPacketBuffer[42] << 8;
      secsSince1900 |= (unsigned long)ntpPacketBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  printMessage("ntp", "No NTP response");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket (IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  ntpPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
  ntpPacketBuffer[1] = 0;     // Stratum, or type of clock
  ntpPacketBuffer[2] = 6;     // Polling Interval
  ntpPacketBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  ntpPacketBuffer[12] = 49;
  ntpPacketBuffer[13] = 0x4E;
  ntpPacketBuffer[14] = 49;
  ntpPacketBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  ntp_udp.beginPacket(address, 123); //NTP requests are to port 123
  ntp_udp.write(ntpPacketBuffer, NTP_PACKET_SIZE);
  ntp_udp.endPacket();
}

// Utility functions

String getNow () {
  time_t t = now();
  String timeNow;
  timeNow += year(t);
  timeNow += "/";
  int months = month(t);
  if (months < 10) {
    timeNow += "0";
  }
  timeNow += months;
  timeNow += "/";
  int days = day(t);
  if (days < 10) {
    timeNow += "0";
  }
  timeNow += days;
  timeNow += " ";
  int hours = hour(t);
  if (hours < 10) {
    timeNow += "0";
  }
  timeNow += hours;
  timeNow += ":";
  int minutes = minute(t);
  if (minutes < 10) {
    timeNow += "0";
  }
  timeNow += minutes;
  timeNow += ":";
  int seconds = second(t);
  if (seconds < 10) {
    timeNow += "0";
  }
  timeNow += seconds;
  return timeNow;
}
