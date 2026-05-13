build = ./build
src = ./src
bin = ./bin
lib = ./lib

all: tcp
	@echo "Done."

tcp: $(src)/tcp.c argparse.o | $(bin)
	@echo "Compiling tcp..."
	@gcc -g -o $(bin)/tcp $(src)/tcp.c $(build)/argparse.o

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