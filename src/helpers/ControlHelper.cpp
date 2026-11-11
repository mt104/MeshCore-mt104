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

bool ControlHelperClass::sendMessage(const char *messageText) {
  if (!my_mesh) {
    Serial.println("Error: Mesh not initialized.");
    return false;
  }

  mesh::Packet pkt;

  // 1. Set up packet header flags (Payload Type Group/Data + Flood Route mode)
  // PH_TYPE_SHIFT is 2, and ROUTE_TYPE_FLOOD is 0x01
  pkt.header = (PAYLOAD_TYPE_GRP_DATA << 2) | ROUTE_TYPE_FLOOD;

  // 2. Set path metadata length (0 for a direct zero-hop broadcast from this node)
  pkt.path_len = 0;

  // 3. Construct the inner payload buffer
  // Layout typically begins with the channel index reference byte followed by text
  uint8_t innerBuffer[200];
  int textLen = strlen(messageText);
  if (textLen > 150) textLen = 150; // Keep within safe LoRa bounds

  innerBuffer[0] = 0x01; // Target private channel index slot
  memcpy(&innerBuffer[1], messageText, textLen);
  int innerLen = 1 + textLen;

  // 4. Encrypt the payload data in place using your AES-128 routine with the hard-coded PSK
  // encrypt_aes_cbc(innerBuffer, innerLen, HARDCODED_PRIVATE_PSK);

  // 5. Assign directly to the packet's public payload buffer and length members
  if (innerLen > sizeof(pkt.payload)) { // Or check against your buffer limit
    Serial.println("Error: Payload too large.");
    return false;
  }

  memcpy(pkt.payload, innerBuffer, innerLen);
  pkt.payload_len = innerLen;

  Serial.printf("Sending message via private channel: %s\n", messageText);

  my_mesh->sendFlood(&pkt);

  return true;
}