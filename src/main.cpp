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
 * @brief  Main entry point
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <EthernetENC.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <ArduinoHttpServer.h>
#include <ArduinoJson.h>

#include "Logging.h"
#include "WebReqRouter.h"
#include "SimpleTimer.hpp"

#include "PSMemory.hpp"
#include "S0Smartmeter.hpp"

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/** This type defines link possible ethernet link status. */
typedef enum
{
    LINK_STATUS_UNKNOWN = 0,  /**< Unknown link status */
    LINK_STATUS_DOWN,         /**< Link is up */
    LINK_STATUS_UP            /**< Link is down */

} LinkStatus;

/**
 * Status id codes for JSON responses.
 */
typedef enum
{
    STATUS_ID_OK = 0,   /**< Successful */
    STATUS_ID_EPENDING, /**< Already pending */
    STATUS_ID_EINPUT,   /**< Input data invalid */
    STATUS_ID_EPAR,     /**< Parameter is missing */
    STATUS_ID_EINTERNAL,/**< Unknown internal error */
    STATUS_ID_EINVALID  /**< Response is invalid */

} StatusId;

/******************************************************************************
 * Prototypes
 *****************************************************************************/

static String ipToStr(IPAddress ip);
static void printNetworkSettings(void);
static void handleNetwork(void);
static void handleRoot(EthernetClient& client, const HttpRequest& httpRequest);
static void s0Smartmeter2JSON(S0Smartmeter& s0Smartmeter, JsonObject& jsonData);
static void handleS0InterfaceReq(EthernetClient& client, const HttpRequest& httpRequest);
static void handleS0InterfacesReq(EthernetClient& client, const HttpRequest& httpRequest);
static void handleConfigureGetReq(EthernetClient& client, const HttpRequest& httpRequest);
static void handleConfigurePostReq(EthernetClient& client, const HttpRequest& httpRequest);

/******************************************************************************
 * Variables
 *****************************************************************************/

#if defined(DEBUG)

/** Serial interface baudrate. */
static const uint32_t           SERIAL_BAUDRATE             = 115200U;

#else

/** Serial interface baudrate. */
static const uint32_t           SERIAL_BAUDRATE             = 19200U;

#endif  /* defined(DEBUG) */

/** Ethernet interface MAC address */
static const uint8_t            DEVICE_MAC_ADDR[]           = { 0x00, 0x22, 0xf9, 0x01, 0x27, 0xeb };

/** Current ethernet link status. */
static LinkStatus               gLinkStatus                 = LINK_STATUS_UNKNOWN;

/** HTML page header, stored in program memory. */
static const char               HTML_PAGE_HEAD[] PROGMEM    = "<!DOCTYPE html>\r\n"
                                                            "<html>\r\n"
                                                            "<head>\r\n"
                                                            "<title>AVR-NET-IO-Smartmeter</title>\r\n"
                                                            "</head>\r\n"
                                                            "<body>\r\n";

/** HTML page footer, stored in program memory. */
static const char               HTML_PAGE_TAIL[] PROGMEM    = "</body>\r\n"
                                                            "</html>";

/** Number of supported web request routes. */
static const uint8_t            NUM_ROUTES                  = 5;

/** Web request router */
static WebReqRouter<NUM_ROUTES> gWebReqRouter;

/** Webserver port number */
static const uint16_t           WEB_SRV_PORT                = 80;

/** Webserver */
static EthernetServer           gWebServer(WEB_SRV_PORT);

/** All S0 interface instances */
static S0Smartmeter             gS0Smartmeters[CONFIG_S0_SMARTMETER_MAX_NUM];

/******************************************************************************
 * External functions
 *****************************************************************************/

/**
 * Setup the system.
 */
