/**
 * @file wifiBot.c
 * @brief A simple robot controlled by a smartphone & an Arachnio
 *
 * Arachnio will become a password protected wifi access point with a
 * self-hosted http server allowing remote control of the robot.
 *
 * Uses ESP8266 library from http://github.com/JeremyRuhland/ITEADLIB_Arduino_WeeESP8266
 *
 * @author Jeremy Ruhland <jeremy ( a t ) goopypanther.org>
 */

#include <avr/pgmspace.h>
#include "ESP8266.h"

#define SSID     "MYROBOT"
#define PASSWORD "SecretPassword"
#define CHAN     11

#define LED13_ON()  digitalWrite(13, HIGH)
#define LED13_OFF() digitalWrite(13, LOW)

// Copied from 404_min.html
const uint8_t PROGMEM htmlPage404[] = "<html><title>wifiBot</title><style type=\"text/css\">body{background-color: #CD5C5C; -webkit-touch-callout:none; -webkit-user-select:none; -khtml-user-select:none; -moz-user-select:none; -ms-user-select:none; user-select:none; -webkit-tap-highlight-color:rgba(0,0,0,0);}div.r{position: relative; left: 50%; top: 50%; transform: translate(-50%, -50%); border-radius: 75px; background: #FFFFFF; opacity: 0.6; width: 500px; height: 500px;}div.r p{position: relative; left: 85%; top: 50%; transform: translate(-50%, -50%); color: #000000; font-size: 400px; font-weight: bold; font-family: serif;}</style><body> <div class=\"r\"> <p>!</p></div></body></html>";

// Copied from index_min.html
const uint8_t PROGMEM htmlPageIndex[] = "<html><meta content='width=device-width, initial-scale=0.5, maximum-scale=0.5, user-scalable=0' name='viewport'/><title>wifiBot</title><style type=\"text/css\">td{display: inline-block;width: 200px;height: 200px;background-color: #0005CA;margin: 1em;border-radius: 75px;text-align: center; vertical-align: middle; line-height: 200px;}.button:active{background-color: #00C96E;}body{-webkit-touch-callout:none;-webkit-user-select:none;-khtml-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;-webkit-tap-highlight-color:rgba(0,0,0,0);background-color: #0071C0;}a{color: white;}table{position: relative; left: 50%; top: 5%; transform: translate(-50%, 25%); color: white; font-weight: bold; font-family: serif;}</style><script>d=document.createElement(\"img\");function fwd(){d.src=\"1\"}function bak(){d.src=\"2\"}function lft(){d.src=\"4\"}function rht(){d.src=\"3\"}function stp(){d.src=\"5\"}function on(){d.src=\"6\"}function off(){d.src=\"7\"}</script><body><table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr><td><a href=\"http://github.com/JeremyRuhland/wifiBot\">wifiBot</a></td><td class=\"button\" ontouchstart=\"fwd()\" ontouchend=\"stp()\">^</td><td><a href=\"http://www.kickstarter.com/projects/logos-electro/arachnio\">Arachnio</a></td></tr><tr><td class=\"button\" ontouchstart=\"lft()\" ontouchend=\"stp()\"><</td><td class=\"button\" ontouchstart=\"stp()\">Stop</td><td class=\"button\" ontouchstart=\"rht()\" ontouchend=\"stp()\">></td></tr><tr><td class=\"button\" ontouchstart=\"off()\">Lights Off</td><td class=\"button\" ontouchstart=\"bak()\" ontouchend=\"stp()\">v</td><td class=\"button\" ontouchstart=\"on()\">Lights On</td></tr></table></body></html>";

const uint8_t PROGMEM htmlPageNull[] = "<html>null</html>";

// Function prototypes
void sendIndex(uint8_t);
void send404(uint8_t);
void sendNull(uint8_t);

void allMotorsStop(void);
void leftMotorForward(void);
void leftMotorBackward(void);
void rightMotorForward(void);
void rightMotorBackward(void);
void robotLightsOn(void);
void robotLightsOff(void);

ESP8266 wifi(Serial1);

