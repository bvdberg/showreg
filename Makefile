showreg: showreg.c
	arm-none-linux-gnueabi-gcc -O2 showreg.c -o showreg

clean:
	rm -f showreg

