/**
 * main.cpp — Ejecuta la batería de experimentos de la sección 4 del enunciado.
 *
 * Para cada N = 2^desde, ..., 2^hasta inserta los primeros N pares de datos.bin en el árbol B,
 * en el Bε con ε ≈ 0,25 (c = 4, l_max = 506) y en el Bε con ε ≈ 0,75 (c = 126, l_max = 384).
 * Cada árbol se escribe a un archivo binario, un nodo por bloque, y las búsquedas por rango
 * leen los nodos desde ese archivo. Los tres árboles usan los mismos datos y los mismos rangos.
 *
 * Salida (carpeta --salida, por defecto resultados/):
 *   metricas.csv          una fila por (árbol, N): tiempo, lecturas y escrituras de construcción,
 *                         nodos, bytes, tiempo e I/Os promedio de búsqueda y pares hallados
 *   verificacion.csv      cada búsqueda de Bε comparada par por par con la del árbol B
 *   rangos.csv            los rangos [l, u] usados, iguales para todo árbol y todo N
 *   busqueda_b_<e>.csv    resultado de la primera búsqueda para el primer y el último N
 *   bonus.csv             solo con --bonus: Bε(126,384) con N = 2^bonus-exponente y α = 0,1..0,8
 *
 * Código de salida: 0 si todo terminó bien; 1 si los argumentos son inválidos, falta un archivo
 * o alguna búsqueda de Bε no coincide con la del árbol B.
 */
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "arbol_b.h"
#include "arbol_be.h"

namespace fs = std::filesystem;
using Reloj = std::chrono::steady_clock;

/** Opciones de la batería de los tres árboles. */
struct Opciones {
    std::string datos = "datos.bin";     // archivo binario con los pares llave-valor
    std::string salida = "resultados";   // carpeta donde se escriben los CSV
    int desde = 15;                      // primer exponente: N = 2^desde
    int hasta = 26;                      // último exponente: N = 2^hasta
    int busquedas = 50;                  // búsquedas por rango para cada N y cada árbol
    std::uint32_t semilla = 4102;        // semilla de mt19937 para generar los rangos
    double alfa = 0.5;                   // α del criterio de vaciado de ambos Bε
    bool bonus = false;                  // true: corre solo el bonus en vez de la batería
    int bonusExponente = 24;             // exponente de N para el bonus
};

/** Entrada: argumentos de consola. Salida: opciones validadas. */
static Opciones leerOpciones(int argc, char** argv) {
    Opciones o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--ayuda") {
            std::cout << "Uso: ./experimento [--datos ruta] [--salida carpeta] "
                         "[--desde 15] [--hasta 26] [--busquedas 50] "
                         "[--semilla 4102] [--alfa 0.5] [--bonus] "
                         "[--bonus-exponente 24]\n";
            std::exit(0);
        }
        if (arg == "--bonus") { o.bonus = true; continue; }
        if (i + 1 >= argc) throw std::invalid_argument("Falta valor para " + arg);
        const std::string valor = argv[++i];
        if (arg == "--datos") o.datos = valor;
        else if (arg == "--salida") o.salida = valor;
        else if (arg == "--desde") o.desde = std::stoi(valor);
        else if (arg == "--hasta") o.hasta = std::stoi(valor);
        else if (arg == "--busquedas") o.busquedas = std::stoi(valor);
        else if (arg == "--semilla") o.semilla = static_cast<std::uint32_t>(std::stoul(valor));
        else if (arg == "--alfa") o.alfa = std::stod(valor);
        else if (arg == "--bonus-exponente") o.bonusExponente = std::stoi(valor);
        else throw std::invalid_argument("Opción desconocida: " + arg);
    }
    if (o.desde < 15 || o.hasta > 26 || o.desde > o.hasta || o.busquedas < 1 ||
        !(o.alfa > 0.0 && o.alfa < 1.0) ||
        o.bonusExponente < 15 || o.bonusExponente > 24)
        throw std::invalid_argument("Se requiere 15 <= desde <= hasta <= 26, busquedas >= 1, 0 < alfa < 1 y 15 <= bonus-exponente <= 24");
    return o;
}

/**
 * Rangos de una semana de la sección 4: l uniforme en [1546300800, 1754006400] y u = l + 604800.
 * Entrada: semilla y cantidad. Salida: rangos reproducibles, iguales para todos los árboles y N.
 * Solo usa mt19937, así que la misma semilla da los mismos rangos en cualquier SO.
 */
static std::vector<std::pair<std::int32_t, std::int32_t>> generarRangos(
        std::uint32_t semilla, int cantidad) {
    std::mt19937 gen(semilla);
    constexpr std::uint64_t cantidadLlaves = 1754006400ull - 1546300800ull + 1;
    std::vector<std::pair<std::int32_t, std::int32_t>> rangos;
    rangos.reserve(static_cast<std::size_t>(cantidad));
    for (int i = 0; i < cantidad; ++i) {
        const std::uint64_t aleatorio = (static_cast<std::uint64_t>(gen()) << 32) | gen();
        const auto inferior = static_cast<std::int32_t>(1546300800ull + aleatorio % cantidadLlaves);
        rangos.emplace_back(inferior, inferior + 604800);
    }
    return rangos;
}

