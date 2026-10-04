/** Comprueba el generador, las colas de prioridad y el resultado de Prim. */
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
/** Registra una condición de prueba e imprime el mensaje si falla. */
#define REVISAR(cond, msg)                                                  \
    do {                                                                    \
        ++pruebas;                                                          \
        if (!(cond)) { ++fallas; std::cerr << "  FALLA: " << msg << "\n"; } \
    } while (0)

/** Construye un grafo CSR con n vértices a partir de la lista de aristas recibida. */
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

/** Calcula con Kruskal el peso del MST de un grafo conexo. */
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

/** Indica si los pesos a y b difieren dentro de la tolerancia numérica. */
static bool cerca(double a, double b) { return std::fabs(a - b) <= 1e-9 * std::max(1.0, std::fabs(b)); }

/** Busca la arista entre a y b en g y devuelve su peso, o -1 si no existe. */
static double pesoArista(const Grafo& g, int a, int b) {
    for (int64_t k = g.inicio[a]; k < g.inicio[a + 1]; ++k)
        if (g.destino[k] == b) return g.peso[k];
    return -1.0;
}

/** Comprueba las aristas, el peso y los contadores del resultado r sobre g.
 *  Usa caso para identificar las verificaciones registradas.
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

/** Prueba que el generador entregue grafos simples, conexos y reproducibles. */
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
    double suma = std::accumulate(a.peso.begin(), a.peso.end(), 0.0);
    std::cout << "  huella (semilla 42): destino[0..2]=" << a.destino[0] << "," << a.destino[1] << ","
              << a.destino[2] << "  suma pesos=" << std::setprecision(15) << suma << "\n";
}

/** Compara inserciones, reducciones y extracciones de Cola con ColaFalsa. */
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
            for (int t = 0; t < 3; ++t) {
                int v = static_cast<int>(rng.enRango(n));
                if (!dentro[v]) continue;
                clave[v] *= rng.peso();
                q.decreaseKey(v, clave[v]);
                ref.decreaseKey(v, clave[v]);
            }
            auto a = q.extractMin();
            auto b = ref.extractMin();
            if (a.first != b.first) ok = false;
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

/** Fuerza cortes en cascada y comprueba los contadores y el orden de extracción. */
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

/** Compara Prim con la cola indicada contra un MST conocido y contra Kruskal. */
template <class Cola>
static void probarPrim() {
    std::cout << "Prim con cola " << Cola::nombre << "\n";
    if (!Cola::implementada) { std::cout << "  (omitida: implementada = false)\n"; return; }

    // El MST del grafo de diez vértices es el camino 0-1-...-9, de peso 1.29.
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

/** Ejecuta todas las pruebas, imprime el resumen y devuelve 0 si pasan todas. */
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
