#!/bin/bash

set -x

# Install packages
apt-get install -y python2
apt-get install -y wget

# Download the right version of GNU Arm Embedded Toolchain (version 4.9.3) and untar it.
wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
tar xjf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
rm gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2

# Get to the project's root.
cd ..

# Build the kernel (not completely, just a little bit, enough to generate the required headers).
cp .trustinsoft/tis_arm.config .config
make ARCH=arm CROSS_COMPILE=`pwd`/.trustinsoft/gcc-arm-none-eabi-4_9-2015q3/bin/arm-none-eabi- oldconfig
make ARCH=arm CROSS_COMPILE=`pwd`/.trustinsoft/gcc-arm-none-eabi-4_9-2015q3/bin/arm-none-eabi- || :

# Ready to analyze!
