/**
 * tests.cpp — Pruebas de corrección en grafos chicos (recomendación 8b y 8c).
 *
 * Compilar y correr desde Tarea1/:
 *   g++ -std=c++17 -O2 -Wall -Wextra -Isrc -o tests_bin tests/tests.cpp && ./tests_bin
 *
 * Las pruebas de una cola se omiten mientras su `implementada` sea false.
 */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "aleatorio.h"
#include "cola_binomial.h"
#include "cola_falsa.h"
#include "cola_fibonacci.h"
#include "generador.h"
#include "grafo.h"
#include "prim.h"

static int fallas = 0, pruebas = 0;
/**
 * Registra una prueba. Entrada: condición `cond` y mensaje `msg` (se puede
 * encadenar con <<). Salida: si cond es falsa, cuenta una falla e imprime msg en stderr.
 */
#define REVISAR(cond, msg)                                                  \
    do {                                                                    \
        ++pruebas;                                                          \
        if (!(cond)) { ++fallas; std::cerr << "  FALLA: " << msg << "\n"; } \
    } while (0)

/** Construye un Grafo CSR a partir de una lista de aristas (solo para tests). */
static Grafo desdeAristas(int n, const std::vector<std::tuple<int, int, double>>& as) {
    Grafo g;
    g.n = n;
    g.m = static_cast<int64_t>(as.size());
    g.inicio.assign(n + 1, 0);
    for (auto& [a, b, w] : as) { (void)w; g.inicio[a + 1]++; g.inicio[b + 1]++; }
    for (int u = 0; u < n; ++u) g.inicio[u + 1] += g.inicio[u];
    g.destino.resize(2 * as.size());
    g.peso.resize(2 * as.size());
    std::vector<int64_t> cur(g.inicio.begin(), g.inicio.end() - 1);
    for (auto& [a, b, w] : as) {
        g.destino[cur[a]] = b; g.peso[cur[a]++] = w;
        g.destino[cur[b]] = a; g.peso[cur[b]++] = w;
    }
    return g;
}

/**
 * Kruskal con union-find: MST de referencia, independiente de las colas.
 * Entrada: grafo conexo g. Salida: peso total del MST.
 */
static double kruskal(const Grafo& g) {
    std::vector<std::tuple<double, int, int>> as;
    for (int u = 0; u < g.n; ++u)
        for (int64_t k = g.inicio[u]; k < g.inicio[u + 1]; ++k)
            if (u < g.destino[k]) as.emplace_back(g.peso[k], u, g.destino[k]);
    std::sort(as.begin(), as.end());
    std::vector<int> p(g.n);
    std::iota(p.begin(), p.end(), 0);
    auto raiz = [&](int x) { while (p[x] != x) x = p[x] = p[p[x]]; return x; };
    double total = 0;
    for (auto& [w, a, b] : as) {
        int ra = raiz(a), rb = raiz(b);
        if (ra != rb) { p[ra] = rb; total += w; }
    }
    return total;
}

/**
 * Compara dos pesos con la misma tolerancia que main.cpp.
 * Entrada: a, b. Salida: true si |a − b| <= 1e-9 · max(1, |b|).
 */
static bool cerca(double a, double b) { return std::fabs(a - b) <= 1e-9 * std::max(1.0, std::fabs(b)); }

/**
 * Busca la arista {a,b} en la lista de adyacencia de a.
 * Entrada: grafo g y extremos a, b. Salida: w(a,b), o -1 si la arista no existe.
 */
static double pesoArista(const Grafo& g, int a, int b) {
    for (int64_t k = g.inicio[a]; k < g.inicio[a + 1]; ++k)
        if (g.destino[k] == b) return g.peso[k];
    return -1.0;
}

/**
 * Revisa el resultado de Prim: T tiene |V| − 1 aristas del grafo, la suma de
 * sus pesos es pesoTotal y dk_ops_cascada <= dk_ops.
 * Entrada: grafo g, resultado r de prim sobre g, y texto del caso para los mensajes.
 * Salida: registra las pruebas con REVISAR.
 */
