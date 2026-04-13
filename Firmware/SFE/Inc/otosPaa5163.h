/*
    SPDX-License-Identifier: MIT
    
    Copyright (c) 2024 SparkFun Electronics
*/

#pragma once

#include "main.h"
#include "sfePaa5163.h"
#include "sfeTkStmSPI.h"
#include "adc.h"
#include "time.h"

class PAA5163 : public sfePaa5163
{
  public:
    sfeTkError_t begin()
    {
        // First we need to power up the PAA5163
        sfeTkError_t err = powerUpSequence();
        if(err != kSTkErrOk)
            return err;

        // Now we need SPI, so initialize the SPI bus
        err = _spiBus.init(PAA_CS_GPIO_Port, PAA_CS_Pin, false);
        if(err != kSTkErrOk)
            return err;

        return sfePaa5163::begin(&_spiBus);
    }
    sfeTkError_t powerUpSequence()
    {
        // 1. Apply power to VDD and VDDIO (any order, max 100ms apart)
        // Note: VDD_VCSEL should be powered on together with the second supply
        LL_GPIO_ResetOutputPin(PAA_POW_GPIO_Port, PAA_POW_Pin);
        LL_GPIO_ResetOutputPin(VCSEL_POW_GPIO_Port, VCSEL_POW_Pin); // Updated: PAA5163 sequence [cite: 138]

        // Wait for voltage stabilization (Datasheet step 2: Wait at least 50ms) 
        delayMillis(50);

        // 2. Measure the 1.8V net (PAA5163 VDD range is 1.8V to 2.1V) [cite: 135]
        uint16_t adcMillivolts;
        if(readADCMillivolts(adcMillivolts) == false)
            return kSTkErrFail;
        if (adcMillivolts < 1700 || adcMillivolts > 2150) // Range adjusted for 5163 specs [cite: 135]
        {
            return kSTkErrFail;
        }

        // 3. Reset SPI Port: Drive NCS high for at least 0.1ms 
        LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_5); // Assuming Pin 5 is NCS
        delayMicroseconds(100); 

        // 4. Hardware Reset: Assert NRST for >20 microseconds [cite: 142, 173]
        LL_GPIO_ResetOutputPin(PAA_RST_GPIO_Port, PAA_RST_Pin);
        delayMicroseconds(20);
        LL_GPIO_SetOutputPin(PAA_RST_GPIO_Port, PAA_RST_Pin);

        // 5. Post-Reset Delay: Datasheet requires 120ms for valid motion after reset 
        // (If you use software reset 0x3A later, this delay is mandatory)
        delayMillis(120);

        return kSTkErrOk;
    }

  protected:
    void delayMillis(uint32_t milliseconds)
    {
        delay(milliseconds);
    }

  private:
    // SPI interface
    sfeTkStmSPI _spiBus;
};