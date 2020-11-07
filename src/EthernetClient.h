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
 * @brief  Ethernet client
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @{
 */

#ifndef __ETHERNET_CLIENT_H__
#define __ETHERNET_CLIENT_H__

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Print.h>
#include <Stream.h>
#include <EtherCard.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * The ethernet client adapts the ethernet driver (EtherCard) to standard
 * Arduino EthernetClient. Only the minimal subset of methods are provided.
 */
class EthernetClient : public Stream
{
public:

    /**
     * Constructs the ethernet client.
     * 
     * @param[in] tcpSdu        TCP SDU
     * @param[in] tcpSduSize    Size in byte of the TCP SDU
     */
    EthernetClient(const uint8_t* tcpSdu, uint16_t tcpSduSize) :
        m_tcpSdu(tcpSdu),
        m_tcpSduSize(tcpSduSize),
        m_readPos(0),
        m_bufferFiller()
    {
    }

    /**
     * Destroys the ethernet client.
     */
    ~EthernetClient()
    {
    }

    /**
     * Get the number of available data.
     * 
     * @return Number of byte which are available
     */
    int available() override;

    /**
     * Read a single data byte.
     * 
     * @return Single data byte
     */
    int read() override;

    /**
     * Read a single data byte, but without increasing the internal read position.
     * 
     * @return Single data byte
     */
    int peek() override;

    /**
     * Not supported!
     * 
     * @param[in] data  Single data byte
     * 
     * @return Number of written data byte
     */
    size_t write(uint8_t data) override
    {
        /* Not supported. */
        return 0;
    }

    /**
     * Write a TCP message.
     * 
     * @param[in] buffer    Message buffer
     * @param[in] size      Message buffer size in byte
     * 
     * @return Number of written data byte
     */
    size_t write(const uint8_t* buffer, size_t size) override;

private:
    
    const uint8_t*  m_tcpSdu;       /**< TCP SDU, containing a HTTP message. */
    uint16_t        m_tcpSduSize;   /**< TCP SDU size in byte */
    uint16_t        m_readPos;      /**< Read index inside the TCP SDU. */
    BufferFiller    m_bufferFiller; /**< HTTP response buffer, which is connected to the ethernet driver. */

    EthernetClient();
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* __ETHERNET_CLIENT_H__ */

/** @} */