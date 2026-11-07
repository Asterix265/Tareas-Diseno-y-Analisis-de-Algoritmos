CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
HDRS     := $(wildcard src/*.h)

all: experimento_b tests_bin

experimento_b: src/main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ src/main.cpp

tests_bin: tests/tests.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/tests.cpp

test: tests_bin
	./tests_bin
	rm -f prueba_arbol_b.bin

clean:
	rm -f experimento_b tests_bin prueba_arbol_b.bin

.PHONY: all test clean
