#include "AutoPower.h"
#include <target.h>

unsigned long lastUpdate = 0;
uint16_t mvLatest = 0;
uint16_t mvSmoothed = 0;

// Array to hold last N samples
const int NUM_SAMPLES = 12;
uint16_t samples[NUM_SAMPLES] = {0};
int sampleIndex = 0;

void AutoPower::setup() {
    mvLatest = board.getBattMilliVolts();
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = mvLatest;
    }
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
    }
}

uint16_t AutoPower::getBattMilliVoltsSmoothed() {
    return mvSmoothed;
}