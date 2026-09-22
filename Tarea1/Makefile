# Uso: make (compila todo) | make test | make rapido | make memoria | make clean
# En Windows sin make, usar los comandos g++ del README.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
HDRS     := $(wildcard src/*.h)

all: prim tests_bin

prim: src/main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ src/main.cpp

tests_bin: tests/tests.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/tests.cpp

test: tests_bin
	./tests_bin

# Prueba rápida de toda la batería con grafos 2^4 veces más chicos
rapido: prim
	./prim --reducir 4 --reps 2

memoria: prim
	./prim --memoria

clean:
	rm -f prim tests_bin prim.exe tests_bin.exe

.PHONY: all test rapido memoria clean
