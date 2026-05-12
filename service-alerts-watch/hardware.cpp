

#include "hardware.h"
#include <LilyGoLib.h> // for instance
#include <math.h>
#include "XPowersLib.h"

extern LilyGoWatch2022 instance;

// Helper: Set AXP2101 charge current to 350mA
void set_charge_current_350mA() {
    // Use instance.pmu for AXP2101
    instance.pmu.setChargerConstantCurr(350); // 350mA
}

// Helper: Get battery percentage
int get_battery_percent() {
    // No isConnected() in XPowersAXP2101, just call getBatteryPercent
    return instance.pmu.getBatteryPercent();
}

// Call this periodically (e.g., in loop) to beep if battery <=10%
void check_and_beep_low_battery() {
    int percent = get_battery_percent();
    if(instance.pmu.isCharging()) {
        return; // Do not beep if charging
    }
    else{
        if(percent >= 0 && percent <= 10) {
            beep(400, 0.5, 0.5); // 400Hz, 200ms, full volume
        }
    }
}

const int SAMPLE_RATE = 44100;

void beep(float freq, float duration_sec, float vol)
{
    setCpuFrequencyMhz(240); 
    const int bufLen = 256;                 // small streaming buffer
    int16_t buf[bufLen];
    const int total_samples = (int)(duration_sec * SAMPLE_RATE);

    // make sure speaker power is on
    instance.powerControl(POWER_SPEAK, true);

    for (int i = 0; i < total_samples; i += bufLen) {
        const int n = (total_samples - i) < bufLen ? (total_samples - i) : bufLen;
        for (int j = 0; j < n; ++j) {
            float s = sinf(2.0f * PI * freq * (i + j) / SAMPLE_RATE);
            int32_t v = (int32_t)(s * 32767.0f * vol);
            if (v > 32767) v = 32767;      // clip just in case
            if (v < -32768) v = -32768;
            buf[j] = (int16_t)v;
        }
        instance.player.write((const uint8_t*)buf, n * sizeof(int16_t));
    }
    setCpuFrequencyMhz(80); // Set lower base frequency (can be 80 or 160)
}

// Haptic motor control (T-Watch S3, use LilyGoLib API)
void vibrate_haptic(uint16_t ms) {
    instance.setHapticEffects(1); // 1 = strong click
    uint32_t start = millis();
    while (millis() - start < ms) { 
        beep(1000, 0.3,0.8); // Play beep (max 300ms for comfort)
        unsigned long start1 = millis();
        while (millis() - start1 < 250){
            instance.vibrator();
            delay(5);
        }
        beep(1000, 0.3,0.8); // Play beep (max 300ms for comfort)
        unsigned long start2 = millis();
        while (millis() - start2 < 250){
            instance.vibrator();
            delay(5);
        }
    }
}

