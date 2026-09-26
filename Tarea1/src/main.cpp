/**
 * main.cpp — Ejecuta toda la batería de experimentos (sección 6) sin modificar código.
 *
 * Uso (desde Tarea1/):
 *   ./prim                         # todo: series A B C D, 10 repeticiones, binomial y fibonacci
 *   ./prim --series AB --reps 3    # subconjunto
 *   ./prim --reducir 4             # i-=4, j-=4: prueba rápida en un notebook
 *   ./prim --colas falsa --reducir 10
 *   ./prim --memoria               # solo imprime la estimación de memoria (6.2)
 *   ./prim --calibrar              # solo mide el costo de steady_clock::now() (10^6 llamadas)
 *
 * Salida (carpeta --salida, por defecto resultados/), un archivo por invocación:
 *   tiempos_<series>.csv       una fila por (configuración, repetición, cola)
 *   curvas_<series>.csv        curva de decreaseKey acumulado (series C y D): sale de una
 *                              ejecución extra sobre el grafo de la repetición 0, que no
 *                              se escribe en tiempos_<series>.csv
 *   verificacion_<series>.csv  comparación del peso del MST entre colas
 *
 * Código de salida: 0 si todo terminó bien; 1 si los argumentos son inválidos o no hay
 * colas para ejecutar; 2 si algún MST tuvo pesos distintos entre colas.
 */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cola_binomial.h"
#include "cola_falsa.h"
#include "cola_fibonacci.h"
#include "generador.h"
#include "grafo.h"
#include "prim.h"

/** Una configuración (i, j) de una serie: v = 2^i, e = 2^j. */
struct Config {
    char serie;  // 'A', 'B', 'C' o 'D' (sección 6.3)
    int i, j;
};

/**
 * Series de la sección 6.3. A y B: costo total. C y D: costo amortizado.
 * Entrada: series, letras de las series a correr (p. ej. "ABCD"); una letra desconocida termina el programa.
 * Salida: las configuraciones (serie, i, j) de esas series, en el orden pedido.
 */
static std::vector<Config> configuraciones(const std::string& series) {
    std::vector<Config> cs;
    for (char s : series) {
        switch (s) {
            case 'A': for (int j = 20; j <= 24; ++j) cs.push_back({'A', 20, j}); break;
            case 'B': for (int i = 18; i <= 22; ++i) cs.push_back({'B', i, 24}); break;
            case 'C': for (int j = 18; j <= 22; ++j) cs.push_back({'C', 18, j}); break;
            case 'D': for (int i = 14; i <= 18; ++i) cs.push_back({'D', i, 22}); break;
            default: std::cerr << "Serie desconocida: " << s << "\n"; std::exit(1);
        }
    }
    return cs;
}

/**
 * Semilla determinista por (serie, i, j, rep): cualquier corrida se puede repetir.
 * Entrada: base (--semilla), configuración c (sin --reducir) y repetición rep (< 100).
 * Salida: base + serie·10^6 + i·10^4 + j·100 + rep.
 */
static uint32_t semilla(uint32_t base, const Config& c, int rep) {
    return base + static_cast<uint32_t>((c.serie - 'A') * 1000000 + c.i * 10000 + c.j * 100 + rep);
}

/**
 * Ejecuta Prim con la cola indicada por nombre, desde la raíz 0.
 * Entrada: cola ("binomial", "fibonacci" o "falsa"), grafo g, medirDK y cadaK (ver prim en prim.h).
 * Salida: el ResultadoPrim de esa ejecución. Lanza invalid_argument si la cola no existe.
 */
static ResultadoPrim ejecutar(const std::string& cola, const Grafo& g, bool medirDK, int64_t cadaK) {
    if (cola == "binomial") return prim<ColaBinomial>(g, 0, medirDK, cadaK);
    if (cola == "fibonacci") return prim<ColaFibonacci>(g, 0, medirDK, cadaK);
    if (cola == "falsa") return prim<ColaFalsa>(g, 0, medirDK, cadaK);
    throw std::invalid_argument("cola desconocida: " + cola);
}

/** Cantidad de puntos por curva cuando --cada es automático. */
static constexpr size_t PUNTOS_CURVA = 4096;

