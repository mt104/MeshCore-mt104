#include "ControlHelper.h"
#include "MeshCore.h"
#include <AES.h>
#include <Arduino.h>
#include "Packet.h"
#include "../../examples/simple_repeater/MyMesh.h"

MyMesh *my_mesh = nullptr;

void ControlHelperClass::Init(MyMesh &mesh)
{
    my_mesh = &mesh;
}

void ControlHelperClass::SendStatus()
{
    sendMessage("Status update");
}

// Define your 16-byte hard-coded pre-shared key for the private channel
const uint8_t HARDCODED_PRIVATE_PSK[16] = { 
    0xb8, 0xa7, 0x10, 0xe3, 0x1f, 0x82, 0x07, 0xbe,
    0x33, 0xcf, 0x9f, 0x00, 0xf5, 0xab, 0x80, 0x11
};

// Calculate 2-byte channel hash from key (or channel secret)
uint16_t getChannelHash(const uint8_t *key) {
  uint16_t h = 0xFFFF;
  for (int i = 0; i < 16; i++) {
    h ^= key[i];
    for (int j = 0; j < 8; j++) {
      if (h & 1)
        h = (h >> 1) ^ 0xA001;
      else
        h >>= 1;
    }
  }
  return h;
}

bool ControlHelperClass::sendMessage(const char *messageText) {
  if (!my_mesh) {
    Serial.println("Error: Mesh not initialized.");
    return false;
  }

  mesh::GroupChannel groupChannel;
  uint16_t channelHash = getChannelHash(HARDCODED_PRIVATE_PSK);
  groupChannel.hash = channelHash;
  groupChannel.secret = HARDCODED_PRIVATE_PSK;



  auto pkt2 = my_mesh->createGroupDatagram

mesh::Packet pkt;

  // 1. Set payload type to GRP_DATA (Flood route bit set automatically by sendFlood)
  pkt.header = (PAYLOAD_TYPE_GRP_DATA << 2);

  // 2. Format Cleartext Payload: [Timestamp/ID (4 bytes)] + [Sender ID (4 bytes)] + [Text String]
  uint8_t cleartext[180];
  uint32_t now = millis() / 1000;
  uint32_t senderId = my_mesh->getNodeId(); // Get repeater's local node ID

  memcpy(&cleartext[0], &now, 4);
  memcpy(&cleartext[4], &senderId, 4);

  int textLen = strlen(messageText);
  if (textLen > 140) textLen = 140;
  memcpy(&cleartext[8], messageText, textLen);

  int rawLen = 8 + textLen;
  int paddedLen = ((rawLen + 15) / 16) * 16;
  uint8_t padValue = paddedLen - rawLen;
  for (int i = rawLen; i < paddedLen; i++) {
    cleartext[i] = padValue;
  }

  // 3. Prepare Encrypted Wire Payload Layout:
  // [Channel Hash (2 bytes)] + [IV (16 bytes)] + [AES-CBC Encrypted Body (paddedLen)]
  uint16_t channelHash = getChannelHash(HARDCODED_PRIVATE_PSK);

  uint8_t iv[16];
  for (int i = 0; i < 16; i++) {
    iv[i] = (uint8_t)random(0, 256); // Generate random IV
  }

  uint8_t encryptedBody[192];
  CBC<AES128> cbc;
  cbc.setKey(HARDCODED_PRIVATE_PSK, 16);
  cbc.setIV(iv, 16);
  cbc.encrypt(encryptedBody, cleartext, paddedLen);

  // Assembly
  int totalPayloadLen = 0;
  memcpy(&pkt.payload[0], &channelHash, 2);
  totalPayloadLen += 2;

  memcpy(&pkt.payload[totalPayloadLen], iv, 16);
  totalPayloadLen += 16;

  memcpy(&pkt.payload[totalPayloadLen], encryptedBody, paddedLen);
  totalPayloadLen += paddedLen;

  pkt.payload_len = totalPayloadLen;

  Serial.printf("Transmitting private channel message (%d bytes)... \n", totalPayloadLen);

  // 4. Hand packet over to Mesh routing engine (it handles path construction and headers)
  my_mesh->sendFlood(&pkt);

  return true;
}