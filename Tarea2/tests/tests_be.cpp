// Pruebas del árbol Bε: invariantes del árbol serializado y búsquedas contra una referencia.
// Uso: ./tests_be [ruta de datos.bin]  (sin datos.bin se omiten los casos con datos reales)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include "arbol_b.h"
#include "arbol_be.h"

namespace {

int fallas = 0;
const char* const kArchivo = "prueba_arbol_be.bin";

/** Entrada: condición y mensaje. Salida: cuenta e imprime una falla si la condición es falsa. */
void comprobar(bool condicion, const std::string& mensaje) {
    if (condicion) return;
    ++fallas;
    if (fallas <= 30) std::printf("  FALLA: %s\n", mensaje.c_str());
}

/** Entrada: dos pares. Salida: true si a va antes que b en el orden (llave, valor). */
bool menorPar(const Par& a, const Par& b) {
    return a.llave < b.llave || (a.llave == b.llave && a.valor < b.valor);
}

/** Entrada: dos vectores de pares. Salida: true si son el mismo multiconjunto. */
bool mismoMulticonjunto(std::vector<Par> a, std::vector<Par> b) {
    if (a.size() != b.size()) return false;
    std::sort(a.begin(), a.end(), menorPar);
    std::sort(b.begin(), b.end(), menorPar);
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].llave != b[i].llave || a[i].valor != b[i].valor) return false;
    return true;
}

/** Estado acumulado al recorrer el árbol serializado. */
struct Recorrido {
    std::vector<int> visitas;     // veces que se alcanzó cada bloque
    std::vector<Par> pares;       // todos los pares guardados (buffers y hojas)
    int profundidadHojas = -1;    // profundidad común de las hojas
    std::size_t nodos = 0;        // nodos alcanzados desde la raíz
};

/**
 * Entrada: archivo, bloque, profundidad y cotas inclusivas de llaves del subárbol.
 * Salida: revisa orden, capacidades, cotas y profundidad de hojas; junta los pares en r.
 */
template <int C, int L>
void revisar(ArchivoNodos<NodoBe<C, L>>& archivo, std::size_t indice, int profundidad,
             long long cotaInf, long long cotaSup, Recorrido& r) {
    if (indice >= r.visitas.size()) { comprobar(false, "hijo fuera del archivo"); return; }
    if (++r.visitas[indice] > 1) { comprobar(false, "bloque alcanzado dos veces"); return; }
    const NodoBe<C, L> n = archivo.leer(indice);
    ++r.nodos;
    const std::string donde = "bloque " + std::to_string(indice) + ": ";
    comprobar(n.k_buffer >= 0 && n.k_buffer <= L, donde + "k_buffer fuera de [0, l_max]");
    for (int i = 0; i < n.k_buffer; ++i) {
        const auto k = n.buffer[i].llave;
        comprobar(k >= cotaInf && k <= cotaSup, donde + "par del buffer fuera del rango del subárbol");
        if (i > 0) comprobar(n.buffer[i - 1].llave <= k, donde + "buffer desordenado");
        r.pares.push_back(n.buffer[i]);
    }
    if (!n.es_interno) {
        comprobar(n.k_llaves == 0, donde + "hoja con llaves divisorias");
        if (r.profundidadHojas < 0) r.profundidadHojas = profundidad;
        comprobar(r.profundidadHojas == profundidad, donde + "hojas a distinta profundidad");
        return;
    }
    // Un nodo interno en reposo tiene entre 1 y c − 1 llaves: al llegar a c se divide.
    comprobar(n.k_llaves >= 1 && n.k_llaves < C, donde + "k_llaves fuera de [1, c − 1]");
    for (int i = 0; i < n.k_llaves; ++i) {
        comprobar(n.llaves[i] >= cotaInf && n.llaves[i] <= cotaSup, donde + "divisoria fuera de rango");
        if (i > 0) comprobar(n.llaves[i - 1] <= n.llaves[i], donde + "divisorias desordenadas");
    }
    for (int i = 0; i <= n.k_llaves; ++i) {
        const long long inf = i == 0 ? cotaInf : n.llaves[i - 1];
        const long long sup = i == n.k_llaves ? cotaSup : n.llaves[i];
        revisar<C, L>(archivo, static_cast<std::size_t>(n.hijos[i]), profundidad + 1, inf, sup, r);
    }
}

