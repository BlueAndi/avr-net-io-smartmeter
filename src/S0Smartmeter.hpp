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
 * @brief  S0 Smartmeter
 * @file   s0smartmeter.hpp
 * @author Andreas Merkle, http://www.blue-andi.de
 *
 * @section desc Description
 * This module provides S0 smartmeter functionality.
 * 
 */

#ifndef __S0SMARTMETER_HPP__
#define __S0SMARTMETER_HPP__
 
/*******************************************************************************
    INCLUDES
*******************************************************************************/
#include <util/atomic.h>

/*******************************************************************************
    CONSTANTS
*******************************************************************************/

/*******************************************************************************
    MACROS
*******************************************************************************/

/*******************************************************************************
    CLASSES, TYPES AND STRUCTURES
*******************************************************************************/

/**
 * Handle a pin on port A, connected to a S0 signal.
 */
class S0Pin
{
public:

    /**
     * Constructs a S0 pin instance.
     */
    S0Pin()
    {
    }
    
    /**
     * Destructs a S0 pin instance.
     */
    ~S0Pin()
    {
    }
    
    /**
     * Configure pin to ditial input with pullup.
     *
     * @return If pin is not a port A pin, it will return false, otherwise true.
     */
    bool init(uint8_t pinNumber)
    {
        bool status = false;
        
        if ((mcPinRangeMin <= pinNumber) &&
            (mcPinRangeMax >= pinNumber))
        {       
            m_pinNo = pinNumber;
            
            pinMode(m_pinNo, INPUT_PULLUP);
            
            status = true;
        }
        
        return status;
    }
    
    /**
     * Enable on change interrupt for the pin by setting only the mask.
     */
    void enable() const
    {        
        /* attachInterrupt() and digitalPinToInterrupt() can not be used, because
         * arduino pinout doesn't support pin change interrupts on the whole port A.
         * Otherwise it would be easy: attachInterrupt(digitalPinToInterrupt(mPinS0), mISR, FALLING);
         */
        PCMSK0 |= _BV(m_pinNo - mcPinRangeMin);
        
        return;
    }
    
    /**
     * Enable on change interrupt for the pin by clearing only the mask.
     */
    void disable() const
    {
        /* deatachInterrupt() and digitalPinToInterrupt() can not be used, because
         * arduino pinout doesn't support pin change interrupts on the whole port A.
         * Otherwise it would be easy: detachInterrupt(digitalPinToInterrupt(mPinS0));
         */
        PCMSK0 &= ~_BV(m_pinNo - mcPinRangeMin);
        
        return;
    }
    
    /**
     * Get S0 pin number.
     *
     * @return S0 pin number
     */
    uint8_t getPinNumber() const
    {
        return m_pinNo;
    }
    
    /**
     * Get port bit number.
     *
     * @return Port bit number
     */
    uint8_t getPortBitNo() const
    {
        return m_pinNo - mcPinRangeMin;
    }
    
    static const uint8_t    mcPinRangeMin   = 24;  /**< Lowest arduino pin number of port A */
    static const uint8_t    mcPinRangeMax   = 31;  /**< Highest arduino pin number of port A */

private:

    uint8_t m_pinNo;    /**< Arduino pin number */

    S0Pin(const S0Pin& pin);
    S0Pin& operator=(const S0Pin& pin);
};

/**
 * S0 smartmeter class.
 * It calculates power and energy consumption, derived from S0 pulses.
 *
 * The pulses for counting the energy, will be recognized by a interrupt
 * service routine, which calls the internalISR() method.
 *
 */
class S0Smartmeter
{
public:

    /**
     * Constructs a S0 smartmeter instance.
     */
    S0Smartmeter() :
        m_id(UINT8_MAX),
        m_isEnabled(false),
        m_name(""),
        m_energyPerPulse(0),
        m_s0Pin(),
        m_pulseCnt(0),
        m_timestamp(0L),
        m_powerConsumption(0L),
        m_isUpdated(false),
        m_timestampLastReq(0L)
    {
        
    }

    /**
     * Destroys a S0 smartmeter instance.
     */
    ~S0Smartmeter()
    {
      
    }

    /**
     * Initialize a S0 smartmeter instance.
     * 
     * @param[in] id            Unique id
     * @param[in] name          User friendly name of the S0 smartmeter
     * @param[in] pinS0         Arduino pin number, where the S0 is connected to
     * @param[in] pulsePerKWH   How many pulses for 1 kWh
     *
     * @return If S0 smartmeter is successful initialized, it will return true, otherwise false.
     */
    bool init(uint8_t id, const char* name, uint8_t pinS0, uint32_t pulsePerKWH)
    {
        bool status = false;
        
        if ((true == m_s0Pin.init(pinS0)) &&
            (PULSES_PER_KWH_RANGE_MIN <= pulsePerKWH) &&
            (PULSES_PER_KWH_RANGE_MAX >= pulsePerKWH))
        {
            m_id                = id;
            m_name              = name;
            m_energyPerPulse    = 60UL * 60UL * 1000UL / pulsePerKWH;
            
            status = true;
        }
        
        return status;
    }

