# Lager beer temperature controller
Trying to lowball a brewing temperature controller.  Basically:
+ A temperature sensor of some kind.  I found some Dallas 1-Wire protocol submersible stainless steel probes, so it'll be one of those
+ Single output will be to an electromechanical relay.  That'll run a small 120VAC submersible fountain pump.
+ Some kinda UI

I plan on bending some 1/4 inch copper tubing into a spiral that will be connected to the bottom of a brew bucket lid with fittings on top that go to plastic tubing.  That will run to a container of saltwater sitting in an otherwise unused dorm-sized fridge.  When temperature needs to go down in the wort the pump runs to circulate cool water through the tubing.  Pretty basic.

For UI, I plan on using an I/R remote control (like a universal TV remote) with some kind of display.  And what the fuck, let's put WiFi in, too.

## Code
This will be a bare-metal c++ project on the rPI Pico2040 chip.  I want to mess around with both cores, so the plan is for the WiFI and logger to run on the second core while temperature control and UI on the first core.

## Temperature control
No need for PID since the output is on/off, it'll just be bang/bang with some hysteresis.  

## WiFi
Yeah, put it chip in it. :/  

I got a couple of [AirLift](https://www.adafruit.com/product/4201) dev boards from Adafruit, pretty nice little thing for about $10USD.  There are some Arduino libraries, so I snagged those and modified them a bit to work with straight c++ on the Pico2040.

#### NTP
Because why shouldn't we know the real time of day.  There's a small NTP client that will get the date and time for my timezone (GMT-6)

#### UDP Server
Logging will be available by hitting a UDP socket on the chingus and it'll reply with whatever is in the log.  Probably some crappy Python script for the client

## I/R
Just an interrupt driven thing to read the timing of the 38KHz pulses from a [Vishay I/R reciever](https://www.vishay.com/docs/82490/tsop321.pdf).  Nothing fancy, just the normal NEC decoding.  

I did mess around with using the PIO on the Pico to do the decoding, but it was messier than just using the interrupt.  Someday I'll find an application for the PIO.

## UI
Not sure what the display will be yet...
