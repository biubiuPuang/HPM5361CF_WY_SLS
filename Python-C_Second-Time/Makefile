CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Iinclude
SRC := src/rs485_transport_win32.c src/servo_axis.c src/pedal_controller.c src/ids830abs_utils.c

.PHONY: all clean

all: build/retract_dual_pedals.exe build/read_status.exe build/emergency_stop.exe

build:
	mkdir -p build

build/retract_dual_pedals.exe: build $(SRC) examples/retract_dual_pedals.c
	$(CC) $(CFLAGS) $(SRC) examples/retract_dual_pedals.c -o $@

build/read_status.exe: build $(SRC) examples/read_status.c
	$(CC) $(CFLAGS) $(SRC) examples/read_status.c -o $@

build/emergency_stop.exe: build $(SRC) examples/emergency_stop.c
	$(CC) $(CFLAGS) $(SRC) examples/emergency_stop.c -o $@

clean:
	rm -rf build