void setup()
{
    bool isError = false;

    /* Setup serial interface */
    Serial.begin(SERIAL_BAUDRATE);

    LOG_INFO(F("Device starts up."));

    if (0 == Ethernet.begin(DEVICE_MAC_ADDR))
    {
        if (LinkOFF == Ethernet.linkStatus())
        {
            LOG_INFO(F("Ethernet cable not connected."));
        }
        else if (EthernetNoHardware == Ethernet.hardwareStatus())
        {
            LOG_ERROR(F("Ethernet controller not found."));
            isError = true;
        }
        else
        {
            LOG_ERROR(F("Couldn't initialize ethernet controller."));
            isError = true;
        }
    }

    if (false == isError)
    {
        uint8_t                 index = 0;
        String                  tmp;
        PersistentMemory::Ret   psRet = PersistentMemory::RET_ERROR;

        LOG_INFO(F("Ethernet controller initialized."));

        if (false == gWebReqRouter.addRoute(ArduinoHttpServer::Method::Get, "/", handleRoot))
        {
            LOG_ERROR(F("Failed to add route."));
        }

        if (false == gWebReqRouter.addRoute(ArduinoHttpServer::Method::Get, "/api/s0-interface/?", handleS0InterfaceReq))
        {
            LOG_ERROR(F("Failed to add route."));
        }

        if (false == gWebReqRouter.addRoute(ArduinoHttpServer::Method::Get, "/api/s0-interfaces", handleS0InterfacesReq))
        {
            LOG_ERROR(F("Failed to add route."));
        }

        if (false == gWebReqRouter.addRoute(ArduinoHttpServer::Method::Get, "/api/configure/?", handleConfigureGetReq))
        {
            LOG_ERROR(F("Failed to add route."));
        }

        if (false == gWebReqRouter.addRoute(ArduinoHttpServer::Method::Post, "/api/configure/?", handleConfigurePostReq))
        {
            LOG_ERROR(F("Failed to add route."));
        }

        LOG_INFO(F("Setup persistent memory."));
        psRet = PersistentMemory::init();

        if (PersistentMemory::RET_RESTORED == psRet)
        {
            LOG_INFO(F("Persistent memory restored."));
        }
        else if (PersistentMemory::RET_OK != psRet)
        {
            LOG_FATAL(F("Failed to initialize persistent memory."));
        }
        else
        {
            LOG_INFO(F("Persistent memory is valid."));
        }

        LOG_INFO(F("Setup S0 interfaces."));
        for(index = 0; index < CONFIG_S0_SMARTMETER_MAX_NUM; ++index)
        {
            PersistentMemory::S0Data psS0Data;

            PersistentMemory::readS0Data(index, psS0Data);

            /* Shall the interface be enabled? */
            if (true == psS0Data.isEnabled)
            {
                tmp = F("Init. and enable interface ");
                tmp += index;
                tmp += F(" ");
                tmp += psS0Data.name;
                tmp += F(" at pin ");
                tmp += psS0Data.pinS0;
                LOG_INFO(tmp.c_str());

                /* Initialize S0 interface */
                if (false == gS0Smartmeters[index].init(index,
                                                        psS0Data.name,
                                                        psS0Data.pinS0,
                                                        psS0Data.pulsesPerKWH))
                {
                    LOG_ERROR(F("Failed to initialize S0 interface."));
                }
                else
                {
                    gS0Smartmeters[index].enable();
                }
            }
        }

        /* Start listening for clients. */
        gWebServer.begin();

        /* Enable pin change interrupt 0 in general, because the S0 interfaces
         * are all on port A of the ATmega644.
         */
        PCICR |= _BV(PCIE0);
    }

    if (true == isError)
    {
        /* Wait infinite. */
        while(1)
        {
            delay(1);
        }
    }

    return;
}

/**
 * Main loop, which is called periodically.
 */
void loop()
{
    handleNetwork();

    return;
}

/******************************************************************************
 * Local functions
 *****************************************************************************/

/**
 * Convert IP-address in byte form to user friendly string.
 *
 * @param[in] ip    IP address
 *
 * @return IP-address in user friendly form
 */
static String ipToStr(IPAddress ip)
{
    String  ipAddr;
    uint8_t idx = 0;

    for(idx = 0; idx < 4; ++idx)
    {
        if (0 < idx)
        {
            ipAddr += ".";
        }

        ipAddr += ip[idx];
    }

    return ipAddr;
}

/**
 * Show network settings.
 */
static void printNetworkSettings(void)
{
    String tmp;

    tmp = F("IP     : ");
    tmp += ipToStr(Ethernet.localIP());
    LOG_INFO(tmp.c_str());

    tmp = F("Subnet : ");
    tmp += ipToStr(Ethernet.subnetMask());
    LOG_INFO(tmp.c_str());

    tmp = F("Gateway: ");
    tmp += ipToStr(Ethernet.gatewayIP());
    LOG_INFO(tmp.c_str());

    tmp = F("DNS    : ");
    tmp += ipToStr(Ethernet.dnsServerIP());
    LOG_INFO(tmp.c_str());

    return;
}

/**
 * Handle network and webserver requests.
 */
