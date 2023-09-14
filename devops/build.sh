#!/bin/bash

sudo apt-get install debhelper

#mk-build-deps
debuild

# build source
debuild -S
