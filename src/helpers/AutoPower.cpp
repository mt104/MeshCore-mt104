#include "AutoPower.h"
#include "TagHelper.h"
#include <target.h>

// For smoothing battery voltage readings
unsigned long lastUpdate = 0;
uint16_t mvLatest = 0;
uint16_t mvSmoothed = 0;
const int NUM_SAMPLES = 12;
uint16_t samples[NUM_SAMPLES] = {0};
int sampleIndex = 0;

// For tracking Voltage drop per hour ovr the last N hours
unsigned long nextHourlyUpdateDue = 0;
uint16_t lowestVoltageSoFarThisHour = 0;
uint16_t highestVoltageSoFarThisHour = 0;
int hourIndex = 0;
const int NUM_HOURS = 48;
uint16_t lowestVoltagePerHour[NUM_HOURS] = {0};
uint16_t highestVoltagePerHour[NUM_HOURS] = {0};
uint16_t mvDropPerHour[NUM_HOURS] = {0};
uint16_t mvLargestDropRecently = 0;
const char *tagHighestMvDropPerHour = "autopower.highestdropperhourmv";
uint16_t mvAverageDropRecently = 0;
const char *tagAverageMvDropRecently = "autopower.averagedroprecently";

// For tracking lowest safe Voltage
uint16_t lowestSafeVoltage = 0;
const uint16_t DEFAULT_LOWEST_SAFE_VOLTAGE = 3200;
const char *tagLowestSafeVoltage = "autopower.lowestsafevoltage";

// Highest Voltage seen, i.e. "full charge" voltage
uint16_t mvHighestVoltageSeen = 0;
const uint16_t DEFAULT_HIGHEST_VOLTAGE_SEEN = 4100;
const char *tagHighestVoltageSeen = "autopower.highestvoltageseen";

void AutoPower::setup() {
    mvLatest = board.getBattMilliVolts();
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = mvLatest;
    }
    nextHourlyUpdateDue = millis() + 3600000; // set next hourly update due in 1 hour
    mvLargestDropRecently = tagHelper.getIntValue((char *)tagHighestMvDropPerHour, 0);
    mvAverageDropRecently = tagHelper.getIntValue((char *)tagAverageMvDropRecently, 0);
    for (int i = 0; i < NUM_HOURS; i++) {
        mvDropPerHour[i] = mvAverageDropRecently;
    }
    lowestSafeVoltage = tagHelper.getIntValue((char *)tagLowestSafeVoltage, DEFAULT_LOWEST_SAFE_VOLTAGE);
    mvHighestVoltageSeen = tagHelper.getIntValue((char *)tagHighestVoltageSeen, DEFAULT_HIGHEST_VOLTAGE_SEEN);
}

void AutoPower::recalibrate() {
    mvLatest = board.getBattMilliVolts();
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = mvLatest;
    }
    mvSmoothed = mvLatest;
    lowestVoltageSoFarThisHour = mvSmoothed;
    highestVoltageSoFarThisHour = mvSmoothed;
    for (int i = 0; i < NUM_HOURS; i++) {
        lowestVoltagePerHour[i] = 0;
        highestVoltagePerHour[i] = 0;
        mvDropPerHour[i] = 0;
    }
    lowestSafeVoltage = DEFAULT_LOWEST_SAFE_VOLTAGE;
    tagHelper.setIntValue((char *)tagLowestSafeVoltage, lowestSafeVoltage);
    mvLargestDropRecently = 0;
    tagHelper.setIntValue((char *)tagHighestMvDropPerHour, mvLargestDropRecently);
    mvAverageDropRecently = 0;
    tagHelper.setIntValue((char *)tagAverageMvDropRecently, mvAverageDropRecently);
    mvHighestVoltageSeen = DEFAULT_HIGHEST_VOLTAGE_SEEN;
    tagHelper.setIntValue((char *)tagHighestVoltageSeen, mvHighestVoltageSeen);
}