    /**
     * Enable S0 smartmeter.
     */
    void enable(void)
    {
        m_s0Pin.enable();
        
        m_isEnabled = true;
        return;
    }

    /**
     * Disable S0 smartmeter.
     */
    void disable(void)
    {
        m_s0Pin.disable();

        m_isEnabled = false;
        return;
    }

    /**
     * Is S0 smartmeter enabled?
     * 
     * @return In case the S0 smartmeter is enabled it will return true otherwise false.
     */
    bool isEnabled(void) const
    {
        return m_isEnabled;
    }

    /**
     * Get S0 smartmeter id.
     * 
     * @return Id
     */
    uint8_t getId(void) const
    {
        return m_id;
    }

    /**
     * Get name of the S0 smartmeter.
     * 
     * @return Interface name
     */
    const String& getName(void) const
    {
        return m_name;
    }
    
    /**
     * Get S0 pin number.
     *
     * @return S0 pin number
     */
    const S0Pin& getS0Pin(void) const
    {
        return m_s0Pin;
    }
    
    /**
     * Get current number of counted pulses.
     *
     * @return Counted pulses
     */
    uint32_t getPulseCnt(void) const
    {
        uint32_t pulseCnt = 0;
        
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            pulseCnt = m_pulseCnt;
        }
        
        return pulseCnt;
    }

    /**
     * Handle a S0 smartmeter update. Call this in the external interrupt service routine.
     * Never call it outside an ISR!
     */
    void internalISR(void)
    {
        unsigned long timestamp = millis();
        unsigned long diff      = timestamp - m_timestamp;
    
        ++m_pulseCnt;
    
        /* Store current timestamp of this pulse */
        m_timestamp = timestamp;
    
        /* Calculate average power consumption over 2 values
         * Current power consumption = energy per pulse / time between two pulses
         * Avg. power consumption = (current power consumption + last power consumption) / 2
         */
        m_powerConsumption += ( m_energyPerPulse * 1000 ) / diff;
        m_powerConsumption /= 2;
    
        if (false == m_isUpdated)
        {
            m_timestampLastReq = timestamp;
            m_isUpdated        = true;
        }
        
        return;
    }

    /**
     * Get current result of power and energy consumption.
     * 
     * Note, every call of this method will reset the pulse counter!
     * 
     * @param[out] powerConsumption   Power consumption in W
     * @param[out] energyConsumption  Energy consumption in Ws
     * @param[out] pulseCnt           Number of pulses counted since last call
     * @param[out] durationLastReq    Duration in ms, from last update till this call
     */
    void getResult(unsigned long& powerConsumption, unsigned long& energyConsumption, uint32_t& pulseCnt, unsigned long& durationLastReq)
    {
        unsigned long timestampLastReq  = 0;
    
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            powerConsumption  = m_powerConsumption;
            pulseCnt          = m_pulseCnt;
            timestampLastReq  = m_timestampLastReq;
    
            /* Reset pulse counter */
            m_pulseCnt = 0;

            /* Wait for next update */
            m_isUpdated = false;
        }

        energyConsumption = pulseCnt * m_energyPerPulse;
        durationLastReq   = millis() - timestampLastReq;
        
        return;
    }
    
    /** Minimum value for pulses per kWh, used for range check. */
    static const uint32_t   PULSES_PER_KWH_RANGE_MIN    = 1;

    /** Maximum value for pulses per kWh, used for range check. */
    static const uint32_t   PULSES_PER_KWH_RANGE_MAX    = 6000;

private:

    uint8_t     m_id;               /**< S0 smartmeter id */
    bool        m_isEnabled;        /**< S0 smartmeter is enabled or disabled */
    String      m_name;             /**< S0 smartmeter name in user friendly form */
    uint32_t    m_energyPerPulse;   /**< Energy per pulse in Ws */
    S0Pin       m_s0Pin;            /**< S0 pin configuration */

    volatile uint32_t       m_pulseCnt;         /**< Counted pulses */
    volatile unsigned long  m_timestamp;        /**< Timestamp in ms of last pulse */
    volatile unsigned long  m_powerConsumption; /**< Average power consumption in W (calculated over 2 values) */
    volatile bool           m_isUpdated;        /**< Marks whether the S0 smartmeter is updated or not, since last request by user */
    volatile unsigned long  m_timestampLastReq; /**< Timestamp in ms before last request by user */

    /* Never copy an S0 smartmeter instance! */
    S0Smartmeter(const S0Smartmeter& interf);
    S0Smartmeter& operator=(const S0Smartmeter& interf);

};

/*******************************************************************************
    FUNCTIONS
*******************************************************************************/

#endif  /* __S0SMARTMETER_HPP__ */