static void handleNetwork(void)
{
    EthernetLinkStatus  linkStatus = Ethernet.linkStatus();

    Ethernet.maintain();

    /* Link status unknown? */
    if (Unknown == linkStatus)
    {
        if (LINK_STATUS_UNKNOWN != gLinkStatus)
        {
            LOG_INFO(F("Link is unknown."));
        }

        gLinkStatus = LINK_STATUS_UNKNOWN;
    }
    /* Link down? */
    else if (LinkOFF == linkStatus)
    {
        if (LINK_STATUS_DOWN != gLinkStatus)
        {
            LOG_INFO(F("Link is down."));
        }

        gLinkStatus = LINK_STATUS_DOWN;
    }
    else
    /* Link is up */
    {
        EthernetClient client = gWebServer.available();

        if (LINK_STATUS_UP != gLinkStatus)
        {
            LOG_INFO(F("Link is up."));

            printNetworkSettings();
        }

        gLinkStatus = LINK_STATUS_UP;

        if (true == client)
        {
            HttpRequest httpRequest(client);

            /* Parse the request */
            if (true == httpRequest.readRequest())
            {
                if (false == gWebReqRouter.handle(client, httpRequest))
                {
                    /* Send a 404 back, which means "Not Found" */
                    ArduinoHttpServer::StreamHttpErrorReply httpReply(client, httpRequest.getContentType(), "404");

                    LOG_ERROR(F("Requested page not found."));
                    LOG_ERROR(httpRequest.getResource().toString().c_str());

                    httpReply.send("Not Found");
                }
            }
            else
            {
                /* HTTP parsing failed. Client did not provide correct HTTP data or
                 * client requested an unsupported feature.
                 *
                 * Send a 400 back, which means "Bad Request".
                 */
                ArduinoHttpServer::StreamHttpErrorReply httpReply(client, httpRequest.getContentType(), "400");

                LOG_ERROR(F("HTTP parsing failed."));
                LOG_ERROR(httpRequest.getError().cStr());

                httpReply.send("Bad Request");
            }
        }
    }
}

/**
 * Handle GET root access.
 *
 * @param[in] client        Ethernet client, used to send the response.
 * @param[in] httpRequest   The http request itself.
 */
static void handleRoot(EthernetClient& client, const HttpRequest& httpRequest)
{
    ArduinoHttpServer::StreamHttpReply  httpReply(client, "text/html");
    String                              data;
    uint8_t                             s0SmartmeterIndex = 0;
    String                              tmp;

    data += reinterpret_cast<const __FlashStringHelper*>(HTML_PAGE_HEAD);
    data += F("<h1>AVR-NET-IO-Smartmeter</h1>\r\n");

    for(s0SmartmeterIndex = 0; s0SmartmeterIndex < CONFIG_S0_SMARTMETER_MAX_NUM; ++s0SmartmeterIndex)
    {
        S0Smartmeter& s0Smartmeter = gS0Smartmeters[s0SmartmeterIndex];

        if (false == s0Smartmeter.isEnabled())
        {
            data += F("<h2>Interface ");
            data += s0SmartmeterIndex;
            data += F(" </h2>\r\n");
            data += F("<p>Disabled</p>\r\n");
        }
        else
        {
            unsigned long powerConsumption  = 0;
            uint32_t      pulseCnt          = 0;
            unsigned long energyConsumption = 0;
            unsigned long durationLastReq   = 0;

            s0Smartmeter.getResult(powerConsumption, energyConsumption, pulseCnt, durationLastReq);

            data += F("<h2>Interface ");
            data += s0SmartmeterIndex;
            data += F(" - ");
            data += s0Smartmeter.getName();
            data += F("</h2>\r\n");
            data += F("<ul>\r\n");

            data += F("    <li>Power Consumption: ");
            data += powerConsumption;
            data += F(" W</li>\r\n");

            data += F("    <li>Pulses counted: ");
            data += pulseCnt;
            data += F("</li>\r\n");

            data += F("    <li>Energy Consumption: ");
            data += energyConsumption;
            data += F(" Ws</li>\r\n");

            data += F("</ul>\r\n");
        }
    }

    data += reinterpret_cast<const __FlashStringHelper*>(HTML_PAGE_TAIL);

    httpReply.send(data);

    return;
}

/**
 * Add JSON S0 smarmeter object to string.
 *
 * @param[in]       s0SmartmeterIndex   Index of the S0 smartmeter
 * @param[inout]    jsonData            JSON data object
 */
