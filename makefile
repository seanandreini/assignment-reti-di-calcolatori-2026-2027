build = ./build
src = ./src
bin = ./bin
lib = ./lib

all: main
	@echo "Done."

main: $(src)/main.c argparse.o | $(bin)
	@echo "Compiling main..."
	@gcc -g -o $(bin)/main $(src)/main.c $(build)/argparse.o


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