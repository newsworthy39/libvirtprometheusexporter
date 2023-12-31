#!/bin/bash

BUILDDIR="$(dpkg-parsechangelog --show-field Source)-$(dpkg-parsechangelog --show-field Version)"
mkdir ${BUILDDIR}
cp -r debian docs include src LICENSE Makefile README.md ${BUILDDIR}/.
cd ${BUILDDIR}
sudo mk-build-deps --install $PWD/debian/control

# build source
debuild -S

# remove build-directory
rm -rf ${BUILDDIR}

# output
echo "* run dput ppa:newsworthy39/monitoring ${BUILDDIR}_source.changes"
