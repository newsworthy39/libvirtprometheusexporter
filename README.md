# Name
    libvirt-prometheus-exporter - a prometheus exporter for libvirt

# Synopsis
    libvirt-prometheus-exporter {http-port} {libvirt-ipc-url}

# Description
    
    http-port
        the http-port to listen on.
    libvirt-ipc-url
        e.g qemu:///system

# Environment for systemd
    /etc/default/libvirt-prometheus-exporter
    

# Build and test
- libvirt (> 8.0)
- debhelper (>9.0)
- g++ (>10)
- Make

## build
    bash devops/build.sh

### Changelog
    remember to update the changelog in debian/changelog

Made by newsworthy39