static void revisarT(const Grafo& g, const ResultadoPrim& r, const std::string& caso) {
    REVISAR(static_cast<int64_t>(r.T.size()) == g.n - 1, caso << ": T con |V|-1 aristas");
    double suma = 0.0;
    bool existen = true;
    for (const auto& [p, v] : r.T) {
        const double w = pesoArista(g, p, v);
        if (w < 0) existen = false;
        suma += w;
    }
    REVISAR(existen, caso << ": toda arista de T existe en el grafo");
    REVISAR(cerca(suma, r.pesoTotal), caso << ": suma de pesos de T " << suma << " != pesoTotal " << r.pesoTotal);
    REVISAR(r.dkOpsCascada <= r.dkOps, caso << ": dk_ops_cascada <= dk_ops");
}

/** Revisa que el grafo sea simple, conexo, con e aristas y pesos en (0,1]. */
static void probarGenerador() {
    std::cout << "Generador\n";
    const int casos[][2] = {{4, 4}, {4, 6}, {6, 8}, {10, 10}, {10, 12}, {12, 16}, {14, 18}};
    for (auto& ij : casos) {
        int v = 1 << ij[0];
        int64_t e = int64_t(1) << ij[1];
        if (e > int64_t(v) * (v - 1) / 2) continue;
        Grafo g = generarGrafo(v, e, 1234 + ij[0] * 100 + ij[1]);
        REVISAR(g.n == v && g.m == e && g.inicio[v] == 2 * e, "tamanos v=2^" << ij[0] << " e=2^" << ij[1]);

        std::set<std::pair<int, int>> vistas;
        bool simple = true, pesosOk = true;
        for (int u = 0; u < v; ++u)
            for (int64_t k = g.inicio[u]; k < g.inicio[u + 1]; ++k) {
                int w = g.destino[k];
                if (w == u || !vistas.insert({u, w}).second) simple = false;
                if (!(g.peso[k] > 0.0 && g.peso[k] <= 1.0)) pesosOk = false;
            }
        REVISAR(simple, "sin lazos ni multiaristas");
        REVISAR(pesosOk, "pesos en (0,1]");

        std::vector<char> vis(v, 0);
        std::queue<int> q;
        q.push(0); vis[0] = 1;
        int cnt = 1;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int64_t k = g.inicio[u]; k < g.inicio[u + 1]; ++k)
                if (!vis[g.destino[k]]) { vis[g.destino[k]] = 1; ++cnt; q.push(g.destino[k]); }
        }
        REVISAR(cnt == v, "conexo");
    }
    Grafo a = generarGrafo(256, 1000, 42), b = generarGrafo(256, 1000, 42);
    REVISAR(a.destino == b.destino && a.peso == b.peso, "misma semilla => mismo grafo");
    // Huella para comparar entre Windows / Mac / servidor: debe imprimir lo mismo en los tres.
    double suma = std::accumulate(a.peso.begin(), a.peso.end(), 0.0);
    std::cout << "  huella (semilla 42): destino[0..2]=" << a.destino[0] << "," << a.destino[1] << ","
              << a.destino[2] << "  suma pesos=" << std::setprecision(15) << suma << "\n";
}

/**
 * Secuencia aleatoria de operaciones comparada contra ColaFalsa; también
 * revisa contiene() de ambas colas después de cada extracción.
 * Entrada: tipo Cola (plantilla). Salida: registra las pruebas con REVISAR.
 */
template <class Cola>
static void probarCola() {
    std::cout << "Cola " << Cola::nombre << "\n";
    if (!Cola::implementada) { std::cout << "  (omitida: implementada = false)\n"; return; }
    for (int n : {1, 2, 3, 10, 100, 1000}) {
        Aleatorio rng(7 + n);
        Cola q(n);
        ColaFalsa ref(n);
        std::vector<double> clave(n);
        std::vector<char> dentro(n, 1), dentroRef(n, 1);
        for (int v = 0; v < n; ++v) {
            clave[v] = rng.peso();
            q.insert(v, clave[v]);
            ref.insert(v, clave[v]);
        }
        bool ok = true, contieneOk = true;
        for (int v = 0; v < n; ++v)
            if (!q.contiene(v) || !ref.contiene(v)) contieneOk = false;
        int quedan = n;
        while (quedan > 0 && ok) {
            for (int t = 0; t < 3; ++t) {  // algunos decreaseKey entre extracciones
                int v = static_cast<int>(rng.enRango(n));
                if (!dentro[v]) continue;
                clave[v] *= rng.peso();
                q.decreaseKey(v, clave[v]);
                ref.decreaseKey(v, clave[v]);
            }
            auto a = q.extractMin();
            auto b = ref.extractMin();
            if (a.first != b.first) ok = false;  // el vértice puede diferir si hay empates
            dentro[a.second] = 0;
            dentroRef[b.second] = 0;
            --quedan;
            for (int v = 0; v < n; ++v)
                if (q.contiene(v) != (dentro[v] != 0) || ref.contiene(v) != (dentroRef[v] != 0)) contieneOk = false;
        }
        REVISAR(ok && q.empty(), "extractMin coincide con la referencia, n=" << n);
        REVISAR(contieneOk, "contiene() coincide con los vertices presentes, n=" << n);
    }
}

