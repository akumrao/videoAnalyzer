all:
	g++ -std=c++17 -g *.c core/*.c  decoder/*.c encoder/*.c -lm  -o x264EncdoerDecoder

clean:
	rm -rf x264EncdoerDecoder

so:
	g++ -std=c++17 -g -fPIC -shared -o libx264.so *.c core/*.c  decoder/*.c encoder/*.c

