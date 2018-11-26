
OBJS= blitz.o hex.o main.o rawhex.o serial.o
TARGET=blitzer
CFLAGS= -g

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

$(TARGET): $(OBJS)
	gcc -g -o $@ $^

