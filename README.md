Keep in mind that the software isn't in a working state yet, and the following are v1.0 goals.
#RPI_BTC_ATM
This is a open-source Bitcoin ATM that can be built for under $100 dollars. It uses the CH-926 coin acceptor currently, but other forms of payment are possible in the future. This ATM is meant for small deposits and withdrawals, and is meant to be kept on a countertop.

##Parts
###You are going to need
####Raspberry Pi
You just need a computer with GPIO headers and a connection to the Internet. It doesn't have to be a Raspberry Pi.
####Webcam
As long as Linux recognizes it as a webcam, it should work. The only use this webcam has is scanning QR codes for Bitcoin addresses, so 640x480 is a sane minimum resolution.
####CH-926 coin acceptor
This is a popular programmable coin acceptor that can be bought on the cheap. There is a pre-programmed lookup table for the pulses (100ms, 1 pulse is a penny, 2, is a nickel, 3, is a dime, 4 is a quarter). It supports a maximum of 8 different coins (or so I heard). Since the power draw is 12V @ 55mA, there should be enough power provided by the GPIO pins to run this device.
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
