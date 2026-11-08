# Uso: make (compila experimento y tests_bin) | make test | make tests_be | make test_be | make clean
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
HDRS     := $(wildcard src/*.h)

all: experimento tests_bin

experimento: src/main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ src/main.cpp

tests_bin: tests/tests.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/tests.cpp

test: tests_bin
	./tests_bin
	rm -f prueba_arbol_b.bin prueba_arbol_be.bin

clean:
	rm -f experimento tests_bin prueba_arbol_b.bin prueba_arbol_be.bin

.PHONY: all test clean

# Árbol Bε (persona A): make test_be compila y corre sus pruebas.
tests_be: tests/tests_be.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/tests_be.cpp

test_be: tests_be
	./tests_be

.PHONY: test_be
