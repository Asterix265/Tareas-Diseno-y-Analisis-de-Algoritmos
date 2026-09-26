#pragma once
/**
 * grafo.h — Representación del grafo (CSR: Compressed Sparse Row).
 *
 * Los vecinos del vértice u están en las posiciones [inicio[u], inicio[u+1])
 * de los arreglos `destino` y `peso`. Cada arista no dirigida {u,v} se guarda
 * dos veces (u->v y v->u), como pide el enunciado.
 *
 * Se eligió CSR en vez de vector<vector<pair>> porque usa memoria contigua,
 * sin overhead por vector, lo que importa para e = 2^24.
 */
#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * Grafo no dirigido con pesos, en formato CSR (listas de adyacencia en arreglos contiguos).
 * Lo construye generarGrafo (generador.h) y lo lee prim (prim.h).
 */
struct Grafo {
    int n = 0;                    // |V|, vértices numerados 0..n-1
    int64_t m = 0;                // |E|, aristas no dirigidas
    std::vector<int64_t> inicio;  // tamaño n+1
    std::vector<int> destino;     // tamaño 2m
    std::vector<double> peso;     // tamaño 2m, pesos en (0,1]

    /**
     * Estimación analítica de bytes para un grafo CSR con n vértices y m aristas.
     * Entrada: n, m. Salida: bytes.
     */
    static size_t bytesEstimados(int64_t n, int64_t m) {
        return (size_t)(n + 1) * sizeof(int64_t) +
               (size_t)(2 * m) * (sizeof(int) + sizeof(double));
    }
};
