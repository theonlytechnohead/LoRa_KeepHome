
DynamicJsonDocument createJSON (String text) {
  DynamicJsonDocument doc(2048);
  doc["millis"] = millis();
  doc["text"] = text;
  return doc;
}
