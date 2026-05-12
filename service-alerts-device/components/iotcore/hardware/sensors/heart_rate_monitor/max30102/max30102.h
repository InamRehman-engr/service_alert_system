#pragma once
#ifndef __MAX30102
#define __MAX30102
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "sdkconfig.h"
#include "spo2_algorithm.h"
#include "stdbool.h"
#include <stdio.h>
#include <string.h>

#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1
#define LAST_NACK_VAL 0x2
#define I2C_BUFFER_LENGTH 32

#define I2C_ADDR_MAX30102 0x57 // max30102 i2c address
#define i2c_port 0
#define i2c_frequency 800000
#define i2c_gpio_sda 18
#define i2c_gpio_scl 19

uint32_t getRed(void);   // Returns immediate red value
uint32_t getIR(void);    // Returns immediate IR value
uint32_t getGreen(void); // Returns immediate green value
bool safeCheck(
    uint8_t maxTimeToCheck); // Given a max amount of time, check for new data

// Configuration
void max30102_softReset(void);
void shutDown();
void wakeUp();

void setLEDMode(uint8_t mode);

void setADCRange(uint8_t adcRange);
void setSampleRate(uint8_t sampleRate);
void setPulseWidth(uint8_t pulseWidth);

void setPulseAmplitudeRed(uint8_t value);
void setPulseAmplitudeIR(uint8_t value);
void setPulseAmplitudeGreen(uint8_t value);
void setPulseAmplitudeProximity(uint8_t value);

void setProximityThreshold(uint8_t threshMSB);

// Multi-led configuration mode (page 22)
void enableSlot(uint8_t slotNumber,
                uint8_t device); // Given slot number, assign a device to slot
void disableSlots(void);

// Data Collection

// Interrupts (page 13, 14)
uint8_t getINT1(void);  // Returns the main interrupt group
uint8_t getINT2(void);  // Returns the temp ready interrupt
void enableAFULL(void); // Enable/disable individual interrupts
void disableAFULL(void);
void enableDATARDY(void);
void disableDATARDY(void);
void enableALCOVF(void);
void disableALCOVF(void);
void enablePROXINT(void);
void disablePROXINT(void);
void enableDIETEMPRDY(void);
void disableDIETEMPRDY(void);

// FIFO Configuration (page 18)
void setFIFOAverage(uint8_t samples);
void enableFIFORollover();
void disableFIFORollover();
void setFIFOAlmostFull(uint8_t samples);

// FIFO Reading
uint16_t check(void); // Checks for new data and fills FIFO
uint8_t available(
    void); // Tells caller how many new samples are available (head - tail)
void nextSample(void);       // Advances the tail of the sense array
uint32_t getFIFORed(void);   // Returns the FIFO sample pointed to by tail
uint32_t getFIFOIR(void);    // Returns the FIFO sample pointed to by tail
uint32_t getFIFOGreen(void); // Returns the FIFO sample pointed to by tail

uint8_t getWritePointer(void);
uint8_t getReadPointer(void);
void clearFIFO(void); // Sets the read/write pointers to zero

// Proximity Mode Interrupt Threshold
void setPROXINTTHRESH(uint8_t val);

// Die Temperature
float readTemperature();
float readTemperatureF();

// Detecting ID/Revision
uint8_t getRevisionID();
uint8_t readPartID();

// Setup the IC with user selectable settings
void max30102_setup(uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode,
                    int sampleRate, int pulseWidth, int adcRange);

// activeLEDs is the number of channels turned on, and can be 1 to 3. 2 is
// common for Red+IR.
uint8_t activeLEDs; // Gets set during setup. Allows check() to calculate how
                    // many bytes to read from FIFO

uint8_t revisionID;

void readRevisionID();

void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);

#define STORAGE_SIZE                                                           \
  4 // Each long is 4 bytes so limit this to fit on your micro
typedef struct Record {
  uint32_t red[STORAGE_SIZE];
  uint32_t IR[STORAGE_SIZE];
  uint8_t head;
  uint8_t tail;
} sense_struct; // This is our circular buffer of readings from the sensor

sense_struct sense;
void app_max30102(void);

#endif