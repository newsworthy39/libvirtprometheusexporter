
CC=g++ -std=c++20 -Iinclude
LINK=-lvirt

.PHONY: all
all: clean libvirt-prometheus-exporter

libvirt-prometheus-exporter: src/prometheus.cpp
	$(CC) $< -o $@ $(LINK)

clean: 
	rm -f libvirt-prometheus-exporter 

uninstall:
	rm -f /usr/bin/libvirt-prometheus-exporter

install:
	install -m 0755 libvirt-prometheus-exporter /usr/bin
