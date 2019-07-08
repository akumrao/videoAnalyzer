.PHONY:	clean all

all: 
	g++ -std=c++17 -g *.cpp -lm -lSDL2  -o plot

clean:
	rm -rf plot

exe:
	g++ -std=c++17 -g *.cpp -lm -lSDL2 -o plot

wasm:
	. /root/emsdk_set_env.sh && emcc *.cpp -g4 -std=c++17 -s WASM=1 -s USE_SDL=2  -o index.js                                                                               

run:
	echo "http://localhost:8080/index.html"	
	python -m SimpleHTTPServer 8080
	


