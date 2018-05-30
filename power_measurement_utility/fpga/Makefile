build/lynsyn.mcs: lynsyn.xdc rtl/defines.v rtl/lynsyn.v rtl/spi.v rtl/jtag.v 
	mkdir -p build
	cd build && vivado -mode batch -source ../run_vivado.tcl

.PHONY: clean
clean:
	rm -rf build vivado* .Xil
