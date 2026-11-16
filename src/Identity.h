#pragma once

#include <Utils.h>
#include <Stream.h>

#ifdef ESP32
#include "helpers/Syslog.h"
#include <cstdio>
#endif

namespace mesh {

/**
 * \brief  An identity in the mesh, with given Ed25519 public key, ie. a party whose signatures can be VERIFIED.
*/
class Identity {
public:
  uint8_t pub_key[PUB_KEY_SIZE];

  Identity();
  Identity(const char* pub_hex);
  Identity(const uint8_t* _pub) { memcpy(pub_key, _pub, PUB_KEY_SIZE); }

  int copyHashTo(uint8_t* dest) const { 
    memcpy(dest, pub_key, PATH_HASH_SIZE);    // hash is just prefix of pub_key
    return PATH_HASH_SIZE;
  }
  int copyHashTo(uint8_t* dest, uint8_t len) const { 
    memcpy(dest, pub_key, len);    // hash is just prefix of pub_key
    return len;
  }
  bool isHashMatch(const uint8_t* hash) const {
    return memcmp(hash, pub_key, PATH_HASH_SIZE) == 0;
  }
  bool isHashMatch(const uint8_t* hash, uint8_t len) const {
    return memcmp(hash, pub_key, len) == 0;
  }

  bool isHashMatchAnywhereInPath(const uint8_t *hash, const uint8_t *path, uint8_t path_len, int max_depth = 0) const {
    int hash_size = (path_len >> 6) + 1;
    int hash_count = path_len & 63;
    for (int i = 0; i < hash_count; i++) {
      if (max_depth > 0 && i >= max_depth) break;
      if (memcmp(hash, &path[i*hash_size], hash_size) == 0) {
#ifdef ESP32
        char buffer[200];
        sprintf(buffer, "Matched %02X %02X %02X hash_size %d at index %d (path_len=%d)", hash[0], hash[1], hash[2], hash_size, i, path_len);
        // Append bytes from the path as a series of hex values for context
        strcat(buffer, " Path:");
        for (int j = 0; j < hash_count; j++) {
          char hex[3];
          sprintf(hex, " %02X", path[j]);
          strcat(buffer, hex);
        }
        syslogDebug("isHashMatchAnywhereInPath", buffer);
#endif
        return true;
      }
    }
    return false;
  }

      /**
       * \brief  Performs Ed25519 signature verification.
       * \param sig IN - must be SIGNATURE_SIZE buffer.
       * \param message IN - the original message which was signed.
       * \param msg_len IN - the length in bytes of message.
       * \returns true, if signature is valid.
       */
      bool verify(const uint8_t *sig, const uint8_t *message, int msg_len) const;

  bool matches(const Identity& other) const { return memcmp(pub_key, other.pub_key, PUB_KEY_SIZE) == 0; }
  bool matches(const uint8_t* other_pubkey) const { return memcmp(pub_key, other_pubkey, PUB_KEY_SIZE) == 0; }

  bool readFrom(Stream& s);
  bool writeTo(Stream& s) const;
  void printTo(Stream& s) const;
};

/**
 * \brief  An Identity generated on THIS device, ie. with public/private Ed25519 key pair being on this device.
*/
class LocalIdentity : public Identity {
  uint8_t prv_key[PRV_KEY_SIZE];
public:
  LocalIdentity();
  LocalIdentity(const char* prv_hex, const char* pub_hex);
  LocalIdentity(RNG* rng);   // create new random

  /**
   * \brief  Ed25519 digital signature.
   * \param sig OUT - must be SIGNATURE_SIZE buffer.
   * \param message IN - the raw message bytes to sign.
   * \param msg_len IN - the length in bytes of message.
  */
  void sign(uint8_t* sig, const uint8_t* message, int msg_len) const;

  /**
   * \brief  the ECDH key exhange, with Ed25519 public key transposed to Ex25519.
   * \param  secret OUT - the 'shared secret' (must be PUB_KEY_SIZE bytes)
   * \param  other IN - the second party in the exchange.
  */
  void calcSharedSecret(uint8_t* secret, const Identity& other) const { calcSharedSecret(secret, other.pub_key); }

  /**
   * \brief  the ECDH key exhange, with Ed25519 public key transposed to Ex25519.
   * \param  secret OUT - the 'shared secret' (must be PUB_KEY_SIZE bytes)
   * \param  other_pub_key IN - the public key of second party in the exchange (must be PUB_KEY_SIZE bytes)
  */
  void calcSharedSecret(uint8_t* secret, const uint8_t* other_pub_key) const;

  /**
   * \brief  Validates that a given private key can be used for ECDH / shared-secret operations.
   * \param  prv IN - the private key to validate (must be PRV_KEY_SIZE bytes)
   * \returns true, if the private key is valid for login.
  */
  static bool validatePrivateKey(const uint8_t prv[64]);

  bool readFrom(Stream& s);
  bool writeTo(Stream& s) const;
  void printTo(Stream& s) const;
  size_t writeTo(uint8_t* dest, size_t max_len);
  void readFrom(const uint8_t* src, size_t len);
};

}

