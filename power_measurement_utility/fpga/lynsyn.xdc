###############################################################################
# Timing constraints
###############################################################################

create_clock -period 20.833 -name mcu_clk [get_ports MCU_OSC]
create_clock -period 50.000 -name jtag_clk [get_ports JTAG_OSC]
create_clock -period 50.000 -name spi_clk [get_ports SCLK]

###############################################################################
# Clock groups
###############################################################################

set_clock_groups -asynchronous -group mcu_clk -group jtag_clk -group spi_clk -group clk

###############################################################################
# IO standard
###############################################################################

set_property IOSTANDARD LVCMOS33 [get_ports *]
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

###############################################################################
# Location constraints
###############################################################################

# MCU SPI
set_property PACKAGE_PIN K13 [get_ports INT]
set_property PACKAGE_PIN P16 [get_ports nSS]
set_property PACKAGE_PIN L14 [get_ports MOSI]
set_property PACKAGE_PIN M16 [get_ports MISO]
set_property PACKAGE_PIN N13 [get_ports SCLK]

# Oscillators
set_property PACKAGE_PIN N14 [get_ports MCU_OSC]
set_property PACKAGE_PIN N11 [get_ports JTAG_OSC]

# JTAG_IN
set_property PACKAGE_PIN C12 [get_ports JTAG_IN_TMS]
set_property PACKAGE_PIN D13 [get_ports JTAG_IN_TCK]
set_property PACKAGE_PIN C13 [get_ports JTAG_IN_TDO]
set_property PACKAGE_PIN E12 [get_ports JTAG_IN_TDI]
set_property PACKAGE_PIN E13 [get_ports JTAG_IN_TRST]

set_property PULLTYPE PULLUP [get_ports JTAG_IN_TRST]

# JTAG_OUT
set_property PACKAGE_PIN R8 [get_ports JTAG_OUT_TMS]
set_property PACKAGE_PIN T7 [get_ports JTAG_OUT_TCK]
set_property PACKAGE_PIN T8 [get_ports JTAG_OUT_TDO]
set_property PACKAGE_PIN T9 [get_ports JTAG_OUT_TDI]
set_property PACKAGE_PIN T10 [get_ports JTAG_OUT_TRST]

###############################################################################
# Timing constraints
###############################################################################

set_max_delay -from [get_ports JTAG_IN_TMS]  -to [get_ports JTAG_OUT_TMS]  8.000
set_max_delay -from [get_ports JTAG_IN_TCK]  -to [get_ports JTAG_OUT_TCK]  8.000
set_max_delay -from [get_ports JTAG_OUT_TDO] -to [get_ports JTAG_IN_TDO]   8.000
set_max_delay -from [get_ports JTAG_IN_TDI]  -to [get_ports JTAG_OUT_TDI]  8.000
set_max_delay -from [get_ports JTAG_IN_TRST] -to [get_ports JTAG_OUT_TRST] 8.000

###############################################################################
# Bitfile constraints
###############################################################################

set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
set_property CONFIG_MODE SPIx4 [current_design]