static void s0Smartmeter2JSON(S0Smartmeter& s0Smartmeter, JsonObject& jsonData)
{
    unsigned long powerConsumption  = 0;
    uint32_t      pulseCnt          = 0;
    unsigned long energyConsumption = 0;
    unsigned long durationLastReq   = 0;

    s0Smartmeter.getResult(powerConsumption, energyConsumption, pulseCnt, durationLastReq);

    jsonData["id"]                  = s0Smartmeter.getId();
    jsonData["name"]                = s0Smartmeter.getName();
    jsonData["powerConsumption"]    = powerConsumption;
    jsonData["pulses"]              = pulseCnt;
    jsonData["energyConsumption"]   = energyConsumption;

    return;
}

/**
 * Handle the route for the /api/s0-interface/? folder, which responds with the data
 * in JSON format.
 *
 * @param[in] client        Ethernet client, used to send the response.
 * @param[in] httpRequest   The http request itself.
 */
static void handleS0InterfaceReq(EthernetClient& client, const HttpRequest& httpRequest)
{
    ArduinoHttpServer::StreamHttpReply  httpReply(client, "application/json");
    String                              data;
    uint8_t                             s0SmartmeterIndex   = httpRequest.getResource()[2].toInt();
    DynamicJsonDocument                 jsonDoc(256);
    JsonObject                          jsonData            = jsonDoc.createNestedObject("data");

    if (CONFIG_S0_SMARTMETER_MAX_NUM <= s0SmartmeterIndex)
    {
        jsonDoc["status"] = STATUS_ID_EPAR;
    }
    else
    {
        S0Smartmeter& s0Smartmeter = gS0Smartmeters[s0SmartmeterIndex];

        if (true == s0Smartmeter.isEnabled())
        {
            s0Smartmeter2JSON(s0Smartmeter, jsonData);
        }

        jsonDoc["status"] = STATUS_ID_OK;
    }

    (void)serializeJson(jsonDoc, data);

    httpReply.send(data);

    return;
}

/**
 * Handle the route for the /api/s0-interfaces folder, which responds with the data
 * in JSON format.
 *
 * @param[in] client        Ethernet client, used to send the response.
 * @param[in] httpRequest   The http request itself.
 */
static void handleS0InterfacesReq(EthernetClient& client, const HttpRequest& httpRequest)
{
    ArduinoHttpServer::StreamHttpReply  httpReply(client, "application/json");
    String                              data;
    uint8_t                             s0SmartmeterIndex = 0;
    DynamicJsonDocument                 jsonDoc(256);
    JsonArray                           jsonDataArray     = jsonDoc.createNestedArray("data");

    for(s0SmartmeterIndex = 0; s0SmartmeterIndex < CONFIG_S0_SMARTMETER_MAX_NUM; ++s0SmartmeterIndex)
    {
        S0Smartmeter&   s0Smartmeter    = gS0Smartmeters[s0SmartmeterIndex];
        JsonObject      jsonData;

        if (true == s0Smartmeter.isEnabled())
        {
            s0Smartmeter2JSON(s0Smartmeter, jsonData);

            jsonDataArray.add(jsonData);
        }
    }

    jsonDoc["status"] = STATUS_ID_OK;

    (void)serializeJson(jsonDoc, data);

    httpReply.send(data);

    return;
}

/**
 * Handle the route for the /configure/? folder.
 *
 * @param[in] client        Ethernet client, used to send the response.
 * @param[in] httpRequest   The http request itself.
 */
