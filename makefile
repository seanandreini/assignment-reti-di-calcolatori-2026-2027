build = ./build
src = ./src
bin = ./bin
lib = ./lib

ifeq ($(OS),Windows_NT)
	EXT = .exe
	LIBS = -lws2_32
else
	EXT =
	LIBS =
endif

all: main
	@echo "Done."

main: $(src)/main.c argparse.o | $(bin)
	@echo "Compiling main..."
	@gcc -g -o $(bin)/main$(EXT) $(src)/main.c $(build)/argparse.o $(LIBS)

argparse.o: $(lib)/argparse/argparse.c | $(build)
	@echo "Compiling argparse lib..."
	@gcc -c -o $(build)/argparse.o $(lib)/argparse/argparse.c

build:
ifeq ($(OS),Windows_NT)
	@if not exist "build" mkdir "build"
else
	@mkdir -p $(build)
endif

bin:
ifeq ($(OS),Windows_NT)
	@if not exist "bin" mkdir "bin"
else
	@mkdir -p $(bin)
endif

clean:
ifeq ($(OS),Windows_NT)
	@if exist "build" rmdir /s /q "build"
	@if exist "bin" rmdir /s /q "bin"
else
	@rm -rf $(build)
	@rm -rf $(bin)
endif