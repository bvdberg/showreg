
CC=arm-none-linux-gnueabi-g++
#CC=g++
ARGS=-O2 -Wall

all: showreg XmlReader.o

XmlReader.o: XmlReader.cpp
	$(CC) $(ARGS) -c XmlReader.cpp -o XmlReader.o

showreg: main.cpp XmlReader.o
	$(CC) $(ARGS) main.cpp XmlReader.o -o showreg

clean:
	rm -f showreg *.o

remake: clean all
