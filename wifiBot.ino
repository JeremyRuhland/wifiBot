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

#define SSID "MYROBOT"
#define PASSWORD "SecretPassword"
#define CHAN 11

// Copied from 404.html
const uint8_t PROGMEM htmlPage404[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><title>wifiBot</title><style type=\"text/css\">body{background-color: #CD5C5C;}div.r{position: relative; left: 50%; top: 50%; transform: translate(-50%, -50%); border-radius: 75px; background: #FFFFFF; opacity: 0.6; width: 500px; height: 500px;}div.r p{position: relative; left: 85%; top: 50%; transform: translate(-50%, -50%); color: #000000; font-size: 400px; font-weight: bold; font-family: serif;}</style><body> <div class=\"r\"> <p>!</p></div></body></html>";

// Copied from index.html
const uint8_t PROGMEM htmlPageIndex[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>Hello World!</body></html>";

// Function prototypes
void sendIndex(uint8_t);
void send404(uint8_t);

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
    
    // Set up motors
    pinMode(2, OUTPUT); // Left motor A
    pinMode(3, OUTPUT); // Left motor B
    pinMode(4, OUTPUT); // Right motor A
    pinMode(5, OUTPUT); // Right motor B
    
    pinMode(A5, OUTPUT); // LED lights
    pinMode(13, OUTPUT); // Status LED
   
    // Set up wifi AP in WPA/WPA2 PSK mode
    wifiStatus = wifi.setOprToStationSoftAP();
    if (!wifiStatus) {
        wifiErrors++;
    } else {}
    
    wifiStatus = wifi.setSoftAPParam(SSID, PASSWORD, CHAN, 4);
    if (!wifiStatus) {
        wifiErrors++;
    } else {}
    delay(2000);
    
    wifiStatus = wifi.enableMUX(); // Mux mode
    if (!wifiStatus) {
        wifiErrors++;
    } else {}
    delay(100);
    
    wifiStatus = wifi.startTCPServer(80); // Start server on port 80
    if (!wifiStatus) {
        wifiErrors++;
    } else {}
    
    wifiStatus = wifi.setTCPServerTimeout(10);
    if (!wifiStatus) {
        wifiErrors++;
    } else {}
    
    if (wifiErrors > 0) {
        for (;;) {} // Trap forever, error state
    } else {
        // Blink lights to signal successful startup
        robotLightsOn();
        delay(2000);
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
                    break;
                case '2':
                    // Backwards
                    leftMotorBackward();
                    rightMotorBackward();
                    break;
                case '3':
                    // Right turn
                    leftMotorForward();
                    rightMotorBackward();
                    break;
                case '4':
                    // Left turn
                    leftMotorBackward();
                    rightMotorForward();
                    break;
                case '5':
                    // Stop
                    allMotorsStop();
                    break;
                case '6':
                    // Lights on
                    robotLightsOn();
                    break;
                case '7':
                    // Lights off
                    robotLightsOff();
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
    digitalWrite(2, LOW);
    digitalWrite(3, LOW);
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
}

/**
 * leftMotorForward
 *
 * Turns left motor forwards
 */
void leftMotorForward(void) {
    // Turn off first to prevent shorting
    digitalWrite(3, LOW);
    digitalWrite(2, HIGH);
}

/**
 * leftMotorBackward
 *
 * Turns left motor backwards
 */
void leftMotorBackward(void) {
    // Turn off first to prevent shorting
    digitalWrite(2, LOW);
    digitalWrite(3, HIGH);
}

/**
 * rightMotorForward
 *
 * Turns right motor forwards
 */
void rightMotorForward(void) {
    // Turn off first to prevent shorting
    digitalWrite(5, LOW);
    digitalWrite(4, HIGH);
}

/**
 * rightMotorBackward
 *
 * Turns right motor backwards
 */
void rightMotorBackward(void) {
    // Turn off first to prevent shorting
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
}

/**
 * robotLightsOn
 *
 * Turn on external switched load
 */
void robotLightsOn(void) {
    digitalWrite(A5, HIGH);
}

/**
 * robotLightsOff
 *
 * Turn off external switched load
 */
void robotLightsOff(void) {
    digitalWrite(A5, LOW);
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
