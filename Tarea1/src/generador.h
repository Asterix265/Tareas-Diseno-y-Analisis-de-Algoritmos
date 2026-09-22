#pragma once
/**
 * generador.h — Generador de grafos conexos aleatorios (sección 6.1).
 *
 * Método:
 *  1. Árbol cobertor: para cada vértice i = 1..v-1 se agrega la arista
 *     {i, p} con p uniforme en [0, i-1]. Garantiza conectividad.
 *  2. Se agregan aristas {a,b} aleatorias (a != b) hasta completar e.
 *     Las repetidas se descartan y se vuelven a sortear (por lotes:
 *     se ordena, se eliminan duplicados y se sortean las que faltan).
 *  3. Pesos uniformes en (0,1], asignados en orden determinista.
 *
 * No usa matrices v x v (el generador de la tarea anterior era O(v^2) en
 * memoria, imposible para v = 2^22). Memoria extra: 8 bytes por arista.
 */
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "aleatorio.h"
#include "grafo.h"

namespace detalle {
/** Codifica la arista no dirigida {a,b} como un entero de 64 bits (menor, mayor). */
inline uint64_t clave(uint32_t a, uint32_t b) {
    if (a > b) std::swap(a, b);
    return (static_cast<uint64_t>(a) << 32) | b;
}
}  // namespace detalle

/**
 * Genera un grafo conexo, simple (sin multiaristas ni lazos), no dirigido.
 * Entrada: v = |V| (>= 2), e = |E| con v-1 <= e <= v(v-1)/2, semilla.
 * Salida: Grafo en formato CSR. Misma semilla => mismo grafo en cualquier SO.
 */
inline Grafo generarGrafo(int v, int64_t e, uint32_t semilla) {
    const int64_t maxAristas = static_cast<int64_t>(v) * (v - 1) / 2;
    if (v < 2 || e < v - 1 || e > maxAristas)
        throw std::invalid_argument("generarGrafo: parametros invalidos v=" +
                                    std::to_string(v) + " e=" + std::to_string(e));

    Aleatorio rng(semilla);
    std::vector<uint64_t> claves;
    claves.reserve(static_cast<size_t>(e));

    // 1. Árbol cobertor aleatorio (aristas distintas por construcción).
    for (uint32_t i = 1; i < static_cast<uint32_t>(v); ++i) {
        uint32_t p = static_cast<uint32_t>(rng.enRango(i));
        claves.push_back(detalle::clave(p, i));
    }

    // 2. Aristas restantes, descartando repetidas y volviendo a sortear.
    while (static_cast<int64_t>(claves.size()) < e) {
        int64_t faltan = e - static_cast<int64_t>(claves.size());
        for (int64_t k = 0; k < faltan; ++k) {
            uint32_t a = static_cast<uint32_t>(rng.enRango(v));
            uint32_t b = static_cast<uint32_t>(rng.enRango(v));
            while (b == a) b = static_cast<uint32_t>(rng.enRango(v));  // sin lazos
            claves.push_back(detalle::clave(a, b));
        }
        std::sort(claves.begin(), claves.end());
        claves.erase(std::unique(claves.begin(), claves.end()), claves.end());
    }

    // 3. Construcción CSR: primero grados, luego se llenan las listas.
    Grafo g;
    g.n = v;
    g.m = e;
    g.inicio.assign(static_cast<size_t>(v) + 1, 0);
    for (uint64_t c : claves) {
        g.inicio[(c >> 32) + 1]++;
        g.inicio[(c & 0xFFFFFFFFu) + 1]++;
    }
    for (int u = 0; u < v; ++u) g.inicio[u + 1] += g.inicio[u];

    g.destino.resize(static_cast<size_t>(2 * e));
    g.peso.resize(static_cast<size_t>(2 * e));
    std::vector<int64_t> cursor(g.inicio.begin(), g.inicio.end() - 1);
    for (uint64_t c : claves) {
        int a = static_cast<int>(c >> 32);
        int b = static_cast<int>(c & 0xFFFFFFFFu);
        double w = rng.peso();
        g.destino[cursor[a]] = b; g.peso[cursor[a]++] = w;
        g.destino[cursor[b]] = a; g.peso[cursor[b]++] = w;
    }
    return g;
}
