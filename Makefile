
CC=g++ -std=c++20 -Iinclude
CFLAGS=-c -Wall
SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
LDFLAGS=-lvirt
EXECUTABLE=libvirt-prometheus-exporter

all: clean $(SOURCES) $(EXECUTABLE) 

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm -f $(EXECUTABLE) $(OBJECTS)

uninstall:
	rm -f /usr/bin/$(EXECUTABLE)

install:
	install -m 0755 $(EXECUTABLE) /usr/sbin
