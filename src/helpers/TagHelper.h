#pragma once

#include "IdentityStore.h"

struct TagValue {
  char *tag = nullptr;
  char *value = nullptr;
  bool dirty = false;
  TagValue *next = nullptr;
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
};

extern TagHelperClass tagHelper;