/** Entrada: ruta y N. Salida: los primeros N pares, fuera del tiempo de construcción. */
static std::vector<Par> cargarDatos(const std::string& ruta, std::uint64_t n) {
    Datos datos(ruta);
    std::vector<Par> pares;
    pares.reserve(static_cast<std::size_t>(n));
    for (std::uint64_t i = 0; i < n; ++i) {
        Par par{};
        if (!datos.siguiente(par)) throw std::runtime_error("datos.bin contiene menos de N pares");
        pares.push_back(par);
    }
    return pares;
}

/** Entrada: dos pares. Salida: orden total para comparar resultados de los tres árboles. */
static bool menorPar(const Par& a, const Par& b) {
    if (a.llave != b.llave) return a.llave < b.llave;
    std::uint32_t bitsA, bitsB;
    std::memcpy(&bitsA, &a.valor, sizeof(bitsA));
    std::memcpy(&bitsB, &b.valor, sizeof(bitsB));
    return bitsA < bitsB;
}

/** Entrada: resultados ordenados. Salida: true si contienen exactamente los mismos pares. */
static bool mismosPares(const std::vector<Par>& a, const std::vector<Par>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].llave != b[i].llave ||
            std::memcmp(&a[i].valor, &b[i].valor, sizeof(float)) != 0) return false;
    return true;
}

/**
 * Construye un árbol, lo guarda en disco y ejecuta las búsquedas; la misma plantilla sirve para
 * ArbolB y ArbolBe<C, L>, así que los tres árboles se miden con el mismo código.
 * Entrada:
 *   arbol         árbol vacío (B o Bε) recién creado.
 *   nombre        "B", "Be025" o "Be075"; se usa en los CSV y en el archivo del árbol.
 *   o, exponente  opciones de la corrida y exponente de N = 2^exponente.
 *   datos         los primeros N pares, ya cargados en memoria (fuera de la medición).
 *   rangos        rangos [l, u] de las búsquedas.
 *   referencia    resultados ordenados del árbol B para cada rango, o nullptr si arbol es el B.
 *   metricas, verificacion  flujos donde se escriben las filas de metricas.csv y verificacion.csv.
 * Salida: resultados ordenados de cada búsqueda (el árbol B los entrega como referencia).
 * Lanza runtime_error si alguna búsqueda difiere de la referencia.
 */
template <class Arbol> static std::vector<std::vector<Par>> ejecutarArbol(
        Arbol& arbol, const std::string& nombre, const Opciones& o, int exponente,
        const std::vector<Par>& datos,
        const std::vector<std::pair<std::int32_t, std::int32_t>>& rangos,
        const std::vector<std::vector<Par>>* referencia,
        std::ostream& metricas, std::ostream& verificacion) {
    // Construcción: solo las N inserciones quedan dentro del intervalo medido.
    const auto inicio = Reloj::now();
    for (const Par& par : datos) arbol.insertar(par);
    const auto fin = Reloj::now();
    const double construccionMs = std::chrono::duration<double, std::milli>(fin - inicio).count();
    // El arreglo de nodos se escribe secuencialmente a un binario, fuera de la medición (3.3).
    const fs::path rutaArbol = fs::path(o.salida) /
        ("arbol_" + nombre + "_" + std::to_string(exponente) + ".bin");
    arbol.guardar(rutaArbol.string());
    // Las búsquedas leen cada nodo desde el archivo; ArchivoNodos cuenta una lectura por bloque.
    ArchivoNodos<typename Arbol::Nodo> archivo(rutaArbol.string());
    std::vector<std::vector<Par>> resultados;
    resultados.reserve(rangos.size());
    double busquedaMs = 0.0;
    std::uint64_t hallados = 0;
    for (std::size_t i = 0; i < rangos.size(); ++i) {
        const auto [l, u] = rangos[i];
        const auto antes = Reloj::now();
        auto pares = Arbol::buscarRango(archivo, l, u);
        const auto despues = Reloj::now();
        busquedaMs += std::chrono::duration<double, std::milli>(despues - antes).count();
        hallados += pares.size();
        std::sort(pares.begin(), pares.end(), menorPar); // Fuera del tiempo de búsqueda.
        // Verificación cruzada: cada búsqueda de Bε debe devolver exactamente los pares del B.
        if (referencia) {
            const bool iguales = mismosPares(pares, (*referencia)[i]);
            verificacion << nombre << ',' << exponente << ',' << i << ',' << pares.size() << ','
                         << (*referencia)[i].size() << ',' << (iguales ? 1 : 0) << '\n';
            if (!iguales) throw std::runtime_error("Búsqueda distinta de B: " + nombre +
                                                  ", N=2^" + std::to_string(exponente) +
                                                  ", rango " + std::to_string(i));
        }
        // Primera búsqueda con el primer y el último N: datos del gráfico de temperatura.
        if (i == 0 && nombre == "B" && (exponente == o.desde || exponente == o.hasta)) {
            const fs::path curva = fs::path(o.salida) / ("busqueda_b_" + std::to_string(exponente) + ".csv");
            std::ofstream salida(curva);
            if (!salida) throw std::runtime_error("No se pudo crear " + curva.string());
            salida << "llave,temperatura\n";
            for (const Par& par : pares)
                salida << par.llave << ',' << std::setprecision(9) << par.valor << '\n';
        }
        resultados.push_back(std::move(pares));
    }
    // Fila de metricas.csv: los promedios de búsqueda se dividen por la cantidad de rangos.
    const double cantidad = static_cast<double>(rangos.size());
    metricas << nombre << ',' << exponente << ',' << datos.size() << ',';
    if (nombre != "B") metricas << o.alfa;
    metricas << ',' << construccionMs << ',' << arbol.lecturas() << ',' << arbol.escrituras() << ','
             << arbol.lecturas() + arbol.escrituras() << ',' << arbol.cantidadNodos() << ','
             << arbol.tamanoBytes() << ',' << busquedaMs / cantidad << ','
             << static_cast<double>(archivo.lecturas) / cantidad << ','
             << static_cast<double>(hallados) / cantidad << '\n';
    metricas.flush();
    verificacion.flush();
    fs::remove(rutaArbol);  // con N = 2^26 cada árbol ocupa cerca de 1 GB
    std::cerr << nombre << " N=2^" << exponente << ": " << construccionMs << " ms, "
              << arbol.cantidadNodos() << " nodos\n";
    return resultados;
}

