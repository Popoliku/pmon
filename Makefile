
ARGS ?= ls -la

all: monitor

monitor: src/monitor.c
	gcc -Wall -Wextra -g src/monitor.c -o build/monitor

clean:
	rm -rf build/*

run: monitor
	./build/monitor $(ARGS)