/** Provoca pérdidas de hijos desde las hojas para comprobar cortes en cascada. */
static void probarCortesCascada() {
    ColaFibonacci q(128);
    for (int v = 0; v < 128; ++v) q.insert(v, static_cast<double>(v));
    REVISAR(q.extractMin() == std::make_pair(0.0, 0), "mínimo inicial Fibonacci");
    bool huboCascada = false;
    for (int v = 127; v >= 1; --v) {
        int64_t antes = q.ops;
        q.decreaseKey(v, -static_cast<double>(128 - v));
        if (q.ops - antes > 1) huboCascada = true;
    }
    REVISAR(huboCascada, "Fibonacci realiza y cuenta cortes en cascada");
    REVISAR(q.opsCascada > 0 && q.opsCascada <= q.ops, "opsCascada cuenta los cortes en cascada y opsCascada <= ops");
    for (int v = 1; v < 128; ++v)
        REVISAR(q.extractMin().second == v, "decreaseKey conserva el vértice tras los cortes");
    REVISAR(q.empty(), "Fibonacci vacía después de todos los cortes");
}

/** Prim con la cola dada contra Kruskal, en un grafo a mano y en grafos aleatorios. */
template <class Cola>
static void probarPrim() {
    std::cout << "Prim con cola " << Cola::nombre << "\n";
    if (!Cola::implementada) { std::cout << "  (omitida: implementada = false)\n"; return; }

    // Grafo a mano (10 vértices). MST: camino 0-1-...-9,
    // pesos 0.1+0.2+0.15+0.3+0.25+0.05+0.12+0.08+0.04 = 1.29.
    Grafo mano = desdeAristas(10, {{0, 1, 0.1}, {1, 2, 0.2}, {2, 3, 0.15}, {3, 4, 0.3},
                                   {4, 5, 0.25}, {5, 6, 0.05}, {6, 7, 0.12}, {7, 8, 0.08},
                                   {8, 9, 0.04}, {0, 2, 0.9}, {1, 3, 0.8}, {2, 4, 0.7},
                                   {0, 6, 0.6}, {3, 6, 0.5}, {5, 8, 0.6}, {6, 9, 0.4}});
    ResultadoPrim r = prim<Cola>(mano);
    REVISAR(cerca(r.pesoTotal, 1.29), "grafo a mano: peso " << r.pesoTotal << " != 1.29");
    revisarT(mano, r, "grafo a mano");

    for (int t = 0; t < 30; ++t) {
        int v = 2 + t * 7;
        int64_t e = std::min<int64_t>(int64_t(v) * (v - 1) / 2, v - 1 + t * 13);
        Grafo g = generarGrafo(v, e, 500 + t);
        ResultadoPrim rp = prim<Cola>(g, 0, t % 2 == 0, 5);
        REVISAR(cerca(rp.pesoTotal, kruskal(g)), "aleatorio v=" << v << " e=" << e);
        revisarT(g, rp, "aleatorio v=" + std::to_string(v));
    }
}

/** Corre todas las pruebas. Salida: resumen por stdout; código 0 si no hubo fallas, 1 si las hubo. */
int main() {
    probarGenerador();
    probarCola<ColaBinomial>();
    probarCola<ColaFibonacci>();
    probarCortesCascada();
    probarPrim<ColaFalsa>();
    probarPrim<ColaBinomial>();
    probarPrim<ColaFibonacci>();
    std::cout << "\n" << (pruebas - fallas) << "/" << pruebas << " pruebas OK\n";
    return fallas ? 1 : 0;
}