/** Entrada: tamaño y semilla. Salida: rangos [l, u] variados dentro de [minimo, maximo]. */
std::vector<std::pair<std::int32_t, std::int32_t>> rangosAzar(std::int32_t minimo, std::int32_t maximo,
                                                             int cantidad, std::uint32_t semilla) {
    std::mt19937 gen(semilla);
    const long long ancho = static_cast<long long>(maximo) - minimo + 1;
    std::vector<std::pair<std::int32_t, std::int32_t>> rangos;
    for (int i = 0; i < cantidad; ++i) {
        const long long l = minimo - 2 + static_cast<long long>(gen() % static_cast<std::uint32_t>(ancho + 4));
        const long long w = static_cast<long long>(gen() % static_cast<std::uint32_t>(ancho / 20 + 2));
        rangos.emplace_back(static_cast<std::int32_t>(l), static_cast<std::int32_t>(l + w));
    }
    return rangos;
}

/**
 * Entrada: nombre del caso, α, pares y rangos de prueba.
 * Salida: construye el árbol, lo serializa, revisa invariantes y compara búsquedas con la referencia.
 */
template <class Arbol>
void probar(const std::string& nombre, double alfa, const std::vector<Par>& pares,
            std::vector<std::pair<std::int32_t, std::int32_t>> rangos) {
    constexpr int C = Arbol::c;
    constexpr int L = Arbol::l_max;
    const int fallasAntes = fallas;
    Arbol arbol(alfa);
    for (const Par& p : pares) arbol.insertar(p);
    arbol.guardar(kArchivo);
    ArchivoNodos<typename Arbol::Nodo> archivo(kArchivo);

    Recorrido r;
    r.visitas.assign(arbol.cantidadNodos(), 0);
    revisar<C, L>(archivo, 0, 0, -2147483648LL, 2147483647LL, r);
    comprobar(r.pares.size() == pares.size(), nombre + ": la suma de k_buffer no es N");
    comprobar(Arbol::contarPares(archivo) == pares.size(), nombre + ": contarPares() no es N");
    comprobar(r.nodos == arbol.cantidadNodos(), nombre + ": hay bloques no alcanzables desde la raíz");
    comprobar(r.profundidadHojas + 1 == arbol.altura(), nombre + ": altura() no coincide con las hojas");
    comprobar(mismoMulticonjunto(r.pares, pares), nombre + ": el árbol no guarda exactamente los pares insertados");

    std::vector<Par> referencia = pares;
    std::sort(referencia.begin(), referencia.end(), menorPar);
    // Consultas puntuales sobre llaves existentes y sobre las divisorias de la raíz.
    std::mt19937 gen(99);
    for (int i = 0; i < 20 && !pares.empty(); ++i) {
        const auto k = pares[gen() % pares.size()].llave;
        rangos.emplace_back(k, k);
    }
    const auto raiz = archivo.leer(0);
    for (int i = 0; i < raiz.k_llaves; ++i) {
        rangos.emplace_back(raiz.llaves[i], raiz.llaves[i]);
        rangos.emplace_back(raiz.llaves[i] - 1, raiz.llaves[i] + 1);
    }
    const std::uint64_t lecturasAntes = archivo.lecturas;
    std::size_t hallados = 0;
    for (const auto& [l, u] : rangos) {
        const auto desde = std::lower_bound(referencia.begin(), referencia.end(), l,
            [](const Par& p, std::int32_t k) { return p.llave < k; });
        const auto hasta = std::upper_bound(referencia.begin(), referencia.end(), u,
            [](std::int32_t k, const Par& p) { return k < p.llave; });
        const std::vector<Par> esperado(desde, hasta);
        const auto obtenido = Arbol::buscarRango(archivo, l, u);
        hallados += obtenido.size();
        comprobar(mismoMulticonjunto(obtenido, esperado),
                  nombre + ": búsqueda [" + std::to_string(l) + ", " + std::to_string(u) + "] obtuvo " +
                  std::to_string(obtenido.size()) + " pares, esperaba " + std::to_string(esperado.size()));
    }
    const double n = pares.empty() ? 1.0 : static_cast<double>(pares.size());
    const auto& e = arbol.estadisticas();
    std::printf("  %-34s N=%-7zu h=%-2d nodos=%-6zu I/O-ins=%5.2f I/O-busq=%6.1f "
                "flush=%llu splitH=%llu splitI=%llu parciales=%llu %s\n",
                nombre.c_str(), pares.size(), arbol.altura(), arbol.cantidadNodos(),
                static_cast<double>(arbol.lecturas() + arbol.escrituras()) / n,
                static_cast<double>(archivo.lecturas - lecturasAntes) / static_cast<double>(rangos.size()),
                static_cast<unsigned long long>(e.vaciados), static_cast<unsigned long long>(e.splitsHoja),
                static_cast<unsigned long long>(e.splitsInternos),
                static_cast<unsigned long long>(e.lotesParciales),
                fallas == fallasAntes ? "OK" : "FALLA");
    (void)hallados;
}

