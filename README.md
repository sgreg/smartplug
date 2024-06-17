# C&#x0413;ApL&#x2200;b SmartPlug

The C&#x0413;ApL&#x2200;b SmartPlug is a "smart" and utterly useless IoT power
plug that tells you whether you plugged it in the socket or not. A companion
Android app will inform you nearly real-time on any plugging changes, and of
course, *shit's tweetin' too, yo*.

### Components

The SmartPlug is built around some [ESP8266 based hardware](hardware/) with
a [C SDK firmware](firmware/).

Optionally, a [simple backend](backend/) written in Python can be utilized
for tweeting and sending status changes via WebSockets to the [Android companion
app](PlugBuddy/). Alternatively, the firmware can be set up to use
[ThingSpeak](https://thingspeak.com/) for tweeting instead.

### Links

[Blog article](http://sgreg.craplab.fi/blog/article/creating-an-utterly-useless-smart-power-plug)  
[Hackaday.io](https://hackaday.io/project/20736-the-utterly-useless-smartplug)

### License

Everything is released under the MIT License.

