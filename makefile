build = ./build
src = ./src
bin = ./bin
lib = ./lib

all: tcp udp
	@echo "Done."

tcp: $(src)/tcp.c argparse.o | $(bin)
	@echo "Compiling tcp..."
	@gcc -g -o $(bin)/tcp $(src)/tcp.c $(build)/argparse.o

udp: $(src)/udp.c argparse.o | $(bin)
	@echo "Compiling udp..."
	@gcc -g -o $(bin)/udp $(src)/udp.c $(build)/argparse.o

argparse.o: $(lib)/argparse/argparse.c $(build)
	@echo "Compiling argparse lib..."
	@gcc -c -o $(build)/argparse.o $(lib)/argparse/argparse.c

build:
	@mkdir -p $(build)

bin:
	@mkdir -p $(bin)

clean:
	@rm -rf $(build)
	@rm -rf $(bin)