all: 
	sdcc -Ideps/libstm8s/inc -lstm8 -mstm8 -c --std-sdcc11 knx.c
	sdcc -Ideps/libstm8s/inc -lstm8 -mstm8 --out-fmt-ihx --std-sdcc11 main.c knx.rel

