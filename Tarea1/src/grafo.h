#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

/** Guarda un grafo no dirigido y ponderado en arreglos CSR. */
struct Grafo {
    int n = 0;                    // Cantidad de vértices.
    int64_t m = 0;                // Cantidad de aristas no dirigidas.
    std::vector<int64_t> inicio;  // Los vecinos de u ocupan [inicio[u], inicio[u+1]).
    std::vector<int> destino;     // Vértice vecino de cada entrada de adyacencia.
    std::vector<double> peso;     // Peso de cada entrada, paralelo a destino.

    /** Estima los bytes del grafo CSR para n vértices y m aristas no dirigidas. */
    static size_t bytesEstimados(int64_t n, int64_t m) {
        return (size_t)(n + 1) * sizeof(int64_t) +
               (size_t)(2 * m) * (sizeof(int) + sizeof(double));
    }
};
