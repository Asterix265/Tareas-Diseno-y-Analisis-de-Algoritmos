#pragma once
/**
 * prim.h — Algoritmo de Prim genérico sobre la cola de prioridad (sección 4.1).
 *
 * `Cola` debe cumplir la interfaz de docs/ACUERDOS.md (ColaBinomial,
 * ColaFibonacci o ColaFalsa). Prim no sabe nada de nodos: solo usa
 * insert / extractMin / decreaseKey / empty y el contador `ops`.
 */
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

#include "grafo.h"

/** Un punto de la curva de costo acumulado de decreaseKey. */
struct PuntoCurva {
    int64_t llamadas;  // llamadas a decreaseKey hechas hasta este punto
    int64_t tiempoNs;  // tiempo acumulado de esas llamadas
    int64_t ops;       // operaciones estructurales acumuladas
};

/** Resultado de una ejecución de Prim. */
struct ResultadoPrim {
    double pesoTotal = 0.0;         // peso del MST
    std::vector<int> padre;         // padre[v] en el MST (-1 para la raíz)
    double tiempoMs = 0.0;          // tiempo total de Prim (construcción de Q incluida)
    int64_t dkLlamadas = 0;         // cantidad de llamadas a decreaseKey
    int64_t dkTiempoNs = 0;         // suma de tiempos de decreaseKey (solo si medirDK)
    int64_t dkOps = 0;              // intercambios (binomial) o cortes (Fibonacci)
    std::vector<PuntoCurva> curva;  // un punto cada `cadaK` llamadas (solo si medirDK)
};

/**
 * Ejecuta Prim sobre g partiendo desde `raiz`.
 * Entrada:
 *   g        grafo conexo.
 *   raiz     vértice inicial.
 *   medirDK  si es true, mide el tiempo de cada decreaseKey (series C y D).
 *            En las series A y B va en false para no sumar el costo del reloj.
 *   cadaK    si > 0 y medirDK, guarda un PuntoCurva cada cadaK llamadas
 *            (cadaK = 1: un punto por llamada; main.cpp lo reduce después).
 *            El push_back ocurre fuera del intervalo medido de decreaseKey, y
 *            se reserva g.m / cadaK + 1 puntos: cada arista provoca a lo sumo
 *            un decreaseKey, así que el vector nunca se realoca durante Prim.
 * Salida: ResultadoPrim. La generación del grafo NO entra en la medición.
 */
template <class Cola>
ResultadoPrim prim(const Grafo& g, int raiz = 0, bool medirDK = false, int64_t cadaK = 0) {
    using Reloj = std::chrono::steady_clock;
    const double INF = std::numeric_limits<double>::infinity();
    const int n = g.n;

    ResultadoPrim r;
    r.padre.assign(n, -1);
    std::vector<double> costo(n, INF);
    std::vector<char> enQ(n, 1);  // enQ[u] <=> u todavía está en Q
    if (medirDK && cadaK > 0) r.curva.reserve(static_cast<size_t>(g.m / cadaK + 1));

    Reloj::time_point t1;
    const auto t0 = Reloj::now();
    {
        Cola Q(n);
        costo[raiz] = 0.0;
        for (int v = 0; v < n; ++v) Q.insert(v, costo[v]);  // construcción: n inserciones

        while (!Q.empty()) {
            const std::pair<double, int> par = Q.extractMin();
            const double c = par.first;
            const int v = par.second;
            enQ[v] = 0;
            r.pesoTotal += c;  // la raíz aporta 0; el resto, el peso de (padre[v], v)

            for (int64_t k = g.inicio[v]; k < g.inicio[v + 1]; ++k) {
                const int u = g.destino[k];
                const double w = g.peso[k];
                if (enQ[u] && w < costo[u]) {
                    costo[u] = w;
                    r.padre[u] = v;
                    if (medirDK) {
                        const auto a = Reloj::now();
                        Q.decreaseKey(u, w);
                        const auto b = Reloj::now();
                        // Desde aquí ya no se mide: acumular y guardar la curva no suma a dkTiempoNs.
                        r.dkTiempoNs += std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
                        ++r.dkLlamadas;
                        if (cadaK > 0 && r.dkLlamadas % cadaK == 0)
                            r.curva.push_back({r.dkLlamadas, r.dkTiempoNs, Q.ops});
                    } else {
                        Q.decreaseKey(u, w);
                        ++r.dkLlamadas;
                    }
                }
            }
        }
        t1 = Reloj::now();  // se mide antes de destruir Q: liberar memoria no es parte de Prim
        r.dkOps = Q.ops;
    }
    r.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return r;
}