/**
 * Ejecuta la batería de experimentos (o solo el bonus con --bonus).
 * Entrada: opciones de línea de comandos (ver --ayuda).
 * Salida: CSV en la carpeta --salida y progreso por stderr; código de salida 0 o 1.
 */
int main(int argc, char** argv) {
    try {
        const Opciones o = leerOpciones(argc, argv);
        fs::create_directories(o.salida);
        // Bonus: un mismo Bε con N = 2^bonusExponente, 20 búsquedas y α = 0,1, ..., 0,8.
        if (o.bonus) {
            const auto datos = cargarDatos(o.datos, std::uint64_t{1} << o.bonusExponente);
            const auto rangos = generarRangos(o.semilla, 20);
            std::ofstream bonus(fs::path(o.salida) / "bonus.csv");
            std::ostringstream verificacionBonus;
            if (!bonus) throw std::runtime_error("No se pudo crear el CSV del bonus");
            bonus << "arbol,exponente,N,alfa,construccion_ms,lecturas_construccion,"
                     "escrituras_construccion,ios_construccion,nodos,bytes,"
                     "busqueda_promedio_ms,ios_busqueda_promedio,hallados_promedio\n";
            bonus << std::fixed << std::setprecision(6);
            for (int decima = 1; decima <= 8; ++decima) {
                Opciones corrida = o;
                corrida.alfa = decima / 10.0;
                ArbolBe<126, 384> arbol(corrida.alfa);
                ejecutarArbol(arbol, "Be075", corrida, o.bonusExponente, datos, rangos, nullptr,
                              bonus, verificacionBonus);
            }
            return 0;
        }
        std::ofstream metricas(fs::path(o.salida) / "metricas.csv");
        std::ofstream verificacion(fs::path(o.salida) / "verificacion.csv");
        std::ofstream archivoRangos(fs::path(o.salida) / "rangos.csv");
        if (!metricas || !verificacion || !archivoRangos)
            throw std::runtime_error("No se pudieron crear los CSV de resultados");
        metricas << "arbol,exponente,N,alfa,construccion_ms,lecturas_construccion,"
                    "escrituras_construccion,ios_construccion,nodos,bytes,"
                    "busqueda_promedio_ms,ios_busqueda_promedio,hallados_promedio\n";
        metricas << std::fixed << std::setprecision(6);
        verificacion << "arbol,exponente,busqueda,hallados,referencia_B,iguales\n";
        archivoRangos << "busqueda,inferior,superior\n";
        const auto rangos = generarRangos(o.semilla, o.busquedas);
        for (std::size_t i = 0; i < rangos.size(); ++i)
            archivoRangos << i << ',' << rangos[i].first << ',' << rangos[i].second << '\n';
        // Batería principal: para cada N se cargan los datos una vez y se construye un árbol a la vez;
        // cada árbol se destruye al cerrar su bloque, así que nunca hay dos en memoria.
        for (int e = o.desde; e <= o.hasta; ++e) {
            const auto datos = cargarDatos(o.datos, std::uint64_t{1} << e);
            std::vector<std::vector<Par>> referencia;
            {
                // El árbol B va primero: sus resultados son la referencia de la verificación.
                ArbolB arbol;
                referencia = ejecutarArbol(arbol, "B", o, e, datos, rangos, nullptr, metricas, verificacion);
            }
            {
                ArbolBe<4, 506> arbol(o.alfa);
                ejecutarArbol(arbol, "Be025", o, e, datos, rangos, &referencia, metricas, verificacion);
            }
            {
                ArbolBe<126, 384> arbol(o.alfa);
                ejecutarArbol(arbol, "Be075", o, e, datos, rangos, &referencia, metricas, verificacion);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
