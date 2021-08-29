// implement FSM!
// link/source: https://www.youtube.com/watch?v=pxaIyNbcPrA

#include <CircularBuffer.h>
CircularBuffer<StaticJsonDocument<256>,4> send_queue;
void (*prev)(); // stores the previous state, to aid in state transitions

void idle() {
  if (prev != state) {
    printMessage("lora", "state=idle");
    digitalWrite(2, LOW); // LED off
    LoRa.receive();
    prev = state;
  }
  
  if (!send_queue.isEmpty()) {
    prev = state;
    state = send;
    return;
  } else {
    vTaskDelay(10);
  }
}

void send() {
  printMessage("lora", "state=send");

  StaticJsonDocument<256> doc = send_queue.shift(); // pop from the front
  // temp, rewrite
  
  char output[measureJson(doc) + 1];
  serializeJson(doc, output, sizeof(output));
  // TEMP?: Display packet
  drawPacket(doc);

  digitalWrite(2, HIGH);
  LoRa.beginPacket();
  LoRa.print(output);
  LoRa.endPacket();
  LoRa.receive();
  // fix up later
  prev = state;
  state = idle;
  return;
}
