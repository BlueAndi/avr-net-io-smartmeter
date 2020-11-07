/* MIT License
 *
 * Copyright (c) 2020 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/

/**
 * @brief  Persistent memory
 * @file   ps.hpp
 * @author Andreas Merkle, http://www.blue-andi.de
 *
 * @section desc Description
 * This module provides the persistent memory functions, to load and store
 * application specific data to the EEPROM.
 * 
 */

#ifndef __PSMEMORY_HPP__
#define __PSMEMORY_HPP__

/*******************************************************************************
    INCLUDES
*******************************************************************************/
#include "Config.h"
#include <EEPROM.h>

/* Namespace begin */
namespace PersistentMemory
{

/*******************************************************************************
    CONSTANTS
*******************************************************************************/

/** Address in the persistent memory for the status. */
#define PSMEMORY_STATUS_ADDR  (0)

/** Size in bytes of the status in the persistent memory. */
#define PSMEMORY_STATUS_SIZE  (1)

/** Address in the persistent memory for the number of S0 interfaces. */
#define PSMEMORY_S0NUM_ADDR   (PSMEMORY_STATUS_ADDR + PSMEMORY_STATUS_SIZE)

/** Size in bytes of the number of S0 interfaces in the persistent memory. */
#define PSMEMORY_S0NUM_SIZE   (1)

/** Address in the persistent memory for the S0 context data. */
#define PSMEMORY_S0DATA_ADDR  (PSMEMORY_S0NUM_ADDR + PSMEMORY_S0NUM_SIZE)

/** Size in bytes of the S0 context data in the persistent memory. */
#define PSMEMORY_S0DATA_SIZE  (sizeof(S0Data) * CONFIG_S0_SMARTMETER_MAX_NUM)

/** Address in the persistent memory for the debug data. */
#define PSMEMORY_S0DATA_DEBUG (PSMEMORY_S0DATA_ADDR + PSMEMORY_S0DATA_SIZE)

/*******************************************************************************
    MACROS
*******************************************************************************/

/*******************************************************************************
    CLASSES, TYPES AND STRUCTURES
*******************************************************************************/

/** This type defines function return parameters. */
typedef enum
{
    RET_OK = 0,     /**< Successful executed */
    RET_RESTORED,   /**< Persistent data restored */
    RET_ERROR       /**< Execution failed */
    
} Ret;

/** This type defines a S0 parameter block. */
struct S0Data
{
    bool        isEnabled;      /**< S0 interface enabled (true) or disabled (false) */
    char        name[32];       /**< S0 interface name in user friendly form */
    uint8_t     pinS0;          /**< S0 interface pin (must be configurable as interrupt) */
    uint32_t    pulsesPerKWH;   /**< Number of pulses per kWh */
    
    /**
     * Set default values.
     */
    S0Data() :
        isEnabled(false),
        name(),
        pinS0(0),
        pulsesPerKWH(1000)
    {
        memset(name, 0, sizeof(name));
    }

    /**
     * Copy constructer.
     *
     * @param[in] S0 data, which to assign.
     */
    S0Data(const S0Data& data) :
        isEnabled(data.isEnabled),
        name(),
        pinS0(data.pinS0),
        pulsesPerKWH(data.pulsesPerKWH)
    {
        strcpy(name, data.name);
    }

    /**
     * Assign S0 data.
     * 
     * @param[in] data  S0 data, which to assign.
     *
     * @return S0 data
     */
    S0Data& operator=(const S0Data& data)
    {
        if (this != &data)
        {
            isEnabled       = data.isEnabled;
            pinS0           = data.pinS0;
            pulsesPerKWH    = data.pulsesPerKWH;

            strcpy(name, data.name);
        }

        return *this;
    }

};

/**
 * This type defines the status pattern, used to check the status of the data in the
 * persistent memory.
 */
typedef enum
{
    STATUS_VALID = 0xa5 /**< Persistent memory is initialized and valid, otherwise not. */
    
} Status;

/*******************************************************************************
    FUNCTIONS
*******************************************************************************/

/**
 * Initialize the persistent memory module.
 *
 * If the data in the persistent memory is not valid, it will be replaced by
 * default values.
 *
 * @return If the persistent memory data is replaced with defaults, it will return RET_RESTORED.
 *         Otherwise it will return RET_OK, if successfuly loaded.
 */
Ret init(void)
{
    Ret     ret     = RET_OK;
    uint8_t status  = EEPROM.read(PSMEMORY_STATUS_ADDR);
    uint8_t s0Num   = EEPROM.read(PSMEMORY_S0NUM_ADDR);

    if ((STATUS_VALID != status) ||
        (CONFIG_S0_SMARTMETER_MAX_NUM != s0Num))
    {
        uint8_t index         = 0;
        S0Data  s0DataDefault;

        EEPROM.write(PSMEMORY_S0NUM_ADDR, CONFIG_S0_SMARTMETER_MAX_NUM);
        
        for(index = 0; index < CONFIG_S0_SMARTMETER_MAX_NUM; ++index)
        {
            snprintf(s0DataDefault.name, sizeof(s0DataDefault.name), "S0-%u", index);
            EEPROM.put(PSMEMORY_S0DATA_ADDR + index * sizeof(S0Data), s0DataDefault);
        }

        EEPROM.write(PSMEMORY_S0DATA_DEBUG, 0);
        EEPROM.write(PSMEMORY_STATUS_ADDR, STATUS_VALID);

        ret = RET_RESTORED;
    }
    
    return ret;
}

/**
 * Get number of s0 parameter blocks in the persistent memory.
 *
 * @return Number of S0 parameter blocks.
 */
uint8_t getNumS0Data(void)
{
    return CONFIG_S0_SMARTMETER_MAX_NUM;
}

/**
 * Read S0 parameter block from persistent memory.
 *
 * @param[in]   index   Index of S0 parameter block
 * @param[out]  s0Data  Parameter block
 */
void readS0Data(uint8_t index, S0Data& s0Data)
{
    S0Data s0DataDefault;

    s0Data = s0DataDefault;

    if (CONFIG_S0_SMARTMETER_MAX_NUM > index)
    {
        EEPROM.get(PSMEMORY_S0DATA_ADDR + index * sizeof(S0Data), s0Data);
    }
    
    return;
}

/**
 * Write S0 parameter block to persistent memory.
 *
 * @param[in]   index   Index of S0 parameter block
 * @param[out]  s0Data  Parameter block
 */
void writeS0Data(uint8_t index, const S0Data& s0Data)
{
    if (CONFIG_S0_SMARTMETER_MAX_NUM > index)
    {
        EEPROM.put(PSMEMORY_S0DATA_ADDR + index * sizeof(S0Data), s0Data);
    }
    
    return;
}
 
/* Namespace end */ 
};

#endif  /* __PSMEMORY_HPP__ */
