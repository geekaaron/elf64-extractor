
.PHONY: all clean

all: ee

ee: ee.c utils.c
	- gcc -o $@ $^

clean:
	- rm ee
