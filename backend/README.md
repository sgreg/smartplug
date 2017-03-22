## C&#x0413;ApL&#x2200;b SmartPlug's simple backend server

The SmartPlug supports two options to communicate its states to the world:

1. [ThingSpeak](https://thingspeak.com/)'s Twitter API
1. This simple backend server

The latter one obviously needs to be self hosted, while the first one is an
IoT service. The main reason [ThingSpeak](https://thingspeak.com/) is
supported in the first place was initial laziness, but eventually the
[companion app](../PlugBuddy/) idea was born, for which a backend was needed
anyway.

### Features

The backend provides three main features:

1. WWW server to receive status messages from the SmartPlug
1. Twitter API to tell the world the fate of your SmartPlug
1. WebSocket server to notify yourself via the [companion app](../PlugBuddy)

The WWW server is a very basic [Bottle](https://bottlepy.org/) application
without much RESTability, just one simple `POST` is all it listens to.

The Twitter API is implemented using [Mike Taylor's](https://github.com/bear/)
[Python Twitter API wrapper](https://github.com/bear/python-twitter). To use
this, you have to set up an application in your Twitter account to get all the
tokens the API needs. This is all nicely documented in the previous link.

Lastly, the WebSocket server is implemented using
[Autobahn](http://autobahn.ws/python/)

Note, there is no persistent storage. Nothing gets stored in a database but
is kept in memory only. Meaning, if the backend process is killed, all
information is lost. Once restarted, the plug state is unknonwn until an
actual event update is received from the SmartPlug.


### Configuration

The main configuration is to set the two server ports for the HTTP REST API and
the WebSocket respectively. Set the `WWW_SERVER_PORT` and `WS_SERVER_PORT`
variables accordingly.

#### Twitter
Tweeting is optionally. If the `TWITTER_ENABLED` variable is set to `False`,
everything Twitter related will be skipped. If set to `True` however, make sure
to properly set up all token variables below it. Once again, check the
[Twitter API's Github documentation](https://github.com/bear/python-twitter)
for guidance on this.

