TARGET = atmega328p
PROGM = usbasp
PROGM_UC = m328p

CC = avr-gcc
CFLAGS = -fshort-enums -ffunction-sections -funsigned-char -std=c11 -Os -w -ffreestanding -DF_CPU=16000000UL -mmcu=$(TARGET) -fno-rtti

all: main.o lcd.o ftoa.o millis.o oled.o app ./build/app.bin
	avr-objcopy -O ihex -R .eeprom ./build/app.bin ./rel/app.hex

flash: all
	sudo avrdude -c ${PROGM} -p ${PROGM_UC} -U flash:w:rel/app.hex


main.o: main.c
	$(CC) $(CFLAGS) -c -o ./build/main.o main.c

lcd.o: ./lcd.c ./lcd.h
	$(CC) $(CFLAGS) -c -o ./build/lcd.o ./lcd.c

ftoa.o: ./ftoa.c ./ftoa.h
	$(CC) $(CFLAGS) -c -o ./build/ftoa.o ./ftoa.c

millis.o: ./millis.c ./millis.h
	$(CC) $(CFLAGS) -c -o ./build/millis.o ./millis.c

oled.o: ./oled.c ./oled.h
	$(CC) $(CFLAGS) -c -o ./build/oled.o ./oled.c

app: main.o lcd.o ftoa.o millis.o oled.o
	$(CC) -mmcu=$(TARGET) ./build/main.o ./build/lcd.o ./build/ftoa.o ./build/millis.o ./build/oled.o -o ./build/app.bin


clean:
	rm -rf ./build/*
