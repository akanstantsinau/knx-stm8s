all: 
	sdcc -Ideps/libstm8s/inc -lstm8 -mstm8 --out-fmt-ihx --std-sdcc11 main.c

