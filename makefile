run:
	nasm -f elf32 -o my_print.o my_print.asm
	g++ -std=c++11 -m32 -o main main.cpp my_print.o
	./main
.PHONY: clean
clean:
	rm -rf *.o
	rm -rf main
