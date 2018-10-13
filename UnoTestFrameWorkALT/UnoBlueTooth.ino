/*
  Source code for the Uno BlueTooth library.
  Functions outlined in the header file is implement here.
*/

#include <AltSoftSerial.h>
#include <CRC32.h>

#define connectionStatusPin 13

#define packetStartMarker   '<'
#define packetEndMarker     '>'

#define dataStartMarker     '!'
#define dataEndMarker       '@'

#define lineStartMarker     '#'
#define lineEndMarker       '$'

#define checksumStartMarker '&'
#define checksumEndMarker   '*'


AltSoftSerial BTSerial;
String MegaMAC = "";

String *storedTransmission;
int storedSize = 0;
String btbuffer = "";

// Change to false to reduce global variables
boolean includeErrorMessage = false;
boolean testingMessages = false;
boolean receiveTesting = false;


/************************************************************************************************************************/
/************************/
/*    Initialize        */
/************************/
/************************************************************************************************************************/

/*
  @desc Initialize the BlueTooth connections
  @param int baudRate
  @return
*/
void beginBluetooth(int baudRate) {
  pinMode(connectionStatusPin, INPUT);
  Serial.begin(baudRate);
  while (!Serial);
  if (includeErrorMessage) {
    Serial.print("\nSketch:   ");   Serial.println(__FILE__);
    Serial.print("Uploaded: ");   Serial.println(__DATE__);
  }


  BTSerial.begin(baudRate);
  if (includeErrorMessage) {
    Serial.println("BTserial started at " + String(baudRate));
  }
  doATCommandSetup();
}



/************************************************************************************************************************/
/************************/
/*  Settings & Status   */
/************************/
/************************************************************************************************************************/

/*
  @desc Returns a pointer to the array holding the last received transmission
  @param
  @return String *pointer
*/
String * getBTData() {

}

/*
  @desc Returns the number of elements in the last received transmission
  @param
  @return int storedSize
*/
int getBTDataSize() {

}

/*
  @desc Delete the last received transmission to free up memory
  @param
  @return
*/
void clearMemory() {

}

/*
  @desc Returns the paired status of the BlueTooth module.
  @param
  @return boolean - true if paired, false if not paired
*/
boolean getConnectionStatus() {
  /*
     Polls the state pin several times and checks whether it is BLINKING or HIGH.
     HM-10 BLE module BLINKs every 500ms when not paired.
     Polling needs to have sufficient fidelity to account for this timing.
  */
  int pollCount = 7;  // number of times to poll state pin
  int pollDelay = 100; // period (ms) between polls

  while (pollCount > 0) {
    if (!digitalRead(connectionStatusPin)) {
      return false;
    }
    pollCount--;
    delay(pollDelay);
  }
  return true;
}

/*
  @desc Pairs the BTLE with the device correseponding to the stored MAC address.
  @param
  @return boolean - true if pairing successfull
  @return boolean - false if pairing unsuccessfull
*/
boolean connectBluetooth() {
  if (!canDoAT()) {
    return false;
  }
  String successFlags[] = {"OK", "Set"};
  String response = "";

  BTSerial.print("AT+CON" + MegaMAC);

  response = atResponse();

  int numFlags = sizeof(successFlags) / sizeof(successFlags[0]);
  if (isATSucessfull(response, successFlags, numFlags)) {
    if (includeErrorMessage) {
      Serial.println("Bluetooth has been connected");
    }
    return true;
  } else {
    if (includeErrorMessage) {
      Serial.println("Bluetooth failed to connect");
    }
    return false;
  }
}

/*
  @desc Executes a predefined set of AT commands
  @param
  @return
*/
void doATCommandSetup() {
  if (canDoAT()) {
    changeRole(0);
    changeName("UnoBluetooth");
  }
}

/*
  @desc Changes the name of the Bluetooth module to the string given.
  Max name length is 12 characters
  @param String name
  @return
*/
void changeName(String newName) {
  String successFlags[] = {"OK", "Set", newName};
  String response = "";

  BTSerial.print("AT+NAME" + newName);
  response = atResponse();

  int numFlags = sizeof(successFlags) / sizeof(successFlags[0]);
  if (includeErrorMessage) {
    if (isATSucessfull(response, successFlags, numFlags)) {
      Serial.println("BLE name changed to " + newName);
    } else {
      Serial.println("Failed to change name");
    }
  }
}

/*
  @desc Changes the role of the Bluetooth module
  @param int role. 0=slave, 1=master
  @return
*/
void changeRole(int role) {
  String successFlags[] = {"OK", "Set", String(role)};
  String response = "";
  String r = "Failed to change role";

  if (role == 0)      {
    r = "slave" ;
  }
  else if (role == 1) {
    r = "master";
  }

  BTSerial.print("AT+ROLE" + String(role));
  response = atResponse();
  int numFlags = sizeof(successFlags) / sizeof(successFlags[0]);
  if (includeErrorMessage) {
    if (isATSucessfull(response, successFlags, numFlags)) {
      Serial.println("BLE role changed to " + r);
    } else {
      Serial.println(r);
    }
  }
}

/*
  @desc Check that the given response string contains all the given success flags
  @param String response - reponse given back by AT commands
  @param String successFlags[] - array of success flags
  @return boolean - true if all flags are found in the response
  @return boolean - false if not all flags are found in the response
*/
boolean isATSucessfull(String response, String successFlags[], int numFlags) {

}

