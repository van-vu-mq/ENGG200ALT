
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


void printReceivedData(String *data, int dataSize) {
  Serial.println("Retrieved Data:");
  for (int i = 0; i < dataSize; i++) {
    Serial.println(*(data + i));
  }
}


void sendTestData() {
  Serial.println("\n---------------- Send Test Begin\n");

  String testData[] = {"one", "two", "test", "234324", "453sdf3243"};

  Serial.println("Test array being sent: ");
  for (int i = 0; i < sizeof(testData) / sizeof(testData[0]); i++) {
    Serial.println(testData[i]);
  }
  sendData(testData, sizeof(testData) / sizeof(testData[0]));

  Serial.println("\n----------------- Send Test End\n");
}


void receiveTestData() {
  Serial.println("\n================= Receive Test Begin\n");
  
  if (receivedNewData()) {
    Serial.println("\nMessage retrieved from memory:");
    String *message = getBTData();
    int messageSize = getBTDataSize();
    for (int i = 0; i < messageSize; i++) {
      Serial.println(*(message + i));
    }

    
    Serial.println("\nClearing memory");
    clearMemory();
    message = getBTData();
    messageSize = getBTDataSize();

    
    Serial.println("\nReading last recieved transmission");
    for (int i = 0; i < messageSize; i++) {
      Serial.println(*(message + i));
    }
  }
  Serial.println("\n================= Receive Test End\n");
}



