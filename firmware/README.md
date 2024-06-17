## C&#x0413;ApL&#x2200;b SmartPlug firmware

The SmartPlug firmware is running on an ESP8266 and is implemented in C using
the ESP8266 non-OS SDK. This is initially based on my
[ESP8266 C SDK template](https://github.com/sgreg/esp8266-c-sdk-template)
which I wrote about in
[this blog article](http://sgreg.craplab.fi/blog/article/simple-esp8266-c-sdk-project-template),
so that's a good starting point to set up a build environment ..if you're
actually like thiking of building this yourself.

Well, let's say you do.

### Configure it

First things first, you have to decide if you want to use the
[simple server backend](../backend) to use the
[PlugBuddy companion app](../PlugBuddy) and optionally
tweet about the SmartPlug's life, or rather use the
[ThingSpeak](https://thingspeak.com/)
API and tweet only.

Set the `USE_THINGSPEAK` flag in [`user_config.h`](user/user_config.h)
accordingly. If you set it to `1`, you'll need a Twitter token from ThingSpeak
and set the `twitter_api_key` variable accordingly. The remote server is
already set up in this case.

However, if you decide to self-host the [backend](../backend/) instead, i.e.
set the flag to `0`, adjust the `backend.tcp->remote_port` and
`backend.tcp->remote_host` accordingly.


Then you'll obviously have to set up the WiFi network by configuring the
`wifi_ssid` and `wifi_password` variables in
[`user_config.h`](user/user_config.h).

If you're using ThingSpeak and want to adjust the tweets, have a look at the
`wif_send_thingspeak()` function in [`main.c`](user/main.c)


### Build and flash it

Building and writing the firmware is just simple ESP8266 C SDK stuff.
Again, check
[my blog article on the ESP8266 C SDK](http://sgreg.craplab.fi/blog/article/simple-esp8266-c-sdk-project-template)
as starting point for that.

