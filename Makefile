TARGET = atmega328
PROGM = usbasp
PROGM_UC = m328

CC = avr-gcc
CFLAGS = -fshort-enums -ffunction-sections -funsigned-char -std=c99 -Os -DF_CPU=8000000UL -mmcu=$(TARGET)

all: main.o lcd.o ftoa.o app ./build/app.bin
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


app: main.o lcd.o ftoa.o
	$(CC) -mmcu=$(TARGET) ./build/main.o ./build/lcd.o ./build/ftoa.o -o ./build/app.bin


clean:
	rm -rf ./build/*
