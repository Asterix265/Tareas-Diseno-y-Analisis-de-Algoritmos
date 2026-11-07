#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "arbol_b.h"

namespace fs = std::filesystem;
using Reloj = std::chrono::steady_clock;

/** Opciones de la batería parcial del árbol B. */
struct Opciones {
    std::string datos = "datos.bin";
    std::string salida = "resultados";
    int desde = 15;
    int hasta = 26;
    int busquedas = 50;
    std::uint32_t semilla = 4102;
};

/** Entrada: argumentos de consola. Salida: opciones validadas. */
static Opciones leerOpciones(int argc, char** argv) {
    Opciones o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--ayuda") {
            std::cout << "Uso: ./experimento_b [--datos ruta] [--salida carpeta] "
                         "[--desde 15] [--hasta 26] [--busquedas 50] [--semilla 4102]\n";
            std::exit(0);
        }
        if (i + 1 >= argc) throw std::invalid_argument("Falta valor para " + arg);
        const std::string valor = argv[++i];
        if (arg == "--datos") o.datos = valor;
        else if (arg == "--salida") o.salida = valor;
        else if (arg == "--desde") o.desde = std::stoi(valor);
        else if (arg == "--hasta") o.hasta = std::stoi(valor);
        else if (arg == "--busquedas") o.busquedas = std::stoi(valor);
        else if (arg == "--semilla") o.semilla = static_cast<std::uint32_t>(std::stoul(valor));
        else throw std::invalid_argument("Opción desconocida: " + arg);
    }
    if (o.desde < 15 || o.hasta > 26 || o.desde > o.hasta || o.busquedas < 1)
        throw std::invalid_argument("Se requiere 15 <= desde <= hasta <= 26 y busquedas >= 1");
    return o;
}

/** Entrada: semilla y número de rangos. Salida: los mismos rangos para cada valor de N. */
static std::vector<std::pair<std::int32_t, std::int32_t>> generarRangos(
        std::uint32_t semilla, int cantidad) {
    std::mt19937 gen(semilla);
    std::uniform_int_distribution<std::int32_t> distribucion(1546300800, 1754006400);
    std::vector<std::pair<std::int32_t, std::int32_t>> rangos;
    rangos.reserve(static_cast<std::size_t>(cantidad));
    for (int i = 0; i < cantidad; ++i) {
        const auto inferior = distribucion(gen);
        rangos.emplace_back(inferior, inferior + 604800);
    }
    return rangos;
}

/** Entrada: opciones y exponente. Salida: fila CSV con métricas de B; guarda una curva de ejemplo. */
static void ejecutarB(const Opciones& o, int exponente, std::ofstream& metricas) {
    const std::uint64_t n = std::uint64_t{1} << exponente;
    Datos datos(o.datos);
    ArbolB arbol;
    const auto inicio = Reloj::now();
    for (std::uint64_t i = 0; i < n; ++i) {
        Par par{};
        if (!datos.siguiente(par)) throw std::runtime_error("datos.bin contiene menos de N pares");
        arbol.insertar(par);
    }
    const auto fin = Reloj::now();
    const double construccionMs = std::chrono::duration<double, std::milli>(fin - inicio).count();
    const fs::path rutaArbol = fs::path(o.salida) / ("arbol_b_" + std::to_string(exponente) + ".bin");
    arbol.guardar(rutaArbol.string());

    const auto rangos = generarRangos(o.semilla, o.busquedas);
    ArchivoNodos<NodoB> archivo(rutaArbol.string());
    double busquedaMs = 0.0;
    std::uint64_t hallados = 0;
    for (int i = 0; i < o.busquedas; ++i) {
        const auto [l, u] = rangos[static_cast<std::size_t>(i)];
        const auto antes = Reloj::now();
        auto pares = ArbolB::buscarRango(archivo, l, u);
        const auto despues = Reloj::now();
        busquedaMs += std::chrono::duration<double, std::milli>(despues - antes).count();
        hallados += pares.size();
        if (i == 0 && (exponente == o.desde || exponente == o.hasta)) {
            std::sort(pares.begin(), pares.end(), [](const Par& a, const Par& b) {
                return a.llave < b.llave || (a.llave == b.llave && a.valor < b.valor);
            });
            const fs::path curva = fs::path(o.salida) / ("busqueda_b_" + std::to_string(exponente) + ".csv");
            std::ofstream salida(curva);
            if (!salida) throw std::runtime_error("No se pudo crear " + curva.string());
            salida << "llave,temperatura\n";
            for (const Par& par : pares) salida << par.llave << ',' << std::setprecision(9) << par.valor << '\n';
        }
    }
    const double cantidad = static_cast<double>(o.busquedas);
    metricas << "B," << exponente << ',' << n << ',' << construccionMs << ','
             << arbol.lecturas() << ',' << arbol.escrituras() << ','
             << arbol.lecturas() + arbol.escrituras() << ',' << arbol.cantidadNodos() << ','
             << arbol.tamanoBytes() << ',' << busquedaMs / cantidad << ','
             << static_cast<double>(archivo.lecturas) / cantidad << ','
             << static_cast<double>(hallados) / cantidad << '\n';
    metricas.flush();
    fs::remove(rutaArbol);
    std::cerr << "B N=2^" << exponente << ": " << construccionMs << " ms, "
              << arbol.cantidadNodos() << " nodos\n";
}

/** Entrada: opciones de línea de comandos. Salida: CSV con la batería del árbol B. */
int main(int argc, char** argv) {
    try {
        const Opciones o = leerOpciones(argc, argv);
        fs::create_directories(o.salida);
        const fs::path ruta = fs::path(o.salida) / "metricas_b.csv";
        std::ofstream metricas(ruta);
        if (!metricas) throw std::runtime_error("No se pudo crear " + ruta.string());
        metricas << "arbol,exponente,N,construccion_ms,lecturas_construccion,"
                    "escrituras_construccion,ios_construccion,nodos,bytes,"
                    "busqueda_promedio_ms,ios_busqueda_promedio,hallados_promedio\n";
        metricas << std::fixed << std::setprecision(6);
        const fs::path rutaRangos = fs::path(o.salida) / "rangos.csv";
        std::ofstream archivoRangos(rutaRangos);
        if (!archivoRangos) throw std::runtime_error("No se pudo crear " + rutaRangos.string());
        archivoRangos << "busqueda,inferior,superior\n";
        const auto rangos = generarRangos(o.semilla, o.busquedas);
        for (std::size_t i = 0; i < rangos.size(); ++i)
            archivoRangos << i << ',' << rangos[i].first << ',' << rangos[i].second << '\n';
        for (int e = o.desde; e <= o.hasta; ++e) ejecutarB(o, e, metricas);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
