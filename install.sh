# Install script for TULIPP STHEM

#!/bin/bash

RELEASE=`lsb_release -a 2>/dev/null | grep Release | awk '{ print $2 }'`

if [[ $RELEASE == "17.10" ]]; then
    sudo apt install build-essential qt5-default libqt5charts5-dev libqt5sql5 libqt5sql5-sqlite libusb-1.0-0-dev llvm-5.0 llvm-5.0-dev clang-5.0 libclang-5.0-dev clang llvm

elif [[ $RELEASE == "18.04" ]]; then
    sudo apt install build-essential qt5-default libqt5charts5-dev libqt5sql5 libqt5sql5-sqlite libusb-1.0-0-dev llvm-6.0 llvm-6.0-dev clang-6.0 libclang-6.0-dev clang llvm

else
    echo "Ubuntu version not supported.  Please do a manual install."
    exit
fi

cd analysis_utility
make
cd ..

echo "#BEGIN TulippProfile" >> ~/.profile
echo export PATH=\$PATH:`pwd`/bin >> ~/.profile
echo "#END TulippProfile" >> ~/.profile

cd power_measurement_utility
sudo make install_hw
cd ..

