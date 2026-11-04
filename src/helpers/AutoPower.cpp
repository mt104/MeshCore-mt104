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

void AutoPower::setup() {
    mvLatest = board.getBattMilliVolts();
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = mvLatest;
    }
    nextHourlyUpdateDue = millis() + 3600000; // set next hourly update due in 1 hour
    mvLargestDropRecently = tagHelper.getIntValue((char *)tagHighestMvDropPerHour, 0);
}

void AutoPower::loop() {
    unsigned long now = millis();
    if (now - lastUpdate >= 10000) {
        mvLatest = board.getBattMilliVolts();
        samples[sampleIndex] = mvLatest;
        sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;
        // Update smoothed value
        uint32_t sum = 0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            sum += samples[i];
        }
        mvSmoothed = sum / NUM_SAMPLES;
        lastUpdate = now;
        if (mvSmoothed < lowestVoltageSoFarThisHour || lowestVoltageSoFarThisHour == 0) {
            lowestVoltageSoFarThisHour = mvSmoothed;
        }
        if (mvSmoothed > highestVoltageSoFarThisHour) {
            highestVoltageSoFarThisHour = mvSmoothed;
        }
        if (now >= nextHourlyUpdateDue) {
            nextHourlyUpdateDue = now + 3600000; // set next hourly update due in 1 hour
            lowestVoltagePerHour[hourIndex] = lowestVoltageSoFarThisHour;
            highestVoltagePerHour[hourIndex] = highestVoltageSoFarThisHour;
            mvDropPerHour[hourIndex] = highestVoltageSoFarThisHour - lowestVoltageSoFarThisHour;
            hourIndex = (hourIndex + 1) % NUM_HOURS;
            lowestVoltageSoFarThisHour = mvSmoothed;
            highestVoltageSoFarThisHour = mvSmoothed;
            mvLargestDropRecently = 0;
            for (int i = 0; i < NUM_HOURS; i++) {
                if (mvDropPerHour[i] > mvLargestDropRecently) {
                    mvLargestDropRecently = mvDropPerHour[i];
                }
            }
            tagHelper.setIntValue((char *)tagHighestMvDropPerHour, mvLargestDropRecently);
        }
    }
}

uint16_t AutoPower::getBattMilliVoltsSmoothed() {
    return mvSmoothed;
}

uint16_t AutoPower::getLargestDropRecently() {
    return mvLargestDropRecently;
}