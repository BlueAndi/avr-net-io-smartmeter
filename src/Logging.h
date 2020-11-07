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
 * @brief  Logging
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @{
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

#if defined(DEBUG)

/**
 * Enable logging with a 1 or disable logging with 0 at all.
 * 
 * Attention: If logging is enabled, don't connect the device to the heatpump,
 *            because logging and heatpump control uses the same serial
 *            interface!
 */
#define LOGGING_ENABLED (1)

#else   /* not defined(DEBUG) */

#define LOGGING_ENABLED (0)

#endif  /* not defined(DEBUG) */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Arduino.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

#if LOGGING_ENABLED

/** Log debug information to the console */
#define LOG_DEBUG(__txt)  Logging::logOutput(F(__FILE__), __LINE__, Logging::LOGTYPE_DEBUG, __txt)

/** Log information to the console */
#define LOG_INFO(__txt)   Logging::logOutput(F(__FILE__), __LINE__, Logging::LOGTYPE_INFO, __txt)

/** Log error information to the console */
#define LOG_ERROR(__txt)  Logging::logOutput(F(__FILE__), __LINE__, Logging::LOGTYPE_ERROR, __txt)

/** Log fatal error information to the console */
#define LOG_FATAL(__txt)  Logging::logOutput(F(__FILE__), __LINE__, Logging::LOGTYPE_FATAL, __txt)

#else

#define LOG_DEBUG(__txt)
#define LOG_INFO(__txt)
#define LOG_ERROR(__txt)
#define LOG_FATAL(__txt)

#endif

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Logging stuff
 */
namespace Logging
{

/** This type defines the different log types. */
typedef enum
{
    LOGTYPE_DEBUG = 0x01, /**< Debug information */
    LOGTYPE_INFO  = 0x02, /**< Information */
    LOGTYPE_ERROR = 0x04, /**< Error information */
    LOGTYPE_FATAL = 0x08  /**< Fatal error information */
    
} LogType;

/******************************************************************************
 * Functions
 *****************************************************************************/

/**
 * Log a message to the console
 * 
 * @param[in] fileNameP Name of the file where the log output happens (string muste be in program memory)
 * @param[in] line      Line number where the log output happens
 * @param[in] logType   Log type
 * @param[in] str       Log message
 */
void logOutput(const __FlashStringHelper* fileNameP, int line, LogType logType, const char* str);

/**
 * Log a message to the console
 * 
 * @param[in] fileNameP Name of the file where the log output happens (string muste be in program memory)
 * @param[in] line      Line number where the log output happens
 * @param[in] logType   Log type
 * @param[in] str       Log message
 */
void logOutput(const __FlashStringHelper* fileNameP, int line, LogType logType, const __FlashStringHelper* strP);

};

#endif  /* __LOGGING_H__ */

/** @} */