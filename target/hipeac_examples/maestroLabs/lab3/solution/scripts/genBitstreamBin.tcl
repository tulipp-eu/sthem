set bitstream_path "tulippTutorial.bit"

write_cfgmem -force \
             -format BIN \
             -interface SMAPx32 \
             -disablebitswap \
             -loadbit "up 0 $bitstream_path" \
             "$bitstream_path.bin"
exit