static void handleConfigureGetReq(EthernetClient& client, const HttpRequest& httpRequest)
{
    ArduinoHttpServer::StreamHttpReply  httpReply(client, "text/html");
    String                              data;
    uint8_t                             s0SmartmeterIndex = httpRequest.getResource()[1].toInt();

    data += reinterpret_cast<const __FlashStringHelper*>(HTML_PAGE_HEAD);
    data += F("<h1>AVR-NET-IO-Smartmeter</h1>\r\n");
    data += F("<h2>Configuration</h2>\r\n");

    if (CONFIG_S0_SMARTMETER_MAX_NUM <= s0SmartmeterIndex)
    {
        data += F("<p>Invalid interface!</p>");
    }
    else
    {
        PersistentMemory::S0Data  s0Data;

        PersistentMemory::readS0Data(s0SmartmeterIndex, s0Data);

        /* Show interface id */
        data += F("<h3>Interface ");
        data += s0SmartmeterIndex;
        data += F("</h3>\r\n");

        data += F("<form action=\"#\"method=\"post\">\r\n");

        /* Interface enabled or disabled */
        data += F("Enabled: ");
        data += F("<select name=\"isEnabled\">");

        if (false == s0Data.isEnabled)
        {
            data += F("<option value=\"0\" selected>false</option>");
            data += F("<option value=\"1\">true</option>");
        }
        else
        {
            data += F("<option value=\"0\">false</option>");
            data += F("<option value=\"1\" selected>true</option>");
        }

        data += F("</select><br />\r\n");

        /* Interface user friendly name */
        data += F("Name: ");
        data += F("<input name=\"name\" type=\"text\" value=\"");
        data += s0Data.name;
        data += F("\"><br />\r\n");

        /* Arduino pin number, where the S0 is connected to */
        data += F("Arduino Pin: ");
        data += F("<input name=\"pinS0\" type=\"number\" min=\"");
        data += S0Pin::mcPinRangeMin;
        data += F("\" max=\"");
        data += S0Pin::mcPinRangeMax;
        data += F("\" value=\"");
        data += s0Data.pinS0;
        data += F("\"><br />\r\n");

        /* Number of pulses per kWh */
        data += F("Pulses per kWh: ");
        data += F("<input name=\"pulsesPerKWH\" type=\"number\" min=\"");
        data += S0Smartmeter::PULSES_PER_KWH_RANGE_MIN;
        data += F("\" max=\"");
        data += S0Smartmeter::PULSES_PER_KWH_RANGE_MAX;
        data += F("\" value=\"");
        data += s0Data.pulsesPerKWH;
        data += F("\"><br />\r\n");

        data += F("<input type=\"submit\" value=\"Update\">\r\n");

        data += F("</form>\r\n");
    }

    data += reinterpret_cast<const __FlashStringHelper*>(HTML_PAGE_TAIL);

    httpReply.send(data);

    return;
}

/**
 * Handle the route for the /configure/? folder.
 *
 * @param[in] client        Ethernet client, used to send the response.
 * @param[in] httpRequest   The http request itself.
 */
