showreg: main.cpp XmlReader.cpp
	arm-none-linux-gnueabi-g++ -O2 -c XmlReader.cpp -o XmlReader.o
	arm-none-linux-gnueabi-g++ -O2 main.cpp XmlReader.o -o showreg

clean:
	rm -f showreg *.o

