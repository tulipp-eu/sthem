# Install script for TULIPP STHEM

#!/bin/bash

RELEASE=`lsb_release -a 2>/dev/null | grep Release | awk '{ print $2 }'`

if [[ $RELEASE == "17.10" ]]; then
    echo "Ubuntu 17.10"

elif [[ $RELEASE == "18.04" ]]; then
    echo "Ubuntu 18.04"

elif [[ $RELEASE == "18.10" ]]; then
    echo "Ubuntu 18.10"

else
    echo "Ubuntu version not supported.  Please do a manual install."
    exit
fi

sudo apt install build-essential qt5-default libqt5charts5-dev libqt5sql5 libqt5sql5-sqlite libusb-1.0-0-dev llvm llvm-dev clang libclang-dev

cd analysis_utility
make
cd ..

echo "#BEGIN TulippProfile" >> ~/.bashrc
echo PATH=\"\$PATH:`pwd`/analysis_utility/bin\" >> ~/.bashrc
echo "#END TulippProfile" >> ~/.bashrc

PATH=$PATH:`pwd`/analysis_utility/bin

cd power_measurement_utility
sudo make install_hw
cd ..

echo
echo
echo "*** STHEM is now installed"