void setup() {
    bool wifiStatus;
    uint8_t wifiErrors = 0;

    delay(2000);

    // Set up motors
    pinMode(9, OUTPUT);  // Left motor A
    pinMode(10, OUTPUT); // Left motor B
    pinMode(11, OUTPUT); // Right motor A
    pinMode(12, OUTPUT); // Right motor B

    allMotorsStop();

    pinMode(A4, OUTPUT); // LED lights
    pinMode(13, OUTPUT); // Status LED

    // Set up wifi AP in WPA/WPA2 PSK mode
    wifiStatus = wifi.setOprToStationSoftAP();
    if (!wifiStatus) {
        wifiErrors++;
    } else {}

    wifiStatus = wifi.setSoftAPParam(SSID, PASSWORD, CHAN, 3, 3);
    if (!wifiStatus) {
        wifiErrors++;
    } else {}

    wifiStatus = wifi.enableMUX(); // Mux mode
    if (!wifiStatus) {
        wifiErrors++;
    } else {}

    wifiStatus = wifi.startTCPServer(80); // Start server on port 80
    if (!wifiStatus) {
        wifiErrors++;
    } else {}

    wifiStatus = wifi.setTCPServerTimeout(5);
    if (!wifiStatus) {
        wifiErrors++;
    } else {}

    if (wifiErrors > 0) {
        for (;;) {} // Trap forever, error state
    } else {
        // Blink lights to signal successful startup
        robotLightsOn();
        delay(500);
        robotLightsOff();
        delay(500);
        robotLightsOn();
        delay(500);
        robotLightsOff();
    }
}

void loop() {
    uint8_t buffer[32];
    uint8_t muxId;
    uint32_t len;
    uint8_t temp;

    // Receive data from wifi
    len = wifi.recv(&muxId, buffer, sizeof(buffer), 100);


    if (len > 0) {
        // We received something

        temp = strncmp((const char*) buffer, "GET /", 5);

        if (temp == 0) {
            switch (buffer[5]) {
            case ' ':
                // Index page
                sendIndex(muxId);
                break;
            case '1':
                // Forwards
                leftMotorForward();
                rightMotorForward();
                sendNull(muxId);
                break;
            case '2':
                // Backwards
                leftMotorBackward();
                rightMotorBackward();
                sendNull(muxId);
                break;
            case '3':
                // Right turn
                leftMotorForward();
                rightMotorBackward();
                sendNull(muxId);
                break;
            case '4':
                // Left turn
                leftMotorBackward();
                rightMotorForward();
                sendNull(muxId);
                break;
            case '5':
                // Stop
                allMotorsStop();
                sendNull(muxId);
                break;
            case '6':
                // Lights on
                robotLightsOn();
                LED13_ON();
                sendNull(muxId);
                break;
            case '7':
                // Lights off
                robotLightsOff();
                LED13_OFF();
                sendNull(muxId);
                break;
            default :
                allMotorsStop();
                send404(muxId);
                break;
            }

        wifi.releaseTCP(muxId); // Release TCP connection
        } else {}
    } else {}
}

/**
 * allMotorsStop
 *
 * Stops all motors
 */
void allMotorsStop(void) {
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
}

/**
 * leftMotorForward
 *
 * Turns left motor forwards
 */
void leftMotorForward(void) {
    // Turn off first to prevent shorting
    digitalWrite(9, LOW);
    digitalWrite(10, HIGH);
}

/**
 * leftMotorBackward
 *
 * Turns left motor backwards
 */
void leftMotorBackward(void) {
    // Turn off first to prevent shorting
    digitalWrite(10, LOW);
    digitalWrite(9, HIGH);
}

/**
 * rightMotorForward
 *
 * Turns right motor forwards
 */
void rightMotorForward(void) {
    // Turn off first to prevent shorting
    digitalWrite(12, LOW);
    digitalWrite(11, HIGH);
}

/**
 * rightMotorBackward
 *
 * Turns right motor backwards
 */
void rightMotorBackward(void) {
    // Turn off first to prevent shorting
    digitalWrite(11, LOW);
    digitalWrite(12, HIGH);
}

/**
 * robotLightsOn
 *
 * Turn on external switched load
 */
void robotLightsOn(void) {
    digitalWrite(A4, HIGH);
    LED13_ON();
}

/**
 * robotLightsOff
 *
 * Turn off external switched load
 */
void robotLightsOff(void) {
    digitalWrite(A4, LOW);
    LED13_OFF();
}

/**
 * sendIndex
 *
 * @param muxId ID of mux channel to send reply to
 *
 * Send index page to esp8266
 */
void sendIndex(uint8_t muxId) {
    wifi.sendFromFlash(muxId, htmlPageIndex, sizeof(htmlPageIndex));
}

/**
 * send404
 *
 * @param muxId ID of mux channel to send reply to
 *
 * Send 404 page to esp8266
 */
void send404(uint8_t muxId) {
    wifi.sendFromFlash(muxId, htmlPage404, sizeof(htmlPage404));
}

/**
 * sendNull
 *
 * @param muxId ID of mux channel to send reply to
 *
 * Sends null page to browser when executing control functions
 */
void sendNull(uint8_t muxId) {
    wifi.sendFromFlash(muxId, htmlPageNull, sizeof(htmlPageNull));
}
