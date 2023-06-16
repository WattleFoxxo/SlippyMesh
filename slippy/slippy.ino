// //--------------//
// //  slippy.ino  //
// //--------------//

// #include <SPI.h>
// #include <LoRa.h>

// #define MAX_PACKET_SIZE 64

// String outgoing;

// void setup() {
//   Serial.begin(115200);
//   Serial.setTimeout(100);
//   while (!Serial);

//   if (!LoRa.begin(915E6)) {
//     while (true);
//   }

//   LoRa.setSpreadingFactor(12);
//   LoRa.setSignalBandwidth(500E3);
//   LoRa.setCodingRate4(8);
//   LoRa.setTxPower(20);
//   LoRa.enableCrc();

//   LoRa.onReceive(onReceive);
//   LoRa.receive();
// }

// void loop() {
//   while (!Serial.available());

//   String outgoing = Serial.readStringUntil("\r\n\r\n\r\n\r\n\r\n\r\n");

//   byte localAddr = getValue(outgoing, ';', 0).toInt();
//   byte targetAddr = getValue(outgoing, ';', 1).toInt();
//   byte protocol = getValue(outgoing, ';', 2).toInt();
  
//   outgoing.remove(0,  getValue(outgoing, ';', 0).length()+
//                       getValue(outgoing, ';', 1).length()+
//                       getValue(outgoing, ';', 2).length()+
//                       3
//   );

//   int length = outgoing.length();
//   for (int i = 0; i < length; i += MAX_PACKET_SIZE) {
//     String chunk = outgoing.substring(i, min(i + MAX_PACKET_SIZE, length));
//     LoRa.beginPacket();
//     LoRa.write(localAddr);
//     LoRa.write(targetAddr);
//     LoRa.write(protocol);
//     LoRa.write(chunk.length());
//     LoRa.print(chunk);
//     LoRa.endPacket();
//     LoRa.receive();
//     Serial.print(localAddr);
//     Serial.print(";");
//     Serial.print(targetAddr);
//     Serial.print(";");
//     Serial.print(protocol);
//     Serial.print(";");
//     Serial.print(chunk);
//     Serial.print("\r\n\r\n");
//   }
// }

// void onReceive(int packetSize) {
//   if (packetSize == 0) return;
//   byte sendAddr = LoRa.read();
//   byte targAddr = LoRa.read();
//   byte proto = LoRa.read();
//   byte incomingLength = LoRa.read();
//   String incoming = "";

//   while (LoRa.available()) {
//     incoming += (char)LoRa.read();
//   }

//   if (incomingLength != incoming.length()) {
//     return;
//   }

//   Serial.print(incoming);
// }

// String getValue(String data, char separator, int index) {
//   int found = 0;
//   int strIndex[] = {0, -1};
//   int maxIndex = data.length()-1;

//   for(int i=0; i<=maxIndex && found<=index; i++) {
//     if(data.charAt(i)==separator || i==maxIndex) {
//         found++;
//         strIndex[0] = strIndex[1]+1;
//         strIndex[1] = (i == maxIndex) ? i+1 : i;
//     }
//   }

//   return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
// }

//--------------//
//  slippy.ino  //
//--------------//

#include <SPI.h>
#include <LoRa.h>

#define MAX_PACKET_SIZE 64

String outgoing;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  while (!Serial);

  if (!LoRa.begin(915E6)) {
    while (true);
  }

  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(500E3);
  LoRa.setCodingRate4(8);
  LoRa.setTxPower(20);
  LoRa.enableCrc();

  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void loop() {
  while (!Serial.available());

  String outgoing = Serial.readStringUntil("\r\n\r\n\r\n\r\n\r\n\r\n");

  byte localAddr = getValue(outgoing, ';', 0).toInt();
  byte targetAddr = getValue(outgoing, ';', 1).toInt();
  byte protocol = getValue(outgoing, ';', 2).toInt();
  
  outgoing.remove(0,  getValue(outgoing, ';', 0).length()+
                      getValue(outgoing, ';', 1).length()+
                      getValue(outgoing, ';', 2).length()+
                      3
  );

  int length = outgoing.length();
  for (int i = 0; i < length; i += MAX_PACKET_SIZE) {
    String chunk = outgoing.substring(i, min(i + MAX_PACKET_SIZE, length));
    LoRa.beginPacket();
    LoRa.write(localAddr);
    LoRa.write(targetAddr);
    LoRa.write(protocol);
    LoRa.write(chunk.length());
    LoRa.print(chunk);
    LoRa.endPacket();
    LoRa.receive();
    Serial.print(localAddr);
    Serial.print(";");
    Serial.print(targetAddr);
    Serial.print(";");
    Serial.print(protocol);
    Serial.print(";");
    Serial.print(chunk);
    Serial.print("\r\n\r\n");
  }
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  byte sendAddr = LoRa.read();
  byte targAddr = LoRa.read();
  byte proto = LoRa.read();
  byte incomingLength = LoRa.read();
  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {
    return;
  }

  Serial.print(sendAddr);
  Serial.print(";");
  Serial.print(targAddr);
  Serial.print(";");
  Serial.print(proto);
  Serial.print(";");
  Serial.print(incoming);
  Serial.print("\r\n\r\n");
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++) {
    if(data.charAt(i)==separator || i==maxIndex) {
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}