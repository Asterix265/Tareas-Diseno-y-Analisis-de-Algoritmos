#pragma once
/**
 * prim.h — Algoritmo de Prim genérico sobre la cola de prioridad (sección 4.1).
 *
 * Traducción línea a línea de Prim(G, r) del enunciado: cada bloque lleva el
 * número de línea del pseudocódigo. Lo que no pertenece al pseudocódigo
 * (medición de tiempo y contadores) está marcado como [medición].
 *
 * `Cola` debe cumplir la interfaz de docs/ACUERDOS.md (ColaBinomial,
 * ColaFibonacci o ColaFalsa). Prim no sabe nada de nodos: solo usa
 * insert / extractMin / decreaseKey / empty / contiene y los contadores
 * `ops` y `opsCascada`.
 */
#include <chrono>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "grafo.h"

/** Valor de parent[v] para "indefinido" (línea 3); distinto del −1 de la raíz (línea 1). */
constexpr int INDEFINIDO = -2;

/** Un punto de la curva de costo acumulado de decreaseKey. */
struct PuntoCurva {
    int64_t llamadas;  // llamadas a decreaseKey hechas hasta este punto
    int64_t tiempoNs;  // tiempo acumulado de esas llamadas
    int64_t ops;       // operaciones estructurales acumuladas (contador `ops` de la cola)
    int64_t opsCascada;  // cortes en cascada acumulados (contador `opsCascada` de la cola)
};

/** Resultado de una ejecución de Prim. */
struct ResultadoPrim {
    std::vector<std::pair<int, int>> T;  // aristas (parent[v], v) del MST (línea 14)
    double pesoTotal = 0.0;         // suma de los costos c de las aristas agregadas a T (línea 8)
    double tiempoMs = 0.0;          // tiempo total de Prim, líneas 1 a 14
    int64_t dkLlamadas = 0;         // cantidad de llamadas a decreaseKey
    int64_t dkTiempoNs = 0;         // suma de tiempos de decreaseKey (solo si medirDK)
    int64_t dkOps = 0;              // intercambios (binomial) o todos los cortes (Fibonacci)
    int64_t dkOpsCascada = 0;       // solo cortes hechos por cascadingCut (Fibonacci; 0 en las otras)
    std::vector<PuntoCurva> curva;  // un punto cada `cadaK` llamadas (solo si medirDK y cadaK > 0)
};

/**
 * Línea 4 del pseudocódigo: Q ← construir(costos), mediante inserciones
 * sucesivas (sección 3.4 del enunciado).
 * Entrada: Q, cola vacía creada para n = costos.size() vértices; costos[v] de cada vértice.
 * Salida: Q con los n pares (costos[v], v).
 * Recibe Q por referencia porque las colas no se pueden copiar ni mover
 * (C++17 no permite retornarlas por valor desde una variable local).
 */
template <class Cola>
void construir(Cola& Q, const std::vector<double>& costos) {
    for (int v = 0; v < static_cast<int>(costos.size()); ++v) Q.insert(v, costos[v]);
}

/**
 * Prim(G, r): MST del grafo conexo g partiendo desde el vértice r.
 * Entrada:
 *   g        grafo conexo.
 *   r        vértice raíz.
 *   medirDK  si es true, mide el tiempo de cada decreaseKey (series C y D).
 *            En las series A y B va en false para no sumar el costo del reloj.
 *   cadaK    si > 0 y medirDK, guarda un PuntoCurva cada cadaK llamadas
 *            (cadaK = 1: un punto por llamada; main.cpp lo reduce después).
 *            main.cpp solo lo usa en una ejecución extra para la curva, nunca
 *            en las 10 repeticiones medidas. El push_back ocurre fuera del
 *            intervalo medido de decreaseKey, y se reservan g.m / cadaK + 1
 *            puntos: cada arista provoca a lo sumo un decreaseKey, así que el
 *            vector nunca se realoca durante Prim.
 * Salida: ResultadoPrim con T, su peso y las mediciones. El tiempo total cubre
 *         las líneas 1 a 14; no incluye generar el grafo ni destruir Q.
 */
template <class Cola>
ResultadoPrim prim(const Grafo& g, int r = 0, bool medirDK = false, int64_t cadaK = 0) {
    using Reloj = std::chrono::steady_clock;
    const double INF = std::numeric_limits<double>::infinity();
    const int n = g.n;

    ResultadoPrim res;
    if (medirDK && cadaK > 0) res.curva.reserve(static_cast<size_t>(g.m / cadaK + 1));  // [medición]

    const auto t0 = Reloj::now();  // [medición] antes de la línea 1
    Reloj::time_point t1;

    std::vector<double> costos(static_cast<size_t>(n));
    std::vector<int> parent(static_cast<size_t>(n));
    // 1: costos[r] ← 0, parent[r] ← −1
    costos[r] = 0.0;
    parent[r] = -1;
    // 2: for v ∈ V ∖ {r}:
    for (int v = 0; v < n; ++v) {
        if (v == r) continue;
        // 3: costos[v] ← ∞, parent[v] ← indefinido
        costos[v] = INF;
        parent[v] = INDEFINIDO;
    }
    {
        // 4: Q ← construir(costos)
        Cola Q(n);
        construir(Q, costos);
        // 5: T ← ∅  (se reserva |V| − 1: el MST tiene exactamente |V| − 1 aristas, sección 2)
        std::vector<std::pair<int, int>> T;
        T.reserve(static_cast<size_t>(n > 0 ? n - 1 : 0));
        // 6: while Q ≠ ∅:
        while (!Q.empty()) {
            // 7: (c, v) ← extractMin(Q)
            const std::pair<double, int> cv = Q.extractMin();
            const double c = cv.first;
            const int v = cv.second;
            // 8: if v ≠ r: T ← T ∪ {(parent[v], v)}
            if (v != r) {
                T.push_back({parent[v], v});
                res.pesoTotal += c;  // c = costos[v] = w(parent[v], v)
            }
            // 9: for u ∈ vecinos(v) tal que u ∈ Q:
            for (int64_t k = g.inicio[v]; k < g.inicio[v + 1]; ++k) {
                const int u = g.destino[k];
                if (!Q.contiene(u)) continue;
                const double w = g.peso[k];  // w(v, u)
                // 10: if w(v, u) < costos[u]:
                if (w < costos[u]) {
                    // 11: costos[u] ← w(v, u)
                    costos[u] = w;
                    // 12: parent[u] ← v
                    parent[u] = v;
                    // 13: decreaseKey(Q, u, w(v, u))
                    if (medirDK) {  // [medición] series C y D
                        const auto a = Reloj::now();
                        Q.decreaseKey(u, w);
                        const auto b = Reloj::now();
                        // Desde aquí ya no se mide: acumular y guardar la curva no suma a dkTiempoNs.
                        res.dkTiempoNs += std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
                        ++res.dkLlamadas;
                        if (cadaK > 0 && res.dkLlamadas % cadaK == 0)
                            res.curva.push_back({res.dkLlamadas, res.dkTiempoNs, Q.ops, Q.opsCascada});
                    } else {
                        Q.decreaseKey(u, w);
                        ++res.dkLlamadas;  // [medición]
                    }
                }
            }
        }
        t1 = Reloj::now();  // [medición] antes de destruir Q: liberar memoria no es parte de Prim
        res.dkOps = Q.ops;
        res.dkOpsCascada = Q.opsCascada;
        // 14: return T
        res.T = std::move(T);
    }
    res.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}
