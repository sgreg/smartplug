#!/usr/bin/python3
#
# CrapLab SmartPlug simple backend server
# 
# Copyright (c) 2017 Sven Gregori <sven@craplab.fi>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import time
from datetime import datetime

import asyncio
from autobahn.asyncio.websocket import WebSocketServerProtocol, WebSocketServerFactory

import json
import bottle
from threading import Thread

import twitter


# Backend configuration
#
# Set the port for the HTTP REST API
WWW_SERVER_PORT = 9000
#
# Set the port for the WebSocket API
WS_SERVER_PORT  = 9001


# Twitter configuration
#
# Set flag to True to use the Twitter API, otherwise set to False
TWITTER_ENABLED = False
#
# Twitter token setup.
# See https://github.com/bear/python-twitter for instructions
#
CONSUMER_KEY=''
CONSUMER_SECRET=''
ACCESS_TOKEN_KEY=''
ACCESS_TOKEN_SECRET=''


class Smartplug:
    def __init__(self, _id):
        self.id = _id
        self.value = 0

    def __str__(self):
        return "Smartplug id {} with value {}".format(self.id, self.value)

    def toJson(self):
        return json.dumps({'id': self.id, 'value': self.value})


class ServerProtocol(WebSocketServerProtocol):
   
    def onConnect(self, request):
        print("Websocket connection from " + request.peer)

    def onOpen(self):
        self.factory.register(self)

    def onMessage(self, payload, isBinary):
        payloadString = payload.decode('utf-8')
        print("Received message from {}: '{}'".format(self.peer, payloadString))
        data = json.loads(payloadString)
        if 'status' in data.keys():
            print("Received status request for {} from {}".format(data['status'], self.peer))
            if data['status'] in self.factory.smartplugs.keys():
                status = self.factory.smartplugs[data['status']].toJson()
                print("Sending status: " + status)
                self.sendMessage(status.encode('utf-8'))
            else:
                print("No such smartplug id")

    def onClose(self, wasClean, code, reason):
        print("Websocket connection closed: " + str(reason))
        WebSocketServerProtocol.onClose(self, wasClean, code, reason)
        self.factory.unregister(self)


class ServerFactory(WebSocketServerFactory):

    def __init__(self, listenUrl):
        WebSocketServerFactory.__init__(self, listenUrl)

        # List of connected websocket clients
        self.clients = []
        # List of received smartplug data
        self.smartplugs = {}

    def register(self, client):
        if client not in self.clients: 
            print("registered client {}".format(client.peer))
            self.clients.append(client)

            for c in self.clients:
                print(c.peer)

    def unregister(self, client):
        if client in self.clients:
            print("unregistered client {}".format(client.peer))
            self.clients.remove(client)

    def sendClients(self, msg):
        print("Sending to all clients: '{}'".format(msg))
        for c in self.clients:
            c.sendMessage(msg.encode('utf-8'))


class Backend(bottle.Bottle):

    def __init__(self, useTwitter=False):
        super(Backend, self).__init__()
        self.route('/', callback=self.index)
        self.route('/data', method='POST', callback=self.postData)
        self.factory = None
        self.twitter = None

        if useTwitter:
            print("Setting up Twitter API")

            self.twitter = twitter.Api(consumer_key=CONSUMER_KEY,
                    consumer_secret=CONSUMER_SECRET,
                    access_token_key=ACCESS_TOKEN_KEY,
                    access_token_secret=ACCESS_TOKEN_SECRET)

            try:
                self.twitterUser = self.twitter.VerifyCredentials()
            except twitter.error.TwitterError as e:
                print("Error creating API connection: " + str(e))
                self.twitter = None

        else:
            print("Twitter API is disabled")



    def index(self):
        return "hallo."

    def postData(self):
        data = bottle.request.json
        if data['id'] not in self.factory.smartplugs.keys():
            self.factory.smartplugs[data['id']] = Smartplug(data['id'])

        smartplug = self.factory.smartplugs[data['id']]
        smartplug.value = data['value']


        if self.twitter is not None:
            if smartplug.value == 1:
                tweet = 'Look at that, my #craplab #smartplug just got plugged in!'
            else:
                tweet = 'It was fun while it lasted. I unplugged my #craplab #smartplug.'

            print("Tweeting..")
            self.twitter.PostUpdate(tweet)

        else:
            print("Skipping tweeting")

        print("Updated: " + str(smartplug))

        self.factory.sendClients(json.dumps(data))
        return {}


    def websocketsLoop(self, port):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        
        self.factory = ServerFactory("ws://0.0.0.0:" + str(port))
        self.factory.protocol = ServerProtocol

        loop = asyncio.get_event_loop()
        coro = loop.create_server(self.factory, '0.0.0.0', port)
        server = loop.run_until_complete(coro)

        try:
            loop.run_forever()
        except KeyboardInterrupt:
            pass
        finally:
            server.close()
            loop.close()

    def startWsServer(self, port):
        websockets = Thread(target=self.websocketsLoop, args=(port,))
        websockets.setDaemon(True)
        websockets.start()
        print ("Websockets server started")

    def startWwwServer(self, port):
        self.run(host='0.0.0.0', port=port)


if __name__ == '__main__':
    backend = Backend(useTwitter=TWITTER_ENABLED)
    backend.startWsServer(WS_SERVER_PORT)
    backend.startWwwServer(WWW_SERVER_PORT)

