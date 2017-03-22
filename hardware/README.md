## C&#x0413;ApL&#x2200;b SmartPlug hardware design

Here you'll find the SmartPlug schematic. Note that a few external, ready
made components are used in here, such as a 5V power supply, the LiPo battery
charger circuit, a 3.3V DC DC converter and, obviously, the ESP8266 module.
I'll represent those external components as simple blackboxes at this point.
And well, there probably won't be *a later point* anyway..

If you're seriously considering rebuilding this SmartPlug, be aware that this
circuit connects straight to the mains electricty
**DO NOT FUCK WITH MAINS ELECTRICTY!**
Seriously, don't ever lose respect for 230V/120V.
Well, now that you've been warned, here's what I used in the prototype circuit,
although alternative choices are available 

* any kind of 5V (switching) power supply. I'm using one with 700mA output,
  which is definitely overkill, but it had kinda perfect size to fit in the
  plug case I used.
* LiPo battery charging circuit and a battery along with it. My protopty has
  [this one from Olimex](https://www.olimex.com/Products/Power/USB-uLiPo/),
  modified to suit the charging current of the 250mAh battery I'm using.
  But again, any other form of charging circuit will do.
* 3.3V DC DC converter, again I chose [something from Olimex](https://www.olimex.com/Products/Breadboarding/BB-PWR-8009/)
  which I had available. The main reason for this is that the ESP8266 needs
  3.3V supply voltage and the battery charging circuit / battery will output
  anything between 4.2V and the last breath of your battery. With this
  converter, you can be sure it won't be over 3.3V and kill your ESP

Side note on the LiPo charing circuit: in this design, there's not separation
between battery charging and power supply circuit, which wouldn't be ideal in
a real device. See [Microchip's Application Note AN1149](http://ww1.microchip.com/downloads/en/AppNotes/01149c.pdf)
for more information on this.


### What this actually does

The initial idea was simple: detect when the plug is connected to the mains,
and detect when it gets unplugged. In each case, send a message to somewhere.

Detecting the *plugged in* is simple, if it's powered up, it has to be pluuged
in - duh. So, if you unplug it, it's obviously ...hmm, right, it's powered off.
So add a battery to keep the circuit powered after losing the main power supply
and use the charger circuit's power input as detection signal if a power supply
is available or not (add a voltage divider to comply with the ESP8266's max
GPIO voltage, since the charger uses 5V and the ESP8266 3.3V)

But then, once the *unplugged* status was sent, the whole thing should be
powered off after all. For once, suicide seemed to be a valid solution: the
ESP8266 will have a GPIO that will disconnect the battery from its supply
voltage pin. The keyword here is **high side switch** implemented with a
P-channel MOSFET.

Using a ESP8266 module with plentiful exposed GPIOs, this worked just fine.
But I wanted a more compact (and cheaper) solution and tried to use the
ESP-01 module. You basically have just GPIO0 and GPIO2 available on that one,
and those are used to determine the ESP's boot mode on power up. In other
words, you are able to use the as GPIO, but you have to make sure they have a
specific state during power up.

#### Summarized
* `U1` ESP-01 module
* `U2` 230V/120V to 5V power supply
* `U3` Battery charger circuit
* `U4` DC DC converter to ensure 3.3V supply voltage for ESP8266
* `R1, R2` voltage divider determining "power plug connected" in 3.3V tolerant levels
* `Q1` high side switch, connect/cut battery charger to power ESP8266 on/off
* `Q2` switch `Q1` on when power plug is connected
* `Q3` switch `Q1` on if pulled high from ESP8266 (i.e. suicide switch)
* `Q4` make sure GPIO2 is high during power up to set proper boot mode

