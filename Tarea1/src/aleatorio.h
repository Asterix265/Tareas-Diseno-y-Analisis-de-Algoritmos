#pragma once
#include <cstdint>
#include <random>

/** Genera enteros y pesos pseudoaleatorios a partir de una semilla reproducible. */
class Aleatorio {
public:
    /** Inicializa el generador con la semilla recibida. */
    explicit Aleatorio(uint32_t semilla) : gen(semilla) {}

    /** Combina dos salidas de mt19937 y devuelve un entero de 64 bits. */
    uint64_t siguiente64() {
        uint64_t alto = gen();
        uint64_t bajo = gen();
        return (alto << 32) | bajo;
    }

    /** Recibe k > 0 y devuelve un entero en [0, k). */
    uint64_t enRango(uint64_t k) { return siguiente64() % k; }

    /** Convierte una salida de mt19937 en un peso uniforme discreto en (0, 1]. */
    double peso() { return (static_cast<double>(gen()) + 1.0) / 4294967296.0; }

private:
    std::mt19937 gen;
};
