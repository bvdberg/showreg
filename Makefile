
CC=arm-none-linux-gnueabi-g++
CC=g++

all: showreg XmlReader.o

XmlReader.o: XmlReader.cpp
	$(CC) -O2 -c XmlReader.cpp -o XmlReader.o

showreg: main.cpp XmlReader.o
	$(CC) -O2 main.cpp XmlReader.o -o showreg

clean:
	rm -f showreg *.o

