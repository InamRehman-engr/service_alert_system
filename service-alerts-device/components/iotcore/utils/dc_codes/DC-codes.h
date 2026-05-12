#ifndef _dc_codes_h_
#define _dc_codes_h_

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union {
  struct {
    uint32_t reserved00 : 1;
    uint32_t reserved01 : 1;
    uint32_t reserved02 : 1;
    uint32_t OutOfRange : 1;
    uint32_t I2CFault : 1;
    uint32_t SPIFault : 1;
    uint32_t UARTError : 1;
    uint32_t reserved07 : 1;
    uint32_t BATTLOW : 1;
    uint32_t OTAFAILED : 1;
    uint32_t reserved10 : 1;
    uint32_t reserved11 : 1;
    uint32_t HeapLow : 1;
    uint32_t watchdogoverflow : 1;
    uint32_t ThreadFaild : 1;
    uint32_t stackoverflow : 1;
    uint32_t mqttMessageOverflow : 1;
    uint32_t reserved17 : 1;
    uint32_t reading_tanksensor : 1;
    uint32_t CommunicationErrBetweenMotorAndTank : 1;
    uint32_t slaveDeviceError : 1;
    uint32_t reserved21 : 1;
    uint32_t reserved22 : 1;
    uint32_t reserved23 : 1;
    uint32_t reserved24 : 1;
    uint32_t reserved25 : 1;
    uint32_t reserved26 : 1;
    uint32_t reserved27 : 1;
    uint32_t reserved28 : 1;
    uint32_t reserved29 : 1;
    uint32_t reserved30 : 1;
    uint32_t reserved31 : 1;
  } flags;
  uint32_t all;
} dc_codes_bits;

#endif