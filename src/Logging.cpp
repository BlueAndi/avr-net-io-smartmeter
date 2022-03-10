/* MIT License
 *
 * Copyright (c) 2020 - 2022 Andreas Merkle <web@blue-andi.de>
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
 * @brief  Logging
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "Logging.h"

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

static bool logHead(const __FlashStringHelper* fileNameP, int line, Logging::LogType logType);

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/** Define the log level here, by adding the log types via OR together. */
static const uint8_t    LOG_LEVEL   = (Logging::LOGTYPE_INFO | Logging::LOGTYPE_ERROR | Logging::LOGTYPE_FATAL);

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/******************************************************************************
 * External Functions
 *****************************************************************************/

void Logging::logOutput(const __FlashStringHelper* fileNameP, int line, Logging::LogType logType, const char* str)
{
    if (true == logHead(fileNameP, line, logType))
    {
        // Show message
        Serial.println(str);
    }
    
    return;
}

void Logging::logOutput(const __FlashStringHelper* fileNameP, int line, Logging::LogType logType, const __FlashStringHelper* strP)
{
    if (true == logHead(fileNameP, line, logType))
    {
        // Show message
        Serial.println(strP);
    }
    
    return;
}

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/**
 * Print log message header, depended on log level.
 * 
 * @param[in] fileNameP Name of the file where the log output happens (string muste be in program memory)
 * @param[in] line      Line number where the log output happens
 * @param[in] logType   Log type
 *
 * @return If the log type is masked out, it will return false, otherwise true.
 */
static bool logHead(const __FlashStringHelper* fileNameP, int line, Logging::LogType logType)
{
    bool ret  = false;
    
    if (0 != (LOG_LEVEL & logType))
    {
        /* Show time */
        Serial.print(millis() / 1000);
        Serial.print(F(" "));
    
        /* Show name of file without path */
        Serial.print(fileNameP);
    
        /* Show line number in braces */
        Serial.print(F(" ("));
        Serial.print(line);
        Serial.print(F(") - "));
    
        /* Show log type */
        switch(logType)
        {
        case Logging::LOGTYPE_DEBUG:
            Serial.print(F("DEBUG"));
            break;

        case Logging::LOGTYPE_INFO:
            Serial.print(F("INFO"));
            break;

        case Logging::LOGTYPE_ERROR:
            Serial.print(F("ERROR"));
            break;
        
        case Logging::LOGTYPE_FATAL:
            Serial.print(F("FATAL"));
            break;
 
        default:
            Serial.print(F("?"));
            break;
        }

        Serial.print(F(": "));

        ret = true;
    }
        
    return ret;
}
