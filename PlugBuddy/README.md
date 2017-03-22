## PlugBuddy - the SmartPlug companion app

PlugBuddy is a simple status display and SmartPlug notification companion
app for Android. Connected to the [SmartPlug backend](../backend/) via matching
[Autobahn WebSocket API for Android](http://autobahn.ws/android/), it receives
real-time information about the plug's state and displays it.
That's all it really does.

The app itself displays a big power button icon representing the SmartPlug
state by color, with

* grey: plug is unplugged
* green: plug is plugged in
* red: communication error / no information

Apart from that, there's a settings dialog to enter the backend's WebSocket
host and port, and the SmartPlug's device id.

