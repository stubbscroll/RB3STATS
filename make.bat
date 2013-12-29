gcc -o rb3stats.exe rb3stats.c -O3 -Wall -lcurl -lcurldll
gcc -o genimage.exe genimage.c sdlfont.c -O3 -Wall -lmingw32 -lsdlmain -lsdl -lsdl_mixer -Wl,--subsystem,windows
