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
 * @brief  Web request router
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @{
 */

#ifndef __WEB_REQ_ROUTER_H__
#define __WEB_REQ_ROUTER_H__

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <ArduinoHttpServer.h>

#include "EthernetClient.h"

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Http request stream buffer type.
 */
typedef ArduinoHttpServer::StreamHttpRequest<256> HttpRequest;

/**
 * The web request router is responsible to route a web request to the
 * right route and handle it.
 *
 * @tparam[in] NUM_OF_ROUTES    Max. number of routes, which can be added.
 */
template < uint8_t NUM_OF_ROUTES >
class WebReqRouter
{
public:

    /**
     * Web request handler.
     *
     * @param[in] client        The ethernet client, used for the response.
     * @param[in] httpRequest   The web request itself.
     */
    typedef void (*WebReqHandler)(EthernetClient& client, const HttpRequest& httpRequest);

    /**
     * A single route.
     */
    struct Route
    {
        ArduinoHttpServer::Method   method;     /**< Http request method */
        String                      uri;        /**< Http request URI */
        WebReqHandler               handler;    /**< Handler of the web request */

        /**
         * Constructs a empty route.
         */
        Route() :
            method(ArduinoHttpServer::Method::Invalid),
            uri(),
            handler(nullptr)
        {
        }

        /**
         * Destroys a route.
         */
        ~Route()
        {
        }
    };

    /**
     * Constructs a empty router.
     */
    WebReqRouter() :
        m_routes()
    {
    }

    /**
     * Destroys the router.
     */
    ~WebReqRouter()
    {
    }

    /**
     * Add a single route.
     *
     * @param[in] method    Http request method
     * @param[in] uri       Http request URI
     * @param[in] handler   Http request handler
     *
     * @return If the route is added, it will return true otherwise false.
     */
    bool addRoute(ArduinoHttpServer::Method method, const String& uri, WebReqHandler handler)
    {
        uint8_t idx         = 0;
        bool    isSlotFound = false;

        while((NUM_OF_ROUTES > idx) && (false == isSlotFound))
        {
            if (0 == m_routes[idx].uri.length())
            {
                m_routes[idx].method    = method;
                m_routes[idx].uri       = uri;
                m_routes[idx].handler   = handler;

                isSlotFound = true;
            }
            else
            {
                ++idx;
            }
        }

        return isSlotFound;
    }

    /**
     * Handle a web request.
     *
     * @param[in] client        The ethernet client, used for the response.
     * @param[in] httpRequest   The http web request.
     *
     * @return If the request is handled, it will return true otherwise false.
     */
    bool handle(EthernetClient& client, const HttpRequest& httpRequest)
    {
        uint8_t idx             = 0;
        bool    isRouteFound    = false;

        while((NUM_OF_ROUTES > idx) && (false == isRouteFound))
        {
            if ((httpRequest.getMethod() == m_routes[idx].method) &&
                (0 < m_routes[idx].uri.length()))
            {
                int lastIndex = m_routes[idx].uri.lastIndexOf('?');

                /* Contans URI a dynamic part? */
                if (0 <= lastIndex)
                {
                    String staticUriPart = m_routes[idx].uri.substring(0, lastIndex - 1);

                    if (0 != httpRequest.getResource().toString().startsWith(staticUriPart))
                    {
                        isRouteFound = true;
                    }
                }
                else
                /* No dynamic part in URI */
                {
                    if (0 != m_routes[idx].uri.equals(httpRequest.getResource().toString()))
                    {
                        isRouteFound = true;
                    }
                }
            }

            if (false == isRouteFound)
            {
                ++idx;
            }
        }

        if (true == isRouteFound)
        {
            m_routes[idx].handler(client, httpRequest);
        }

        return isRouteFound;
    }

private:

    Route   m_routes[NUM_OF_ROUTES];    /**< All added routes. */

};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* __WEB_REQ_ROUTER_H__ */

/** @} */