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

Default pin on **Atmega 8** for reading user's input is `PD7`. 

**UBC** is designed to work with` 8 MHz internal oscillator` of **Atmega 8.**

## Software
```c
// main.c:37-38
volatile static const float PULSE_DISTANCE = 0.00006823;
volatile static const float INJECTION_VALUE = 0.0031667;

// Values for Audi 80 with ABK engine (2.0 115KM, gasoline), year 1993.
```

To get it work with your car, you need to count VSS pulses and divide known distance by them, and know how much liters of fuel your injector is injecting in one second.  

For the VSS I drove 10 kilometeres and then divided it by the sum of all VSS pulses.
For the fuel injector I read the datasheet and searched for the `cc/min` value.

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
