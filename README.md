Keep in mind that the software isn't in a working state yet, and the following are v1.0 goals.
#RPI_BTC_ATM
This is a open-source Bitcoin ATM that can be built for under $100 dollars. It uses the CH-926 coin acceptor currently, but other forms of payment are possible in the future. This ATM is meant for small deposits, and is meant to be kept on a countertop.

##Dependencies
####bitcoind
You should run it with -prune=550 to keep disk usage low. Set -rpcusername and -rpcpassword properly and use the --json-rpc-username and --json-rpc-password fields properly

##Parts
###You are going to need
####Raspberry Pi
You just need a Linux-based computer with GPIO headers and a connection to the Internet. It doesn't have to be a Raspberry Pi.
####Webcam
As long as it is recognized as a webcam, it should work. The only use this webcam has is scanning QR codes for Bitcoin addresses, so 640x480 is a sane minimum resolution.
####CH-926 coin acceptor
This is a popular programmable coin acceptor that can be bought on the cheap. It supports a maximum of 8 different coins (or so I heard). Since the power draw is 12V @ 55mA, an external power supply is needed (can be derived from a 2.1A USB brick somehow, look into implementation-specific manuals for that). Here is the pre-programmed list (100ms):
- 1 pulse = penny
- 2 pulses = nickel
- 3 pulses = dime
- 4 pulses = quarter

####Internet connection
For the sake of simplicity, wired is the best option. However, wireless, when configured properly, should work just as well.

###You probably want
####16x2 screen
You don't have to buy this, and enough output can come from misc. LEDs that are connected through GPIO, but it would help the UX.
####LEDs
LEDs are simple to use on the programming side, and can be used as the de-facto debugging tool when there is no screen present. All you need to do is solder it to the GPIO headers and you should be set.

##TODO
- Use the BV20 bill acceptor
- Use an NFC/RFID reader for Bitcoin addresses
- Make a unified ATM case that can be 3D printed
