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
        m_pulsesPerKWH(1000),
        m_energyPerPulse(0),
        m_s0Pin(),
        m_pulseCnt(0),
        m_timestamp(0L),
        m_isFirstPulse(true),
        m_powerConsumption(0L),
        m_lastTimeDiff(0L)
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
     * @param[in] pulsesPerKWH  How many pulses for 1 kWh
     *
     * @return If S0 smartmeter is successful initialized, it will return true, otherwise false.
     */
    bool init(uint8_t id, const char* name, uint8_t pinS0, uint32_t pulsesPerKWH)
    {
        bool status = false;
        
        if ((true == m_s0Pin.init(pinS0)) &&
            (PULSES_PER_KWH_RANGE_MIN <= pulsesPerKWH) &&
            (PULSES_PER_KWH_RANGE_MAX >= pulsesPerKWH))
        {
            m_id                = id;
            m_name              = name;
            m_pulsesPerKWH      = pulsesPerKWH;
            m_energyPerPulse    = 60UL * 60UL * 1000UL / pulsesPerKWH;
            
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
     * Get the number of pulses for 1 kWh.
     * 
     * @return Pulses per 1 kWh
     */
    uint32_t getPulsesPerKWh(void) const
    {
        return m_pulsesPerKWH;
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
    
        /* Count the pulse continuously */
        ++m_pulseCnt;
    
        /* Store current timestamp of this pulse */
        m_timestamp = timestamp;
    
        /* Start calculation the power consumption with the 2nd pulse.
         * The first pulse is just used to initialize the m_timestamp.
         */
        if (false == m_isFirstPulse)
        {
            /* Calculate time till the last pulse. */
            m_lastTimeDiff      = timestamp - m_timestamp;

            /* Calculate time after which the power will be decreased manually, because waiting for next pulse. */
            m_decPowerDuration  = 2 * m_lastTimeDiff;

            /* Calculate current power consumption. */
            m_powerConsumption = ( m_energyPerPulse * 1000 ) / m_lastTimeDiff;
        }
            
        return;
    }

    /**
     * Get current result of power and energy consumption.
     * 
     * @param[out] powerConsumption   Power consumption in W
     * @param[out] energyConsumption  Energy consumption in Ws
     * @param[out] pulseCnt           Number of pulses counted since last call
     */
    void getResult(unsigned long& powerConsumption, unsigned long& energyConsumption, uint32_t& pulseCnt)
    {    
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            powerConsumption  = m_powerConsumption;
            pulseCnt          = m_pulseCnt;
        }

        energyConsumption = pulseCnt * m_energyPerPulse;
        
        return;
    }

    /**
     * Handle S0 smartmeter power consumption calculation.
     * This mechanism is used in case that a high power consumption changes to a low power consumption.
     * A low power consumption means the time to the next pulse increases. If in between the current
     * power is requested, the last calculated high power consumption will be returned. This is bad!
     * Therefore the power will be decreased by (2^x) * last time between two pulses.
     */
    void process(void)
    {
        /* S0 smartmeter must be enabled and the mechanism can only be used
         * in case at least 2 pulses were received at all.
         */
        if ((true == m_isEnabled) && (false == m_isFirstPulse))
        {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                /* If power is greater than 0, it will be checked whether it is time to decrease the power consumption. */
                if (0 < m_powerConsumption)
                {
                    unsigned long timeTillLastPulse = millis() - m_timestamp;

                    /* Time to decrease the power consumption? */
                    if (m_decPowerDuration <= timeTillLastPulse)
                    {
                        unsigned long delta = ( m_energyPerPulse * 1000 ) / m_decPowerDuration; /* W */

                        if (1 >= delta)
                        {
                            m_powerConsumption = 0;
                        }
                        else if (delta >= m_powerConsumption)
                        {
                            m_powerConsumption = 0;
                        }
                        else
                        {
                            m_powerConsumption -= delta;
                        }

                        /* Calculate next time for decreasing the power consumption again. */
                        m_decPowerDuration *= 2;
                    }
                }
            }
        }
    }
    
    /** Minimum value for pulses per kWh, used for range check. */
    static const uint32_t   PULSES_PER_KWH_RANGE_MIN    = 1;

    /** Maximum value for pulses per kWh, used for range check. */
    static const uint32_t   PULSES_PER_KWH_RANGE_MAX    = 6000;

private:

    uint8_t     m_id;               /**< S0 smartmeter id */
    bool        m_isEnabled;        /**< S0 smartmeter is enabled or disabled */
    String      m_name;             /**< S0 smartmeter name in user friendly form */
    uint32_t    m_pulsesPerKWH;     /**< Number of pulses for 1 kWh. */
    uint32_t    m_energyPerPulse;   /**< Energy per pulse in Ws */
    S0Pin       m_s0Pin;            /**< S0 pin configuration */

    volatile uint32_t       m_pulseCnt;         /**< Counted pulses */
    volatile unsigned long  m_timestamp;        /**< Timestamp in ms of last pulse */
    volatile bool           m_isFirstPulse;     /**< Used to initialize the m_timestamp with the first pulse. */
    volatile unsigned long  m_powerConsumption; /**< Power consumption in W */
    volatile unsigned long  m_lastTimeDiff;     /**< Last duration in ms between the last 2 pulses. */
    volatile unsigned long  m_decPowerDuration; /**< Duration until the power will be decreased automatically because no pulse received yet. */

    /* Never copy an S0 smartmeter instance! */
    S0Smartmeter(const S0Smartmeter& interf);
    S0Smartmeter& operator=(const S0Smartmeter& interf);

};

/*******************************************************************************
    FUNCTIONS
*******************************************************************************/

#endif  /* __S0SMARTMETER_HPP__ */
