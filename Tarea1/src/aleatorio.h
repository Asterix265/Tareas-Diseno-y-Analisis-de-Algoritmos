#pragma once
/**
 * aleatorio.h — Generador pseudoaleatorio PORTABLE.
 *
 * std::mt19937 produce exactamente la misma secuencia en GCC, clang (Mac) y
 * MSVC, pero std::uniform_*_distribution NO (cada biblioteca estándar las
 * implementa distinto). Por eso convertimos la salida de mt19937 a mano:
 * así una misma semilla genera el mismo grafo en Windows, Mac y el servidor.
 */
#include <cstdint>
#include <random>

class Aleatorio {
public:
    /** Entrada: semilla. Crea el generador. */
    explicit Aleatorio(uint32_t semilla) : gen(semilla) {}

    /** Salida: entero uniforme de 64 bits (dos llamadas a mt19937, en orden fijo). */
    uint64_t siguiente64() {
        uint64_t alto = gen();
        uint64_t bajo = gen();
        return (alto << 32) | bajo;
    }

    /**
     * Entrada: k > 0. Salida: entero en [0, k).
     * Usa módulo sobre 64 bits: el sesgo es < k / 2^64, despreciable.
     */
    uint64_t enRango(uint64_t k) { return siguiente64() % k; }

    /** Salida: double uniforme en (0, 1] (nunca 0, como pide el enunciado). */
    double peso() { return (static_cast<double>(gen()) + 1.0) / 4294967296.0; }

private:
    std::mt19937 gen;
};
