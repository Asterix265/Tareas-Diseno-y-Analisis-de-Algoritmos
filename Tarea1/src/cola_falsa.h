#pragma once
/**
 * cola_falsa.h — Cola de prioridad TRIVIAL (arreglo + búsqueda lineal).
 *
 * Solo para desarrollo y tests: permite probar Prim, el generador y el main
 * antes de que existan las colas reales. extractMin es O(n), así que NO se
 * usa en los experimentos. Cumple exactamente la interfaz de docs/ACUERDOS.md.
 */
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

class ColaFalsa {
public:
    /** Nombre con que main.cpp y los CSV identifican esta cola (--colas falsa). */
    static constexpr const char* nombre = "falsa";
    /** true: la cola participa en los tests y en ./prim (false = se omite con un aviso). */
    static constexpr bool implementada = true;

    /** Contador de operaciones estructurales en decreaseKey (aquí siempre 0). */
    int64_t ops = 0;

    /** Entrada: n = |V|. Reserva espacio para los vértices 0..n-1. */
    explicit ColaFalsa(int n) : clave(n, std::numeric_limits<double>::infinity()), presente(n, 0) {}

    /** Inserta el vértice v con costo key. */
    void insert(int v, double key) {
        clave[v] = key;
        presente[v] = 1;
        ++tam;
    }

    /** Extrae y retorna el par (costo, vértice) de menor costo. Empates: menor vértice. */
    std::pair<double, int> extractMin() {
        if (tam == 0) throw std::logic_error("extractMin sobre cola vacia");
        int mejor = -1;
        for (int v = 0; v < static_cast<int>(clave.size()); ++v)
            if (presente[v] && (mejor == -1 || clave[v] < clave[mejor])) mejor = v;
        presente[mejor] = 0;
        --tam;
        return {clave[mejor], mejor};
    }

    /** Reduce el costo del vértice v a key (key <= costo actual). */
    void decreaseKey(int v, double key) { clave[v] = key; }

    /** Salida: true si la cola no tiene elementos. */
    bool empty() const { return tam == 0; }

    /** Bytes por elemento almacenado (para la estimación de memoria). */
    static size_t bytesPorNodo() { return sizeof(double) + sizeof(char); }

private:
    std::vector<double> clave;
    std::vector<char> presente;
    int tam = 0;
};
