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
#include "osapi.h"
#include "os_type.h"
#include "ets_sys.h"
#include "gpio.h"
#include "user_config.h"
#include "ip_addr.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"


#if(DEBUG > 0)
#define DBG_PRINTF()
#define dbg_printf(fmt, ...) do { os_printf(fmt, __VA_ARGS__); } while (0)
#else
#define dbg_printf(fmt, ...)
#endif

#define PIN_POWER_STATE PERIPHS_IO_MUX_GPIO0_U
#define PIN_FUNC_POWER_STATE FUNC_GPIO0
#define GPIO_POWER_STATE 0

#define PIN_SUICIDE PERIPHS_IO_MUX_GPIO2_U
#define PIN_FUNC_SUICIDE FUNC_GPIO2
#define GPIO_SUICIDE 2


struct espconn *backend;
uint8_t connected;

uint8_t state;
uint8_t last_state;

os_timer_t debounce_timer;
uint8_t debounce_timer_armed;
void debounce_timer_cback(void *);

void gpio_cback(void *);

os_timer_t suicide_timer;
void commit_suicide(void);
void suicide_timer_cback(void *);

void wifi_init();
void wifi_event_cb(System_Event_t *);
void remote_connect_cb(void *);
void wifi_send_state(int);
void wifi_send_remote(struct espconn *);
#if (USE_THINGSPEAK == 1)
void wifi_send_thingspeak(struct espconn *);
#else
void wifi_send_backend(struct espconn *);
#endif


/**
 * Main entry
 */
void ICACHE_FLASH_ATTR
user_init() {
    // setup UART
    uart_div_modify(0, UART_CLK_FREQ / 9600);

    os_printf("\n\nCrapLab Smartplug\ncraplab.fi\n\n");

    // setup GPIO
    gpio_init();
    last_state = GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_POWER_STATE));

    // setup power state GPIO debounce timer
    os_timer_disarm(&debounce_timer);
    os_timer_setfn(&debounce_timer, debounce_timer_cback, NULL);
    debounce_timer_armed = 0;

    // setup suicide timer in case remote isn't responding
    os_timer_disarm(&suicide_timer);
    os_timer_setfn(&suicide_timer, suicide_timer_cback, NULL);

    // setup WiFi
    wifi_init();

    // setup connection structure
    backend = (struct espconn *) os_zalloc(sizeof(struct espconn));
    backend->proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));
    backend->type = ESPCONN_TCP;
    backend->state = ESPCONN_NONE;


#if (USE_THINGSPEAK == 1)
    backend->proto.tcp->remote_port  =  80;
    backend->proto.tcp->remote_ip[0] = 184;
    backend->proto.tcp->remote_ip[1] = 106;
    backend->proto.tcp->remote_ip[2] = 153;
    backend->proto.tcp->remote_ip[3] = 149;
#else
    backend->proto.tcp->remote_port  = 9000;
    backend->proto.tcp->remote_ip[0] =  192;
    backend->proto.tcp->remote_ip[1] =  168;
    backend->proto.tcp->remote_ip[2] =    1;
    backend->proto.tcp->remote_ip[3] =   64;
#endif

    espconn_regist_connectcb(backend, remote_connect_cb);
}



/**
 * Initialize GPIO.
 *
 * Setup the Power state input pin and its interrupt,
 * as well as the suicide pin output.
 */
void ICACHE_FLASH_ATTR
gpio_init(void) {
    // disable GPIO interrupts
    ETS_GPIO_INTR_DISABLE();

    // set up power connector state as input
    PIN_FUNC_SELECT(PIN_POWER_STATE, PIN_FUNC_POWER_STATE); // GPIO function
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(GPIO_POWER_STATE)); // set GPIO to input
    ETS_GPIO_INTR_ATTACH(gpio_cback, NULL); // attach interrupt handler
    gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_POWER_STATE), GPIO_PIN_INTR_ANYEDGE);

    // set up suicide GPIO as output
    PIN_FUNC_SELECT(PIN_SUICIDE, PIN_FUNC_SUICIDE);
    GPIO_OUTPUT_SET(GPIO_SUICIDE, 1);

    // enable GPIO interrupts
    ETS_GPIO_INTR_ENABLE();
}

