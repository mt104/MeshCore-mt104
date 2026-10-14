#include "helpers/TagHelper.h"
#include "IdentityStore.h"

static FILESYSTEM *_fs = nullptr;
const char *_filenameTagValue = "/taghelper.config";
TagValue* _firstTagValue = nullptr;

void TagHelperClass::setFilesystem(FILESYSTEM *fs) {
  _fs = fs;
  tagHelper.load(); // Load existing tag values from the filesystem
}

bool TagHelperClass::save() {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _fs->remove(_filenameTagValue);
  File file = _fs->open(_filenameTagValue, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(_filenameTagValue, "w");
#else
  File file = _fs->open(_filenameTagValue, "w", true);
#endif
  if (file) {
    // Iterate through the tags, writing one per line, in the format "tag=value\n"
    TagValue *current = _firstTagValue;
    while (current) {
      if (current->tag && current->value) {
        file.print(current->tag);
        file.print('=');
        file.println(current->value);
      }
      current = current->next;
    }
    file.close();
    return true;
  }
  return false;
}

bool TagHelperClass::load() {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File file = _fs->open(_filenameTagValue, FILE_O_READ);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(_filenameTagValue, "r");
#else
  File file = _fs->open(_filenameTagValue, "r", true);
#endif
  if (file) {
    // Read the file line by line, parsing each line into a tag and value
    while (file.available()) {
      String line = file.readStringUntil('\n');
      int equalsIndex = line.indexOf('=');
      if (equalsIndex > 0) {
        String tag = line.substring(0, equalsIndex);
        String value = line.substring(equalsIndex + 1);
        setValue(const_cast<char*>(tag.c_str()), const_cast<char*>(value.c_str()));
      }
    }
    file.close();
    return true;
  }
  return false;
}

TagValue* TagHelperClass::get(char *tag) {
  TagValue *current = _firstTagValue;
  while (current) {
    if (strcmp(current->tag, tag) == 0) {
      return current;
    }
    current = current->next;
  }
  return nullptr; // Tag not found
}

char *TagHelperClass::getValue(char *tag, char *defaultValue /* =nullptr */) {
    TagValue *tagValue = get(const_cast<char*>(tag));
    if (tagValue) {
      return tagValue->value;
    }
    return defaultValue; // Tag not found
}

void TagHelperClass::setValue(char *tag, char *value) {
  TagValue *current = nullptr;
  if (!_firstTagValue) {
    _firstTagValue = new TagValue();
    current = _firstTagValue;
  } else {
    // Look for a tag that matches the one being set
    current = get(tag);
    if (!current) {
      // If not found, create a new TagValue at the end of the list
      current = _firstTagValue;
      while (current->next) {
        current = current->next;
      }
      current->next = new TagValue();
      current = current->next;
    }
  }
  if (current->value) {
    free(current->value);
  }
  if (!current->tag) {
    current->tag = strdup(tag);
  }
  if (!current->value || strcmp(current->value, value) != 0) {
    current->value = strdup(value);
    current->dirty = true;
  }
  if (current->dirty) {
    save(); // Save to filesystem if the value has changed
    current->dirty = false;
  }
}

void TagHelperClass::listToSerial() {
  TagValue *current = _firstTagValue;
  while (current) {
    Serial.print("Tag: ");
    Serial.print(current->tag);
    Serial.print(", Value: ");
    Serial.println(current->value);
    current = current->next;
  }
}