/**
 * Reduce una curva con un punto por llamada a `puntos` puntos espaciados
 * uniformemente según la cantidad real de llamadas (el último siempre se incluye).
 * Entrada: curva completa y cantidad deseada. Salida: curva reducida.
 */
static std::vector<PuntoCurva> reducirCurva(const std::vector<PuntoCurva>& curva, size_t puntos) {
    const size_t n = curva.size();
    if (n <= puntos) return curva;
    std::vector<PuntoCurva> r;
    r.reserve(puntos);
    for (size_t t = 1; t <= puntos; ++t) r.push_back(curva[t * n / puntos - 1]);
    return r;
}

/**
 * Entrada: nombre de una cola. Salida: true si la cola existe y tiene
 * `implementada = true`; false si no existe o si todavía no está implementada.
 */
static bool implementada(const std::string& cola) {
    if (cola == "binomial") return ColaBinomial::implementada;
    if (cola == "fibonacci") return ColaFibonacci::implementada;
    if (cola == "falsa") return ColaFalsa::implementada;
    return false;
}

/** Entrada: cantidad de bytes. Salida: texto con esa cantidad en MiB y un decimal (p. ej. "24.3 MiB"). */
static std::string mb(double bytes) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << bytes / (1024.0 * 1024.0) << " MiB";
    return o.str();
}

/**
 * Estimación analítica de memoria (sección 6.2) para v vértices y e aristas.
 * Entrada: v, e. Salida: imprime en stdout cada componente y los picos estimados.
 */
static void imprimirMemoria(int64_t v, int64_t e) {
    const double grafo = static_cast<double>(Grafo::bytesEstimados(v, e));
    const double aux = static_cast<double>(v) * (sizeof(double) + sizeof(int));  // costos, parent
    const double ptrs = static_cast<double>(v) * sizeof(void*);                  // nodoDe
    const double arbol = static_cast<double>(v - 1) * sizeof(std::pair<int, int>);  // T (línea 5)
    const double gen = static_cast<double>(e) * sizeof(uint64_t);  // temporal del generador
    const double cursor = static_cast<double>(v) * sizeof(int64_t);  // cursor del paso CSR
    // Pico en la generación: CSR completo + claves + cursor viven a la vez (paso 3).
    const double picoGen = grafo + gen + cursor;
    // Pico en Prim: CSR + nodos de la cola + nodoDe + costos/parent + T.
    const double picoPrimBin = grafo + aux + ptrs + arbol + static_cast<double>(v) * ColaBinomial::bytesPorNodo();
    const double picoPrimFib = grafo + aux + ptrs + arbol + static_cast<double>(v) * ColaFibonacci::bytesPorNodo();
    std::cout << "v = " << v << ", e = " << e << "\n"
              << "  Lista de adyacencia (CSR, 2e entradas): " << mb(grafo) << "\n"
              << "  Arreglos costos/parent:                 " << mb(aux) << "\n"
              << "  Arreglo de punteros a nodos de Q:       " << mb(ptrs) << "\n"
              << "  Aristas del MST (T, v-1 pares):         " << mb(arbol) << "\n"
              << "  Nodos cola binomial  (" << ColaBinomial::bytesPorNodo() << " B/nodo):  "
              << mb(static_cast<double>(v) * ColaBinomial::bytesPorNodo()) << "\n"
              << "  Nodos cola Fibonacci (" << ColaFibonacci::bytesPorNodo() << " B/nodo):  "
              << mb(static_cast<double>(v) * ColaFibonacci::bytesPorNodo()) << "\n"
              << "  Temporal del generador (se libera):     " << mb(gen) << "\n"
              << "  Pico generacion (CSR + claves + cursor): " << mb(picoGen) << "\n"
              << "  Pico Prim binomial  (CSR + cola + aux + T):  " << mb(picoPrimBin) << "\n"
              << "  Pico Prim Fibonacci (CSR + cola + aux + T):  " << mb(picoPrimFib) << "\n";
}

/**
 * Mide el costo promedio de una llamada a steady_clock::now() (sirve para
 * descontar el costo del reloj en la medición de decreaseKey, series C y D).
 * Entrada: cantidad de llamadas. Salida: imprime el promedio en ns y la resolución.
 */
