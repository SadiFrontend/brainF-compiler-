CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
TARGET = bfc

all: $(TARGET)

$(TARGET): bfc.c
	$(CC) $(CFLAGS) -o $(TARGET) bfc.c

clean:
	rm -f $(TARGET) *.o *.s program

# Example: compile and run a brainfuck program
test: $(TARGET)
	./$(TARGET) examples/hello.bf output.s
	as output.s -o output.o
	ld output.o -o program
	./program

.PHONY: all clean test
