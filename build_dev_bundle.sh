#!/bin/bash

make
fakeroot dpkg-deb --build deb-bundle
mv deb-bundle.deb deb-pkg/e502monitor_0.8-1_amd64.deb