static void calibrarReloj(int64_t llamadas) {
    using Reloj = std::chrono::steady_clock;
    int64_t suma = 0;  // se usa el resultado para que el compilador no elimine las llamadas
    const auto t0 = Reloj::now();
    for (int64_t k = 0; k < llamadas; ++k) suma += Reloj::now().time_since_epoch().count();
    const auto t1 = Reloj::now();
    static volatile int64_t sumidero;
    sumidero = suma;
    (void)sumidero;  // evita -Wunused-but-set-variable; la escritura volátil se conserva
    const double totalNs = std::chrono::duration<double, std::nano>(t1 - t0).count();
    std::cout << "Calibracion de steady_clock::now() (" << llamadas << " llamadas)\n"
              << "  Tiempo total:       " << std::fixed << std::setprecision(3) << totalNs / 1e6 << " ms\n"
              << "  Costo por llamada:  " << std::setprecision(2) << totalNs / static_cast<double>(llamadas) << " ns\n"
              << "  Resolucion nominal: " << 1e9 * Reloj::period::num / Reloj::period::den << " ns\n";
}

/**
 * Ejecuta la batería de experimentos (o solo --memoria / --calibrar).
 * Entrada: opciones de línea de comandos (ver el encabezado de este archivo o --ayuda).
 * Salida: CSV en la carpeta --salida y progreso por stderr; código de salida 0, 1 o 2
 * (ver el encabezado).
 */
