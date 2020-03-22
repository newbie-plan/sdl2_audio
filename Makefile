CFLAG=-I /usr/local/sdl2/include
LDFLAG=-L /usr/local/sdl2/lib -lSDL2

alsa_sdl:alsa_sdl2.c
	gcc -g $< -o $@ ${CFLAG} ${LDFLAG}
