#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

/** Guarda claves por vértice y encuentra el mínimo mediante búsqueda lineal. */
class ColaFalsa {
public:
    static constexpr const char* nombre = "falsa";
    static constexpr bool implementada = true;

    int64_t ops = 0;         // No realiza intercambios ni cortes.
    int64_t opsCascada = 0;  // No realiza cortes en cascada.

    /** Prepara una cola vacía para los vértices 0..n-1. */
    explicit ColaFalsa(int n) : clave(n, std::numeric_limits<double>::infinity()), presente(n, 0) {}

    /** Inserta el vértice v con el costo key. */
    void insert(int v, double key) {
        clave[v] = key;
        presente[v] = 1;
        ++tam;
    }

    /** Busca, elimina y devuelve el par (costo, vértice) mínimo. */
    std::pair<double, int> extractMin() {
        if (tam == 0) throw std::logic_error("extractMin sobre cola vacia");
        int mejor = -1;
        for (int v = 0; v < static_cast<int>(clave.size()); ++v)
            if (presente[v] && (mejor == -1 || clave[v] < clave[mejor])) mejor = v;
        presente[mejor] = 0;
        --tam;
        return {clave[mejor], mejor};
    }

    /** Actualiza el costo del vértice v con la nueva clave key. */
    void decreaseKey(int v, double key) { clave[v] = key; }

    /** Indica si la cola está vacía. */
    bool empty() const { return tam == 0; }

    /** Indica si el vértice v sigue presente en la cola. */
    bool contiene(int v) const { return presente[v] != 0; }

    /** Devuelve los bytes usados por clave y presencia de un vértice. */
    static size_t bytesPorNodo() { return sizeof(double) + sizeof(char); }

private:
    std::vector<double> clave;  // Costo guardado para cada vértice.
    std::vector<char> presente; // Indica qué vértices siguen en la cola.
    int tam = 0;                // Cantidad de vértices presentes.
};