int main(int argc, char** argv) {
    std::string series = "ABCD";
    std::string colasArg = "binomial,fibonacci";
    std::string salida = "resultados";
    int reps = 10, reducir = 0;
    int64_t cadaK = 0;  // 0 = automático: se guarda cada llamada y se reduce a PUNTOS_CURVA
    uint32_t base = 20260928;
    bool soloMemoria = false, soloCalibrar = false;

    for (int a = 1; a < argc; ++a) {
        std::string s = argv[a];
        auto sig = [&]() -> std::string {
            if (a + 1 >= argc) { std::cerr << "Falta valor para " << s << "\n"; std::exit(1); }
            return argv[++a];
        };
        if (s == "--series") series = sig();
        else if (s == "--reps") reps = std::stoi(sig());
        else if (s == "--colas") colasArg = sig();
        else if (s == "--salida") salida = sig();
        else if (s == "--reducir") reducir = std::stoi(sig());
        else if (s == "--cada") cadaK = std::stoll(sig());
        else if (s == "--semilla") base = static_cast<uint32_t>(std::stoul(sig()));
        else if (s == "--memoria") soloMemoria = true;
        else if (s == "--calibrar") soloCalibrar = true;
        else {
            std::cerr << "Uso: prim [--series ABCD] [--reps N] [--colas binomial,fibonacci,falsa]\n"
                         "            [--salida dir] [--reducir k] [--cada K (0=auto)] [--semilla S]\n"
                         "            [--memoria] [--calibrar]\n";
            return s == "--ayuda" || s == "-h" ? 0 : 1;
        }
    }

    if (soloMemoria) {
        std::cout << "Estimacion de memoria (seccion 6.2)\n";
        imprimirMemoria(int64_t(1) << 15, int64_t(1) << 20);
        imprimirMemoria(int64_t(1) << 22, int64_t(1) << 24);  // caso más grande de la serie B
        return 0;
    }
    if (soloCalibrar) {
        calibrarReloj(1000000);
        return 0;
    }

    std::vector<std::string> colas;
    {
        std::stringstream ss(colasArg);
        std::string c;
        while (std::getline(ss, c, ',')) {
            if (!implementada(c)) {
                std::cerr << "AVISO: la cola '" << c << "' aun no esta implementada; se omite.\n";
                continue;
            }
            colas.push_back(c);
        }
    }
    if (colas.empty()) { std::cerr << "No hay colas para ejecutar.\n"; return 1; }

    std::filesystem::create_directories(salida);
    const std::string suf = series + (reducir ? "_red" + std::to_string(reducir) : "");
    std::ofstream fT(salida + "/tiempos_" + suf + ".csv");
    std::ofstream fC(salida + "/curvas_" + suf + ".csv");
    std::ofstream fV(salida + "/verificacion_" + suf + ".csv");
    fT << "serie,i,j,v,e,rep,semilla,cola,tiempo_ms,peso_mst,dk_llamadas,dk_tiempo_ns,dk_ops,dk_ops_cascada\n";
    fC << "serie,i,j,rep,cola,llamadas,tiempo_acum_ns,ops_acum\n";
    fV << "serie,i,j,rep,cola_ref,peso_ref,cola,peso,diferencia,ok\n";
    fT << std::setprecision(12);
    fV << std::setprecision(12);

    int fallas = 0;
    const auto inicio = std::chrono::steady_clock::now();
    for (const Config& c0 : configuraciones(series)) {
        Config c = c0;
        c.i -= reducir;
        c.j -= reducir;
        const int v = 1 << c.i;
        const int64_t e = int64_t(1) << c.j;
        const bool amortizado = (c.serie == 'C' || c.serie == 'D');
        if (e > int64_t(v) * (v - 1) / 2) {
            std::cerr << "AVISO: " << c.serie << " i=" << c.i << " j=" << c.j
                      << " no es posible (e > v(v-1)/2); se omite. Use un --reducir menor.\n";
            continue;
        }
        // Curva (solo en la ejecución extra): con --cada K se guarda un punto cada K
        // llamadas y se escribe tal cual; en automático se guarda cada llamada y se
        // reduce al escribir.
        const int64_t k = cadaK > 0 ? cadaK : 1;

        for (int rep = 0; rep < reps; ++rep) {
            const uint32_t sem = semilla(base, c0, rep);
            Grafo g = generarGrafo(v, e, sem);  // fuera de la medición

            double pesoRef = 0.0;
            // Se alterna el orden de las colas entre repeticiones para no favorecer
            // a ninguna con efectos de caché / frecuencia de CPU.
            for (size_t q = 0; q < colas.size(); ++q) {
                const std::string& cola = colas[(q + rep) % colas.size()];
                // Repeticiones medidas: en C y D se mide cada decreaseKey, pero SIN curva (cadaK = 0).
                ResultadoPrim r = ejecutar(cola, g, amortizado, 0);

                fT << c.serie << ',' << c.i << ',' << c.j << ',' << v << ',' << e << ',' << rep << ','
                   << sem << ',' << cola << ',' << r.tiempoMs << ',' << r.pesoTotal << ','
                   << r.dkLlamadas << ',' << r.dkTiempoNs << ',' << r.dkOps << ',' << r.dkOpsCascada << '\n';

                if (q == 0) {
                    pesoRef = r.pesoTotal;
                } else {
                    const double dif = std::fabs(r.pesoTotal - pesoRef);
                    const bool ok = dif <= 1e-9 * std::max(1.0, pesoRef);
                    if (!ok) ++fallas;
                    fV << c.serie << ',' << c.i << ',' << c.j << ',' << rep << ','
                       << colas[rep % colas.size()] << ',' << pesoRef << ',' << cola << ','
                       << r.pesoTotal << ',' << dif << ',' << (ok ? 1 : 0) << '\n';
                }
                std::cerr << "[" << c.serie << " i=" << c.i << " j=" << c.j << " rep=" << rep << "] "
                          << std::setw(9) << cola << "  " << std::fixed << std::setprecision(2)
                          << r.tiempoMs << " ms  peso=" << std::setprecision(6) << r.pesoTotal
                          << "  dk=" << r.dkLlamadas << '\n';
            }
            // Curva de decreaseKey (series C y D): ejecución extra sobre el grafo de la
            // repetición 0. Va solo a curvas_*.csv; no se escribe en tiempos_*.csv.
            if (amortizado && rep == 0)
                for (const std::string& cola : colas) {
                    ResultadoPrim rc = ejecutar(cola, g, true, k);
                    for (const PuntoCurva& p : cadaK > 0 ? rc.curva : reducirCurva(rc.curva, PUNTOS_CURVA))
                        fC << c.serie << ',' << c.i << ',' << c.j << ',' << rep << ',' << cola << ','
                           << p.llamadas << ',' << p.tiempoNs << ',' << p.ops << '\n';
                }
            fT.flush(); fC.flush(); fV.flush();  // si se corta, lo ya medido queda en disco
        }
    }
    const double seg = std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count();
    std::cerr << "Listo en " << seg << " s. Resultados en " << salida << "/ (*_" << suf << ".csv)\n";
    if (fallas) std::cerr << "ATENCION: " << fallas << " MST con pesos distintos entre colas.\n";
    return fallas ? 2 : 0;
}