/**
 * GPIO input interrupt handler.
 * Sets up the debounce timer that handles the actual input state changes.
 *
 * @param args callback data - unused here.
 *
 */
void ICACHE_FLASH_ATTR
gpio_cback(void *args) {
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    dbg_printf("gpio status 0x%x\n", gpio_status);

    if (gpio_status & BIT(GPIO_POWER_STATE)) {
        if (debounce_timer_armed) {
            os_timer_disarm(&debounce_timer);
        }
        debounce_timer_armed = 1;
        os_timer_arm(&debounce_timer, 200, false);
    }

    // clear interrupt states
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}

/**
 * Debounce timer handler.
 *
 * If the previous power state input value is still the same than in the
 * last timer handler, it's safe to assume the pin state it stable and the
 * actual state is read and handled.
 *
 * @param args callback data - unused here.
 * 
 */
void ICACHE_FLASH_ATTR
debounce_timer_cback(void *args) {
    debounce_timer_armed = 0;
    state = GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_POWER_STATE));

    os_printf("\npower plug state %d -> %d\n", last_state, state);
    if (state != last_state) {
        wifi_send_state(state);
    }
    last_state = state;
}


/**
 * Suicide timer handler.
 * Make sure everything is powered down after the plug was unplugged.
 *
 * @param args callback data - unused here.
 *
 */
void ICACHE_FLASH_ATTR
suicide_timer_cback(void *args) {
    commit_suicide();
}


/**
 * Pull the suicide pin low, resulting in shutting off the MOSFET and
 * ultimately cutting off its own power supply. So Sad.
 */
void ICACHE_FLASH_ATTR
commit_suicide(void) {
    os_printf("Goodbye, cruel world.\r\n");
    GPIO_OUTPUT_SET(GPIO_SUICIDE, 0);
}



/**
 * Initialize WiFi settings.
 * Adjust SSID and password in user_config.h
 */
void ICACHE_FLASH_ATTR
wifi_init(void) {
    struct station_config station_conf;
    os_printf("Setting up WiFi %s\r\n", wifi_ssid);
    wifi_set_opmode(STATION_MODE);
    os_memcpy(&station_conf.ssid, wifi_ssid, sizeof(wifi_ssid));
    os_memcpy(&station_conf.password, wifi_password, sizeof(wifi_password));
    wifi_station_set_config(&station_conf);
    wifi_station_connect();
    wifi_set_event_handler_cb(wifi_event_cb);
}


/**
 * WiFi event handler.
 * This is called during internal WiFi setup.
 *
 * @param event Event to be handled
 *
 */
void ICACHE_FLASH_ATTR
wifi_event_cb(System_Event_t * event)
{
    switch (event->event) {
        case EVENT_STAMODE_CONNECTED:
            os_printf("\nconnected to %s\n", wifi_ssid);
            break;

        case EVENT_STAMODE_GOT_IP:
            os_printf("received IP address: " IPSTR "\r\n", IP2STR(&event->event_info.got_ip.ip));
            wifi_send_state(1); // DHCP IP received, so just got plugged in.
            break;
    }
}


/**
 * Callback after data was sent to the remote server.
 * Disconnect in this case.
 *
 * @param args callback data, pointer to the remote server's espconn struct
 *
 */
void ICACHE_FLASH_ATTR
remote_sent_cb(void *arg)
{
    struct espconn *server = (struct espconn *) arg;
    os_printf("sent to server " IPSTR "\r\n", IP2STR(server->proto.tcp->remote_ip));
    espconn_disconnect(server);
}

/**
 * Callback after connection to the remote server was disconnected.
 * If the last sent state was 0, i.e. "unplugged", commit suicide.
 *
 * @param args callback data, pointer to the remote server's espconn struct
 *
 */
