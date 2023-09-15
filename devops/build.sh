#!/bin/bash

sudo mk-build-deps --install $PWD/debian/control

#mk-build-deps
debuild

# build source
debuild -S