/*
  @desc Listen on the Serial port for a response, reads it and returns it as a single string.
  @param
  @return String - response
*/
String atResponse() {
  if (!canDoAT()) {
    return "ERROR";
  }

  String response = "";
  unsigned long timeout = 2000;
  unsigned long timeStart = millis();

  // Check if there is a response within timeout period
  while (!BTSerial.available()) {
    if ((millis() - timeStart) > timeout) {
      if (includeErrorMessage) {
        Serial.println("AT Response Timeout");
      }
      return "TIMEOUT";
    }
  }

  // Allow full response to load into buffer
  delay(150);

  // Read response
  while (BTSerial.available()) {
    char c = BTSerial.read();
    response.concat(c);
  }
  if (includeErrorMessage) {
    Serial.println("\n" + response);
  }
  return response;
}

/*
  @desc Checks whether the conditions are met to execute AT commands
  @param
  @return boolean - false if not able to execute AT commands
  @return boolean - true if able to execute AT commands
*/
boolean canDoAT() {
  if (!getConnectionStatus()) {
    return true;
  } else {
    if (includeErrorMessage) {
      Serial.println("Error.\nBlueTooth is currently paired, unable to perform AT commands");
    }
    return false;
  }
}


/************************************************************************************************************************/
/************************/
/*      Transmit        */
/************************/
/************************************************************************************************************************/

/*
  @desc Handles the transmission process for an array of Strings
  @param String data[] - array of message to be sent
  @param int arraySize
  @return boolean - true if message is sent and received by other paired device
  @return boolean - false if message is unable to be sent or not confirmed to be received by other paired device
*/
boolean sendData(String data[], int arraySize) {
  // TODO /*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/

  // clone array so we can edit the data in memory
  String copyData[arraySize];
  memcpy(copyData, data, sizeof(data[0])*arraySize);

  // Add markers
  addMarker(&copyData[0], arraySize);
  if (testingMessages) {
    Serial.println("\nAfter adding markers");
    for (int i = 0; i < arraySize; i++) {
      Serial.println(copyData[i]);
    }
  }

  // Convert to single String
  String packet = transformToString(copyData, arraySize);
  if (testingMessages) {
    Serial.println("\nAfter transforming into a single string:");
    Serial.println(packet);
  }

  // Encrypt data
  packet = encrypt(packet);
  if (testingMessages) {
    Serial.println("\nAfter encrypting:");
    Serial.println(packet);
  }

  // Prepend checksum
  packet = addCheckSum(packet);
  if (testingMessages) {
    Serial.println("\nAfter Adding checksum:");
    Serial.println(packet);
  }


  packet = packetStartMarker + packet + packetEndMarker;

  // Write to BTSerial
  int transmitAttempts = 5;
  for (int i = 0; i < transmitAttempts; i++) {
    transmitData(packet);
    if (receivedAcknowlegement()) {
      return true;
    }
  }
  return false;
}

/*
  @desc Listens on the Bluetooth Serial for acknowledgement of receiving data
  @param
  @return boolean - true if acknowledgement received
  @return boolean - false if acknowledgement not received
*/
boolean receivedAcknowlegement() {
  // TODO /*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/
  return true;
}

/*
  @desc Transmit data using te Bluetooth module. Handles breaking down the data into appropriate packet lengths
  @param String packet - data to be sent
  @return boolean - true if acknowledgement received
  @return boolean - false if acknowledgement not received
*/
void transmitData(String data) {

}

/*
  @desc Generates string containing all the data within given array
  @param String dataArray[]
  @param int arraySize
  @return String
*/
String transformToString(String dataArray[], int arraySize) {

}

/*
  @desc Encrypts a given string.
  @param String data
  @return String - the data in encrypted form
*/
String encrypt(String data) {

}

/*
  @desc Add markers to delimit end/beginning of file and lines
  @param String *data - pointer to data in memory
  @param int arraySize - number of elements in the array
  @return
*/
void addMarker(String * dataArray, int arraySize) {

}

/*
  @desc Add checksum to the data to be used for error detection by recipient
  @param String data
  @return String - data prepended with checksum
*/
String addCheckSum(String data) {

}


/************************************************************************************************************************/
/************************/
/*     Receive          */
/************************/
/************************************************************************************************************************/

/*
  @desc Checks if there is incoming transmission on the Bluetooth Serial.
  If there is, handles the process to read and store it for access.
  @param
  @return boolean - true if there is incomming tranmission
  @return boolean - false if there is no incomming tranmission
*/
boolean receivedNewData() {

}

/*
  @desc
  @param
  @return
*/
void rebuildData(String dataFromBT) {


}

/*
  @desc
  @param
  @return
*/
String readFromBTBuffer() {

}

/*
  @desc Undoes the encryption on given String
  @param String data - encrypted datas
  @return String - unencrypted data
*/
String decrypt(String data) {

}

/*
  @desc Reads the checksum of given data
  @param byte data
  @return boolean - TRUE if data and checksum matches
  @return boolean - FALSE if data and checksum does not match
*/
boolean confirmCheckSum(String data) {

}


/*
  @desc
  @param
  @return
*/
String removeCheckSum(String data) {

}

/*
  @desc Removes the added markers for transmission from given data
  @param String data - data that contains markers
  @return String - data without markers
*/
void removeMarkers() {

}

/************************************************************************************************************************/
/************************/
/*      Test            */
/*    TO BE DELETED     */
/************************/
/************************************************************************************************************************/


/*
  @desc Reads input from Serial Monitor and transmit it through BlueTooth
  @param
  @return
*/
void readFromSerialToBT() {

}

/*
  @desc Reads data receieved from BTLE and prints it to Serial Monitor
  @param
  @return
*/
void readFromBlueTooth() {

}








