# Lager beer temperature controller
Trying to lowball a brewing temperature controller.  Basically:
+ A temperature sensor of some kind.  I found some Dallas 1-Wire protocol submersible stainless steel probes, so it'll be one of those
+ Single output will be to an electromechanical relay.  That'll run a small 120VAC submersible fountain pump.
+ Some kinda UI

I plan on bending some 1/4 inch copper tubing into a spiral that will be connected to the bottom of a brew bucket lid with fittings on top that go to plastic tubing.  That will run to a container of saltwater sitting in an otherwise unused dorm-sized fridge.  When temperature needs to go down in the wort the pump runs to circulate cool water through the tubing.  Pretty basic.

For UI, I plan on using an I/R remote control (like a universal TV remote) with some kind of display.  And what the fuck, let's put WiFi in, too.

## Code
This will be a bare-metal c++ project on the rPI Pico2040 chip.  I want to mess around with both cores, so the plan is for anything that might be blocking to run on core 1 while the rest runs on core 0.

#### Breakdown by directory
In general, here are the file layout.  For more detail, look at the comments in the files.  

+  `. (project root)`
   +  The "main" functions for both cores; core 0 in `main.cpp`, core 1 in the appropriately-named `core1.cpp`.
   +  `project.h` with some really common defines
   +  `creds.h` wifi credentials used for default parameters
   +  `reefer.h/.cpp` the refrigeration class.  This will handle turning the pump output on and off and such.  It's implemented as a bone-simple state machine.  About the only thing interesting is that I decided minimum on and off time for the pump should by 60 seconds, so there're a few extra states for that.
   +  `pull.py` a python script that acts as a UDP client to the chingus.  Used to pull the log on a periodic basis.  Can also command the chingus to reboot or reboot to bootloader
+  `./af`, `./alibs` - these are files I pulled from [Adafruit for the Airlift Wifi module](https://github.com/adafruit/nina-fw).  They are Arduino libraries that I modified to be used in bare-metal ARM.  Of course, I also had to get the dependencies from the Arduino libraries and make them build, too.  Did you know I kinda dislike the Arduino system - the dependencies are a mess and the IDE is junk and so much is abstracted away from you... </rant>  
+  `doc` - documentation as I add it...
+  `ds1820` - the temperature sensor is a 1-wire thing, so I just [stole some code](https://www.i-programmer.info/programming/hardware/14527-the-pico-in-c-a-1-wire-pio-program.html) for it.  It uses a PIO state machine.  I cleaned up the code, replaced of the naked arrays with STL containers, and put it all in a class.
+  `ipc` stuff common to both cores
   +  `ipc.h/.cpp` - Interprocess communications (inter-core, really).  Handle syncing of data, commands, status between cores.  The pico2040 silicon does have a pair of 32-bit FIFOs for this, but I will not be using them for a couple of reasons - I don't want it to be blocking (which the FIFOs are), and they will not be available when using the core locking API (more info later)
   +  `mlogger.h/.cpp` - logger handler.  The log is a circular buffer that can be written to by either core; there's a mutex to handle concurrency.  The data can be "pulled" by an Ethernet UDP client.
+  `pilznet/pilznet.h/.cpp` - a wrapper class around the Ethernet module.  Nothing fancy, just wraps it up and does the stuff I want it to.  Specifically, it will try to connect to the specified access point, it will do NTP to get time (for logging), and will be a UDP server.  See the class and `pull.py` for more
+  `references` - datasheets and the like
+  `sys` utility and system stuff
   +  `ir.h/.cpp` - Infrared remote decode class
   +  `nvm.h/.cpp` - non-volatile memory handler.  Non-vol stuff will be written to Flash memory, there's information in the header of `nvm.cpp` for details about inter-core locking
   +  `walltime.h/.cpp` - real-time clock handler
+  `utils` - `stringFormat.h/.cpp` a utility to do `printf()` type formatting with STL strings.  

## Temperature control
No need for PID since the output is on/off, it'll just be bang/bang with some hysteresis and minimum on/off times for the pump.

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
