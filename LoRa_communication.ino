// implement FSM!
// link/source: https://www.youtube.com/watch?v=pxaIyNbcPrA

//#include "mbedtls/md.h"

#include <CircularBuffer.hpp>

CircularBuffer<StaticJsonDocument<256>,4> send_queue;

void (*prev)(); // stores the previous state, to aid in state transitions

int count = 0;
int packetSize = 0;

void idle () {
  if (prev != state) {
    printMessage("lora", "idle");
//    digitalWrite(2, LOW); // LED off
    LoRa.receive();
    prev = state;
  }
  
  if (!send_queue.isEmpty()) {
    prev = state;
    state = send;
    return;
  } else {
    packetSize = LoRa.parsePacket();
    if (packetSize) {
      prev = state;
      state = receive;
      return;
    }
  }
}

void send () {
  printMessage("lora", "sending packet " + String(count));

  StaticJsonDocument<256> doc = send_queue.first(); // pull from the front
  doc["count"] = count;
  
  char output[measureJson(doc) + 1];
  serializeJson(doc, output, sizeof(output));
  // TEMP?: Display packet
  drawPacket(doc);

  digitalWrite(2, HIGH);
  LoRa.beginPacket();
  LoRa.print(output);
  LoRa.endPacket();
  digitalWrite(2, LOW);
  
  prev = state;
  state = wait;
  return;
}

unsigned long startWaitTime;
unsigned short retries = 0; // for retransmit
void wait () {
  if (prev != state) {
    printMessage("lora", "wait");
    startWaitTime = millis();
    LoRa.receive();
    prev = state;
  }

  // try receiving
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String packet;
    for (int i = 0; i < packetSize; i++) {
      packet += (char) LoRa.read();
    }
    if (packet == "ACK") {
      send_queue.shift(); // dequeue (pop)
      retries = 0;
      printMessage("lora", "packet " + String(count) + " ACK'd");
      count++;
      prev = state;
      state = idle;
      return;
    } else { // we didn't get a proper ACK, try again
      prev = state;
      state = retransmit;
      return;
    }
  }

  if (millis() - startWaitTime >= 1000) { // check timeout
    prev = state;
    state = retransmit;
    return;
  }
}

void retransmit () {
  printMessage("lora", "retransmitting, retried " + String(retries) + " times");
  // check retries
  if (retries < 3) {
    retries++;
    // retransmit
    prev = state;
    state = send;
    return;
  } else {
    printMessage("lora", "dropped packet " + String(count) + " after 3 retries");
    count++;
    retries = 0;
    prev = state;
    state = idle;
    return;
  }
}

void receive () {
  printMessage("lora", "receiving");
  String packet;
  for (int i = 0; i < packetSize; i++) {
    packet += (char) LoRa.read();
  }
  
  // TODO: verify checksum, nak on fail

  printMessage("lora", "received message, " + packet);

  prev = state;
  state = ack;
  return;
}

void ack () {
  printMessage("lora", "ack");
  digitalWrite(2, HIGH);
  LoRa.beginPacket();
  LoRa.print("ACK");
  LoRa.endPacket();
  digitalWrite(2, LOW);

  prev = state;
  state = idle;
  return;
}

void nak () {
  printMessage("lora", "nak");
  digitalWrite(2, HIGH);
  LoRa.beginPacket();
  LoRa.print("NAK");
  LoRa.endPacket();
  digitalWrite(2, LOW);
  
  prev = state;
  state = idle;
  return;
}
