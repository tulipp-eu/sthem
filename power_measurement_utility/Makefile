export SDK=/opt/SimplicityStudio_v4/developer/sdks/gecko_sdk_suite/v2.3/
export TOOLCHAIN=/opt/SimplicityStudio_v4/developer/toolchains/gnu_arm/7.2_2017q4/bin/

.PHONY: all
all:
	mkdir -p bin
	cd mcu && $(MAKE)
	cp mcu/lynsyn.bin bin
	cd fpga && $(MAKE)
	cp fpga/build/lynsyn.mcs bin
	cd lynsyn_tester && $(MAKE)
	cp lynsyn_tester/lynsyn_tester bin

.PHONY: install_hw
install_hw:
	cp udev/48-lynsyn.rules /etc/udev/rules.d
	udevadm control --reload-rules

.PHONY : clean
clean :
	cd mcu && $(MAKE) clean
	cd fpga && $(MAKE) clean
	cd lynsyn_tester && $(MAKE) clean