/** Entrada: ruta y cantidad. Salida: los primeros n pares de datos.bin, o vacío si no existe. */
std::vector<Par> leerReales(const std::string& ruta, std::size_t n) {
    std::ifstream existe(ruta, std::ios::binary);
    if (!existe) return {};
    Datos datos(ruta);
    std::vector<Par> v(n);
    for (Par& p : v)
        if (!datos.siguiente(p)) return {};
    return v;
}

/** Entrada: cantidad, cantidad de llaves distintas y semilla. Salida: pares con muchas llaves repetidas. */
std::vector<Par> repetidas(std::size_t n, std::uint32_t distintas, std::uint32_t semilla) {
    std::mt19937 gen(semilla);
    std::vector<Par> v(n);
    for (std::size_t i = 0; i < n; ++i)
        v[i] = {static_cast<std::int32_t>(gen() % distintas), static_cast<float>(i)};
    return v;
}

/** Entrada: cantidad y paso (positivo o negativo). Salida: llaves en orden monótono. */
std::vector<Par> monotonas(std::size_t n, int paso) {
    std::vector<Par> v(n);
    for (std::size_t i = 0; i < n; ++i)
        v[i] = {static_cast<std::int32_t>(paso > 0 ? i : n - i), static_cast<float>(i)};
    return v;
}

/** Entrada: cantidad. Salida: n pares con la misma llave (peor caso de repetidas). */
std::vector<Par> iguales(std::size_t n) {
    std::vector<Par> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = {42, static_cast<float>(i)};
    return v;
}

/** Entrada: ruta de datos.bin. Salida: corre todos los casos para una configuración del árbol. */
template <class Arbol>
void probarConfiguracion(const char* nombre, const std::vector<Par>& reales) {
    std::printf("== %s (c=%d, l_max=%d)\n", nombre, Arbol::c, Arbol::l_max);
    const std::int32_t kMin = 1546300800, kMax = 1754006400;
    for (double alfa : {0.1, 0.5, 0.9}) {
        const std::string a = " α=" + std::to_string(alfa).substr(0, 3);
        for (std::size_t n : {std::size_t{0}, std::size_t{1}, std::size_t(Arbol::l_max),
                              std::size_t(Arbol::l_max + 1), std::size_t(3 * Arbol::l_max)})
            probar<Arbol>("chico " + std::to_string(n) + a, alfa, repetidas(n, 1000000, 5),
                          rangosAzar(0, 1000000, 30, 1));
        probar<Arbol>("repetidas 200k" + a, alfa, repetidas(200000, 30000, 7), rangosAzar(0, 30000, 200, 2));
        probar<Arbol>("ascendentes 100k" + a, alfa, monotonas(100000, 1), rangosAzar(0, 100000, 100, 3));
        probar<Arbol>("descendentes 100k" + a, alfa, monotonas(100000, -1), rangosAzar(0, 100000, 100, 4));
        probar<Arbol>("iguales 20k" + a, alfa, iguales(20000), rangosAzar(40, 44, 20, 5));
        if (!reales.empty()) {
            // Rangos de una semana como en el experimento, más otros de ancho variado.
            std::vector<std::pair<std::int32_t, std::int32_t>> semanas;
            std::mt19937 gen(4102);
            for (int i = 0; i < 50; ++i) {
                const auto l = static_cast<std::int32_t>(kMin + gen() % static_cast<std::uint32_t>(kMax - kMin + 1));
                semanas.emplace_back(l, l + 604800);
            }
            auto todos = rangosAzar(kMin, kMax, 50, 6);
            todos.insert(todos.end(), semanas.begin(), semanas.end());
            probar<Arbol>("reales 2^18" + a, alfa, reales, todos);
        }
    }
}

