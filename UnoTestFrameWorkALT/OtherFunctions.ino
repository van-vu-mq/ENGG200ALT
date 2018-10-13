
// Used to periodically check BlueTooth connection status
unsigned long timePrev = millis();
unsigned long timeCur;
unsigned long timeLapsed;
unsigned long period = 5000;  // Time (ms) between checks


void printBTStatus () {
  timeCur = millis();
  timeLapsed = timeCur - timePrev;

  if (timeLapsed >= period) {
    if (getConnectionStatus()) {
      Serial.println("Connected");
    } else {
      Serial.println("No Connection");
    }
    timePrev = millis();
  }
}

