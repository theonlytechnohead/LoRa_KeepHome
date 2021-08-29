// deprecated
DynamicJsonDocument createJSON (String text) {
  DynamicJsonDocument doc(2048);
  doc["millis"] = millis();
  doc["text"] = text;
  return doc;
}

// tweak size as necessary
StaticJsonDocument<256> getDoc (String text) {
  StaticJsonDocument<256> doc;
  doc["millis"] = millis();
  doc["text"] = text;
  return doc;
}