void AutoPower::loop() {
  unsigned long now = millis();

  // Use safe millis rollover subtraction
  if (now - lastUpdate >= 10000) {
    mvLatest = board.getBattMilliVolts();

    // Safety bounds check for sampleIndex
    if (sampleIndex < NUM_SAMPLES) {
      samples[sampleIndex] = mvLatest;
    }
    sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

    // Update smoothed value
    uint32_t sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      sum += samples[i];
    }
    mvSmoothed = sum / NUM_SAMPLES;
    lastUpdate = now;

    if (lowestVoltageSoFarThisHour == 0 || mvSmoothed < lowestVoltageSoFarThisHour) {
      lowestVoltageSoFarThisHour = mvSmoothed;
    }
    if (mvSmoothed > highestVoltageSoFarThisHour) {
      highestVoltageSoFarThisHour = mvSmoothed;
    }
    if (mvSmoothed > mvHighestVoltageSeen) {
      mvHighestVoltageSeen = mvSmoothed;
      tagHelper.setIntValue((char *)tagHighestVoltageSeen, mvHighestVoltageSeen);
    }

    // Hourly update check using safe subtraction
    if ((long)(now - nextHourlyUpdateDue) >= 0) {
      nextHourlyUpdateDue = now + 3600000;

      if (hourIndex < NUM_HOURS) {
        lowestVoltagePerHour[hourIndex] = lowestVoltageSoFarThisHour;
        highestVoltagePerHour[hourIndex] = highestVoltageSoFarThisHour;

        // Prevent underflow if high/low got inverted
        if (highestVoltageSoFarThisHour >= lowestVoltageSoFarThisHour) {
          mvDropPerHour[hourIndex] = highestVoltageSoFarThisHour - lowestVoltageSoFarThisHour;
        } else {
          mvDropPerHour[hourIndex] = 0;
        }

        // Calculate the average drop over recent hours
        uint32_t sumDrops = 0;
        for (int i = 0; i < NUM_HOURS; i++) {
          sumDrops += mvDropPerHour[i];
        }
        mvAverageDropRecently = sumDrops / NUM_HOURS;
        tagHelper.setIntValue((char *)tagAverageMvDropRecently, mvAverageDropRecently);

        hourIndex = (hourIndex + 1) % NUM_HOURS;
      }

      lowestVoltageSoFarThisHour = mvSmoothed;
      highestVoltageSoFarThisHour = mvSmoothed;

      uint16_t previousMaxDrop = mvLargestDropRecently;
      mvLargestDropRecently = 0;

      for (int i = 0; i < NUM_HOURS; i++) {
        if (mvDropPerHour[i] > mvLargestDropRecently) {
          mvLargestDropRecently = mvDropPerHour[i];
        }
      }

      // Only write to storage if the value actually changed to prevent WDT / flash wear
      if (mvLargestDropRecently != previousMaxDrop) {
        tagHelper.setIntValue((char *)tagHighestMvDropPerHour, mvLargestDropRecently);
      }

      uint16_t newLowestSafeVoltage = lowestVoltageSoFarThisHour + (mvLargestDropRecently / 2);
      if (newLowestSafeVoltage < lowestSafeVoltage) {
        lowestSafeVoltage = newLowestSafeVoltage;
        tagHelper.setIntValue((char *)tagLowestSafeVoltage, lowestSafeVoltage);
      }
    }
  }
}

uint16_t AutoPower::getBattMilliVoltsSmoothed() {
    if (mvSmoothed == 0) {
        return board.getBattMilliVolts();
    }
    return mvSmoothed;
}

const uint16_t MIN_BATT_MV = 3000;
const uint16_t MAX_BATT_MV = 4200;
uint8_t AutoPower::getBattPercentSmoothed() {
    uint16_t mvSmoothed = getBattMilliVoltsSmoothed();
    uint16_t mvRange = MAX_BATT_MV - MIN_BATT_MV;
    uint16_t mvAboveSafe = mvSmoothed - MIN_BATT_MV;
    uint8_t battPercent = (uint8_t)(mvAboveSafe * 100 / mvRange);
    if (battPercent > 100) battPercent = 100;
    if (battPercent < 0) battPercent = 0;
    return battPercent;
}

uint8_t AutoPower::getBattPercentCalibrated() {
  // Handle overshoot above and below known Voltage range
  if (getBattMilliVoltsSmoothed() <= lowestSafeVoltage) {
      return 0;
  }
  if (getBattMilliVoltsSmoothed() >= mvHighestVoltageSeen) {
      return 100;
  }
  uint16_t mvCurrent = getBattMilliVoltsSmoothed();
  uint16_t mvHighestMinusLowest = mvHighestVoltageSeen - lowestSafeVoltage;
  uint16_t mvCurrentMinusLowest = mvCurrent - lowestSafeVoltage;
  uint8_t battPercent = (uint8_t)(mvCurrentMinusLowest * 100 / mvHighestMinusLowest);

  /*
  Serial.println();
  Serial.print("Batt mV Smoothed: ");
  Serial.println(getBattMilliVoltsSmoothed());
  Serial.print("Lowest Safe Voltage: ");
  Serial.println(lowestSafeVoltage);
  Serial.print("Highest Voltage Seen: ");
  Serial.println(mvHighestVoltageSeen);
  Serial.print("Voltage Above Safe: ");
  Serial.println(getBattMilliVoltsSmoothed() - lowestSafeVoltage);
  Serial.print("mvHighestMinusLowest: ");
  Serial.println(mvHighestMinusLowest);
  Serial.print("mvCurrentMinusLowest: ");
  Serial.println(mvCurrentMinusLowest);
  Serial.print("Batt Percent: ");
  Serial.println(battPercent);
  */

  // Make sure battPercent is within 0-100 range
  if (battPercent > 100) battPercent = 100;
  if (battPercent < 0) battPercent = 0;
  return battPercent;
}

uint16_t AutoPower::getLargestDropRecently() {
    if ((mvLargestDropRecently > 0)) {
        return mvLargestDropRecently;
    }
    return highestVoltageSoFarThisHour - lowestVoltageSoFarThisHour;
}

uint16_t AutoPower::getAverageDropRecently() {
    return mvAverageDropRecently;
}

uint16_t AutoPower::getLowestSafeVoltage() {
    return lowestSafeVoltage;
}

uint16_t AutoPower::getHighestVoltageSeen() {
    return mvHighestVoltageSeen;
}

float AutoPower::hoursRemaining() {
    // Estimate hours remaining based on the largest drop recently and the lowest safe voltage
    uint16_t mvAverageDropRecently = getAverageDropRecently(); 
    if (mvAverageDropRecently == 0) {
        return -1.0; // Unknown
    }
    uint16_t voltageAboveSafe = getBattMilliVoltsSmoothed() - lowestSafeVoltage;
    float hours = (float)voltageAboveSafe / (float)mvAverageDropRecently;
    return hours;
}