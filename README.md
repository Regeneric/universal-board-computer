## Getting started
More about project (in Polish): https://itcrowd.net.pl/devblog-1-czym-jest-ubc-jak-dziala-i-plany-na-rozwoj/

Known issue is that `avr-gcc` for Windows doesn't recognize the `-Os` flag so it won't compile due to large output file size. 

I won't try to make it work in the future, Windows doesn't concern me at all. But if someone has an idea, what should we do with that fact - go ahead.

The other, simpler solution is to just use **WSL**

### Hardware
If you're using `usbasp` programmer, make sure that you've made right connections to `MISO`, `MOSI`, `SCK` and `RESET` pins.

Also you HAVE TO use pairs of capcitors (*100 nF + 2-40 uF*) for the filtering of the power supply output. Make sure that analog section of the AVR is powered up, too.  
Don't forget to use pull-up resistor for `PC6` aka `RESET` pin.

You also need to intercept and read fuel injector pulse and VSS signal from your car. Without that **UBC** cannot work.

Default pins on **Atmega 328P** for reading user's input are `PD6, PD7 and PB0`. 

**UBC** is designed to work with `8 MHz internal oscillator` of **Atmega 328P.**

## Software
To get it work with your car, you need to count VSS pulses and divide known distance by them, and know how much liters of fuel your injector is injecting in one second.  

### VSS
For the VSS you need to be in the calibration mode.
On the first screen press and hold "*FUN*" button for **6** seconds. 
Then you're going to see three lines:
```c
40 0
41 0
56 1
```
Line `40` is telling you how much pulses from the VSS it registered.  
Line `41` is counting all overflows (if there were any).  
Line `56` is number of all fuel injectors.

To calibrate the device, you need to drive **exactly 10 kilometeres**. The more precise you are, the better.

Here are two examples, both are correct:
```c
40 42627
41 0
56 6
```

```c
40 12317
41 3
56 4
```

But if you already know the correct values, you can enter them by hand.  
To do so press and hold "*FUN*" button, then you can add `500` pulses by pressing "*NEXT*" button or substract `500` pulses by pressing "*PREV*" button. 

Then release all the buttons and start pressing "*FUN*" button until the number  of injectors is on the desired level.
After all of that press "*NEXT*" to proceed to the next screen.

### Injector

For the fuel injector I read the datasheet and searched for the `cc/min` value.  
In my case injectors can inject `149.8 cc/min`, but in the calibration mode we can add or substract only by `0.5`. So, the closest value would be `150 cc/min`.  

On the calibration screen you are going to see two lines:
```c
50 100.0
55 .0
```

Line `50` is the `cc/min` value.  
Line `55` is the divide factor for the fuel left in tank, whe you use float to measure it.   

The value is calculated using simple formula:  
`divideFactor = 1024/maxTankCap`  
So fo the 68 liters tank it would be ~15.  

First you need to press and hold "*FUN*" button, then you can add `0.5` cc/min by pressing "*NEXT*" button or substract `0.5` cc/min by pressing "*PREV*" button. 

Then release all the buttons and start pressing "*FUN*" button until the divide factor is on the desired level.

Here are two examples, both are correct:
```c
50 150.0
55 .5
```

```c
50 195.5
55 22
```

Press "*NEXT*" button to save all the calibration data and then restart the device.

### Navigation

To clear data on the current screen, press and hold "*FUN*" button for **3 seconds**.

To access "secret menu" press and hold "*FUN*" button for **1 second** on one of the three screens.

### Debian
```bash
sudo apt update
sudo apt install avr-gcc avr-libc make

git clone https://github.com/Regeneric/universal-board-computer.git
cd universal-board-computer/

# To just compile
make

# To compile and flash
make flash
```

### Arch
```bash
sudo pacman -Syu
sudo pacman -S avr-gcc avr-libc make

git clone https://github.com/Regeneric/universal-board-computer.git
cd universal-board-computer/

# To just compile
make

# To compile and flash
make flash
```
