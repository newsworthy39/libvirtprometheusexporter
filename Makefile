
CC=g++ -std=c++20 -Iinclude
LINK=-lvirt

.PHONY: all
all: clean libvirtexporter

libvirtexporter: src/prometheus.cpp
	$(CC) $< -o $@ $(LINK)

clean: 
	rm -f libvirtexporter 

uninstall:
	rm -f /usr/bin/libvirtexporter

install:
	install -m 0755 libvirtexporter /usr/bin