/** Entrada: pares reales. Salida: compara las 50 búsquedas de una semana entre el árbol B y ambos Bε. */
void compararConArbolB(const std::vector<Par>& reales) {
    if (reales.empty()) return;
    std::printf("== Mismos resultados que el árbol B (2^18 pares reales, 50 semanas)\n");
    ArbolB b;
    ArbolBe025 be025(0.5);
    ArbolBe075 be075(0.5);
    for (const Par& p : reales) { b.insertar(p); be025.insertar(p); be075.insertar(p); }
    b.guardar("prueba_cmp_b.bin");
    be025.guardar("prueba_cmp_025.bin");
    be075.guardar("prueba_cmp_075.bin");
    ArchivoNodos<NodoB> fb("prueba_cmp_b.bin");
    ArchivoNodos<ArbolBe025::Nodo> f025("prueba_cmp_025.bin");
    ArchivoNodos<ArbolBe075::Nodo> f075("prueba_cmp_075.bin");
    std::mt19937 gen(4102);
    std::size_t total = 0;
    for (int i = 0; i < 50; ++i) {
        const auto l = static_cast<std::int32_t>(1546300800 + gen() % 207705601u);
        const auto u = l + 604800;
        const auto rb = ArbolB::buscarRango(fb, l, u);
        total += rb.size();
        comprobar(mismoMulticonjunto(rb, ArbolBe025::buscarRango(f025, l, u)), "B y Bε 0,25 difieren");
        comprobar(mismoMulticonjunto(rb, ArbolBe075::buscarRango(f075, l, u)), "B y Bε 0,75 difieren");
    }
    std::printf("  promedio %.1f pares por búsqueda (%.3f%% de N)\n", total / 50.0,
                100.0 * static_cast<double>(total) / 50.0 / static_cast<double>(reales.size()));
    std::remove("prueba_cmp_b.bin");
    std::remove("prueba_cmp_025.bin");
    std::remove("prueba_cmp_075.bin");
}

}  // namespace

/** Entrada: ruta opcional de datos.bin. Salida: 0 si todas las pruebas pasan, 1 si alguna falla. */
int main(int argc, char** argv) {
    const std::string ruta = argc > 1 ? argv[1] : "datos.bin";
    const auto reales = leerReales(ruta, std::size_t{1} << 18);
    if (reales.empty()) std::printf("(sin %s: se omiten los casos con datos reales)\n", ruta.c_str());
    bool alfaInvalido = false;
    try { ArbolBe025 malo(1.0); } catch (const std::invalid_argument&) { alfaInvalido = true; }
    comprobar(alfaInvalido, "α = 1 debería rechazarse");
    probarConfiguracion<ArbolBe025>("Bε ε≈0,25", reales);
    probarConfiguracion<ArbolBe075>("Bε ε≈0,75", reales);
    compararConArbolB(reales);
    std::remove(kArchivo);
    std::printf("\nRESULTADO: %d fallas\n", fallas);
    return fallas == 0 ? 0 : 1;
}
