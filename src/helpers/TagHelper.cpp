#include "helpers/TagHelper.h"
#include "IdentityStore.h"
#include <Arduino.h>

static FILESYSTEM *_fs = nullptr;
const char *_filenameTagValue = "/taghelper.config";
TagValue* _firstTagValue = nullptr;

// Linked list of registered callbacks for tag value changes
static TagChangedCallbackNode* _firstCallbackNode = nullptr;

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
        // Remove trailing newline or carriage return characters from value
        value.trim();
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
    // Notify anyone that cares that the tag value has changed
    notifyTagChanged(tag, value);
  }
}

void TagHelperClass::forget(char *tag) {
  TagValue *current = _firstTagValue;
  TagValue *prev = nullptr;
  while (current) {
    if (strcmp(current->tag, tag) == 0) {
      // Found the tag to forget
      if (prev) {
        prev->next = current->next;
      } else {
        _firstTagValue = current->next;
      }
      free(current->tag);
      free(current->value);
      delete current;
      save(); // Save to filesystem after forgetting the tag
      return;
    }
    prev = current;
    current = current->next;
  }
}

void TagHelperClass::listToSerial() {
  TagValue *current = _firstTagValue;
  while (current) {
    Serial.print(current->tag);
    Serial.print(" = \"");
    Serial.print(current->value);
    Serial.println("\"");
    current = current->next;
  }
}

void TagHelperClass::registerCallback(TagChangedCallback callback, const char *tagFilter) {
  TagChangedCallbackNode *newNode = new TagChangedCallbackNode();
  newNode->callback = callback;
  if (tagFilter) {
    newNode->tagFilter = strdup(tagFilter);
  }
  newNode->next = _firstCallbackNode;
  _firstCallbackNode = newNode;
}

// When a tag value changes, call all registered callbacks that match the tag
void TagHelperClass::notifyTagChanged(const char *tag, const char *value) {
  TagChangedCallbackNode *current = _firstCallbackNode;
  while (current) {
    if (!current->tagFilter || strcmp(current->tagFilter, tag) == 0) {
      current->callback(tag, value);
    }
    current = current->next;
  }
}