void ICACHE_FLASH_ATTR
remote_disconnect_cb(void *arg)
{
    struct espconn *server = (struct espconn *) arg;
    os_printf("disconnected from server " IPSTR "\r\n", IP2STR(server->proto.tcp->remote_ip));
    connected = 0;

    if (last_state == 0) {
        commit_suicide();
    }
}

/**
 * Callback after connection to the remote server was established.
 * Set up sent and disconnect callbacks (not receiving anything, so no need
 * for a receive callback here) and send the current state to it.
 *
 * @param args callback data, pointer to the remote server's espconn struct
 *
 */
void ICACHE_FLASH_ATTR
remote_connect_cb(void *arg)
{
    struct espconn *server = (struct espconn *) arg;
    os_printf("connected to server " IPSTR "\r\n", IP2STR(server->proto.tcp->remote_ip));

    espconn_regist_sentcb(server, remote_sent_cb);
    espconn_regist_disconcb(server, remote_disconnect_cb);
    connected = 1;

    wifi_send_remote(server);
}

/**
 * Send the plug state to the remote server, which is either ThingSpeak
 * or the Python backend, depending on USE_THINGSPEAK setting in user_config.h
 *
 * This either sends the state directly if already connected to the remote
 * server (which is unlikely), or then sets up the connection itself, and the
 * on-connect callback will take care of it.
 *
 * @param new_state The new plug state to be sent.
 *
 */
void ICACHE_FLASH_ATTR
wifi_send_state(int new_state) {
    if (new_state == 0) {
        /*
         * In case remote doesn't answer, the sent callback is never executed
         * and the plug won't commit suicide. Set a timer that forces shutdown
         * after 15 seconds for that case.
         */
        os_timer_arm(&suicide_timer, 15000, false);
    }

    if (!connected) {
        espconn_connect(backend);
    } else {
        wifi_send_remote(backend);
    }
}

/**
 * Send the current plug state to the remote server.
 * Depending on the USE_THINGSPEAK setting in user_config.h, this is either
 * the ThingSpeak server or the Python backend server.
 *
 * @param server espconn struct to the active server
 *
 */
void ICACHE_FLASH_ATTR
wifi_send_remote(struct espconn *server)
{
    os_printf("\nsending status\n\n");

#if (USE_THINGSPEAK == 1)
    wifi_send_thingspeak(server);
#else
    wifi_send_backend(server);
#endif
}



#if (USE_THINGSPEAK == 1)
/**
 * Send the plug state to the ThingSpeak server.
 *
 * @param server espconn struct to the ThingSpeak server
 *
 */
void ICACHE_FLASH_ATTR
wifi_send_thingspeak(struct espconn *server) {
    char *sendme  = (char *) os_zalloc(450);
    char *payload = (char *) os_zalloc(250);

    os_sprintf(payload, "api_key=%s&status=I%%20just%%20%s%%smy%%20#SmartPlug.%%20%%23smartpluglife", twitter_api_key, ((last_state) ? "plugged%%20in" : "unplugged"));

    os_sprintf(sendme,
            "POST /apps/thingtweet/1/statuses/update HTTP/1.1\n"
            "Host: api.thingspeak.com\n"
            "Connection: close\n"
            "Content-Type: application/x-www-form-urlencoded\n"
            "Content-Length: %d\n\n%s", strlen(payload), payload);

    espconn_send(server, sendme, strlen(sendme));
}

#else

/**
 * Send the plug state to the Python backend server.
 *
 * @param server espconn struct to the Python backend server
 *
 */
void ICACHE_FLASH_ATTR
wifi_send_backend(struct espconn *server) {
    char *sendme  = (char *) os_zalloc(200);
    char *payload = (char *) os_zalloc(50);

    os_sprintf(payload, "{\"id\": \"0xb00bf0cc\", \"value\": %d}", last_state);

    os_sprintf(sendme,
            "POST /data HTTP/1.1\n"
            "Host: craplab.local\n"
            "Connection: close\n"
            "Content-Type: application/json\n"
            "Content-Length: %d\n\n%s", strlen(payload), payload);

    espconn_send(server, sendme, strlen(sendme));
}

#endif