static void handleConfigurePostReq(EthernetClient& client, const HttpRequest& httpRequest)
{
    ArduinoHttpServer::StreamHttpReply  httpReply(client, "text/html");
    String                              data;
    uint8_t                             s0SmartmeterIndex = httpRequest.getResource()[1].toInt();
    const char*                         body              = httpRequest.getBody();
    char*                               tokStr            = strtok(const_cast<char*>(body), "=");
    const char*                         isEnabledStr      = PSTR("isEnabled");
    const char*                         nameStr           = PSTR("name");
    const char*                         pinS0Str          = PSTR("pinS0");
    const char*                         pulsesPer_kWhStr  = PSTR("pulsesPerKWH");
    PersistentMemory::S0Data            s0Data;
    bool                                isDirty           = false;
    long                                value             = 0;

    PersistentMemory::readS0Data(s0SmartmeterIndex, s0Data);

    /* Parameter are key:value pairs:
     * <key>=[<value>][&]
     */
    while(NULL != tokStr)
    {
        /* Count number of characters till the value of the current key.
         * This is necessary to check whether a value is available or not.
         */
        size_t cnt = strcspn(body, tokStr) + strlen(tokStr) + 1;

        LOG_DEBUG(tokStr);

        /* Interface enabled or not? */
        if (0 == strcmp_P(tokStr, isEnabledStr))
        {
            /* Value available? */
            if ('&' != body[cnt])
            {
                tokStr = strtok(NULL, "&");

                if (NULL != tokStr)
                {
                    bool  isEnabled = false;

                    value = atol(tokStr);

                    if (0 == value)
                    {
                        isEnabled = false;
                    }
                    else
                    {
                        isEnabled = true;
                    }

                    if (isEnabled != s0Data.isEnabled)
                    {
                        s0Data.isEnabled = value;

                        isDirty = true;
                    }
                }
            }
        }
        /* Interface name? */
        else if (0 == strcmp_P(tokStr, nameStr))
        {
            // Value not available?
            if ('&' == body[cnt])
            {
                if (0 != strlen(s0Data.name))
                {
                    s0Data.name[0] = '\0';

                    isDirty = true;
                }
            }
            else
            {
                tokStr = strtok(NULL, "&");

                if (NULL != tokStr)
                {
                    if (0 != strncmp(s0Data.name, tokStr, sizeof(s0Data.name) - 1))
                    {
                        strncpy(s0Data.name, tokStr, sizeof(s0Data.name) - 1);
                        s0Data.name[sizeof(s0Data.name) - 1] = '\0';

                        isDirty = true;
                    }
                }
            }
        }
        /* S0 pin? */
        else if (0 == strcmp_P(tokStr, pinS0Str))
        {
            // Value available?
            if ('&' != body[cnt])
            {
                tokStr = strtok(NULL, "&");

                if (NULL != tokStr)
                {
                    value = atol(tokStr);

                    if ((0 <= value) &&
                        (UINT8_MAX >= value))
                    {
                        uint8_t pinNo = static_cast<uint8_t>(value);

                        if ((pinNo != s0Data.pinS0) &&
                            (S0Pin::mcPinRangeMin <= pinNo) &&
                            (S0Pin::mcPinRangeMax >= pinNo))
                        {
                            s0Data.pinS0 = pinNo;

                            isDirty = true;
                        }
                    }
                }
            }
        }
        /* Pulses per kWh? */
        else if (0 == strcmp_P(tokStr, pulsesPer_kWhStr))
        {
            // Value available?
            if ('&' != body[cnt])
            {
                tokStr = strtok(NULL, "&");

                if (NULL != tokStr)
                {
                    value = atol(tokStr);

                    if (0 <= value)
                    {
                        uint32_t pulses = static_cast<uint32_t>(value);

                        if ((pulses != s0Data.pulsesPerKWH) &&
                            (S0Smartmeter::PULSES_PER_KWH_RANGE_MIN <= pulses) &&
                            (S0Smartmeter::PULSES_PER_KWH_RANGE_MAX >= pulses))
                        {
                            s0Data.pulsesPerKWH = pulses;

                            isDirty = true;
                        }
                    }
                }
            }
        }

        /* Next key:value pair */
        tokStr = strtok(NULL, "=");
    }

    data += reinterpret_cast<const __FlashStringHelper*>(HTML_PAGE_HEAD);

    /* Only write to persistency in case something was changed. */
    if (true == isDirty)
    {
        uint8_t                   index       = 0;
        PersistentMemory::S0Data  s0DataOther;
        bool                      isInvalid   = false;

        /* Verify that the new parameters are valid to all other
         * activated interfaces.
         */
        for(index = 0; (index < CONFIG_S0_SMARTMETER_MAX_NUM) && (false == isInvalid); ++index)
        {
            if (index != s0SmartmeterIndex)
            {
                PersistentMemory::readS0Data(s0SmartmeterIndex, s0DataOther);

                /* Enabled? */
                if (0 != s0DataOther.isEnabled)
                {
                    /* Pin already configured? */
                    if (s0DataOther.pinS0 == s0Data.pinS0)
                    {
                        isInvalid = true;
                    }
                }
            }
        }

        if (true == isInvalid)
        {
            LOG_INFO("Parameter not updated, because they are invalid.");

            data += F("Parameter not updated, because they are invalid.");
        }
        else
        {
            PersistentMemory::writeS0Data(s0SmartmeterIndex, s0Data);

            LOG_INFO("Parameter updated. Please reboot.");

            data += F("Parameter updated. Please reboot.");
        }
    }
    else
    {
        LOG_INFO("Parameter not updated.");

        data += F("Parameter not updated.");
    }

    data += reinterpret_cast<const __FlashStringHelper*>(HTML_PAGE_TAIL);

    httpReply.send(data);

    return;
}

/**
 * ISR of pin change interrupt 0.
 */
ISR(PCINT0_vect)
{
    static uint8_t  lastValue = 0xff;
    uint8_t         value     = PINA;
    uint8_t         index     = 0;
    uint8_t         bitNo     = 0;

    /* Which pin triggered? */
    for(index = 0; index < CONFIG_S0_SMARTMETER_MAX_NUM; ++index)
    {
        if (true == gS0Smartmeters[index].isEnabled())
        {
            bitNo = gS0Smartmeters[index].getS0Pin().getPortBitNo();

            /* Falling edge? */
            if ((0 != (lastValue & _BV(bitNo))) &&
                (0 == (value & _BV(bitNo))))
            {
                gS0Smartmeters[index].internalISR();
            }
        }
    }

    lastValue = value;

    return;
}
