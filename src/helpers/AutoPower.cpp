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
const uint16_t DEFAULT_LOWEST_SAFE_VOLTAGE = 3500;
const char *tagLowestSafeVoltage = "autopower.lowestsafevoltage";

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
    return mvSmoothed;
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

float AutoPower::hoursRemaining() {
    // Estimate hours remaining based on the largest drop recently and the lowest safe voltage
    uint16_t mvAverageDropRecently = getAverageDropRecently(); 
    if (mvAverageDropRecently == 0) {
        return -1.0; // Unknown
    }
    uint16_t voltageAboveSafe = mvSmoothed - lowestSafeVoltage;
    float hours = (float)voltageAboveSafe / (float)mvAverageDropRecently;
    return hours;
}