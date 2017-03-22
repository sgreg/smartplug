/*
 * CrapLab SmartPlug firmware
 *
 * Copyright (c) 2017 Sven Gregori <sven@craplab.fi>
 *
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

// set to 1 to enable debug UART output, 0 to disable
#define DEBUG 1
// set to 1 to use thingspeak.com as backend
#define USE_THINGSPEAK 0


// WiFi configuration
const char wifi_ssid[32] = "";
const char wifi_password[32] = "";

#if (USE_THINGSPEAK == 1)
// Thingspeak.com configuration
const char twitter_api_key[] = "";
#endif // USE_THINGSPEAK

#endif // _USER_CONFIG_H_
