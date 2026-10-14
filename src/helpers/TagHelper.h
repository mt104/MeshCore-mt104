#pragma once

#include "IdentityStore.h"

struct TagValue {
  char *tag = nullptr;
  char *value = nullptr;
  bool dirty = false;
  TagValue *next = nullptr;
};

// Signature for callback function to be called when a tag value changes
typedef void (*TagChangedCallback)(const char *tag, const char *value);
// Linked list of registered callbacks
struct TagChangedCallbackNode {
  TagChangedCallback callback;
  TagChangedCallbackNode *next = nullptr;
  char *tagFilter = nullptr; // If not null, only call this callback for the specified tag
};

class TagHelperClass {

public:
  TagHelperClass() { }
  static void setFilesystem(FILESYSTEM *fs);
  bool save();
  bool load();
  TagValue *get(char *tag);
  char *getValue(char *tag, char *defaultValue = nullptr);
  void setValue(char *tag, char *value);
  void forget(char *tag);
  void listToSerial();
  void registerCallback(TagChangedCallback callback, const char *tagFilter);
  void notifyTagChanged(const char *tag, const char *value);
};

extern TagHelperClass tagHelper;