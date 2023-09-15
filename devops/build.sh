#!/bin/bash

sudo mk-build-deps --install $PWD/debian/control

#mk-build-deps
debuild

# build source
debuild -S

# output
echo "Now run dput ppa:newsworthy39/libvirtpromexporter ../libvirt-prometheus-exporter_x.x.x_source.changes"