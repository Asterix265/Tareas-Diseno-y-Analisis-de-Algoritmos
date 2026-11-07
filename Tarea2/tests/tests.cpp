#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>
#include <stdexcept>
#include <vector>

#include "arbol_b.h"
#include "arbol_be.h"

/** Compara una configuración Bε con un multimap, incluidas llaves duplicadas. */
template <class Arbol> static void probarBe() {
    Arbol arbol(0.5);
    std::multimap<std::int32_t, float> referencia;
    std::mt19937 gen(2026);
    for (int i = 0; i < 30000; ++i) {
        Par par{static_cast<std::int32_t>(gen() % 2000), static_cast<float>(i)};
        arbol.insertar(par);
        referencia.emplace(par.llave, par.valor);
    }
    arbol.guardar("prueba_arbol_be.bin");
    ArchivoNodos<typename Arbol::Nodo> archivo("prueba_arbol_be.bin");
    assert(Arbol::contarPares(archivo) == 30000);
    for (int i = 0; i < 100; ++i) {
        const std::int32_t l = static_cast<std::int32_t>(gen() % 2000);
        const std::int32_t u = l + static_cast<std::int32_t>(gen() % 200);
        auto obtenido = Arbol::buscarRango(archivo, l, u);
        std::vector<Par> esperado;
        for (auto it = referencia.lower_bound(l); it != referencia.end() && it->first <= u; ++it)
            esperado.push_back({it->first, it->second});
        const auto ordenar = [](const Par& a, const Par& b) {
            return a.llave < b.llave || (a.llave == b.llave && a.valor < b.valor);
        };
        std::sort(obtenido.begin(), obtenido.end(), ordenar);
        std::sort(esperado.begin(), esperado.end(), ordenar);
        assert(obtenido.size() == esperado.size());
        for (std::size_t j = 0; j < obtenido.size(); ++j) {
            assert(obtenido[j].llave == esperado[j].llave);
            assert(obtenido[j].valor == esperado[j].valor);
        }
    }
}

/** Verifica divisiones internas cuando las llaves llegan en orden creciente. */
template <class Arbol> static void probarBeOrdenado() {
    Arbol arbol(0.8);
    for (int i = 0; i < 20000; ++i) arbol.insertar({i, static_cast<float>(i)});
    arbol.guardar("prueba_arbol_be.bin");
    ArchivoNodos<typename Arbol::Nodo> archivo("prueba_arbol_be.bin");
    assert(Arbol::contarPares(archivo) == 20000);
    for (int l = 0; l < 20000; l += 997) {
        const int u = std::min(l + 300, 19999);
        const auto pares = Arbol::buscarRango(archivo, l, u);
        assert(pares.size() == static_cast<std::size_t>(u - l + 1));
    }
}

/** Verifica los bloques, el lector y el árbol B contra una referencia en memoria. */
int main() {
    assert(verificarMuestra("datos.bin", "datos_sample.txt") == 50);
    ArbolB arbol;
    std::multimap<std::int32_t, float> referencia;
    std::mt19937 gen(4102);
    for (int i = 0; i < 10000; ++i) {
        Par par{static_cast<std::int32_t>(gen() % 2500), static_cast<float>(i)};
        arbol.insertar(par);
        referencia.emplace(par.llave, par.valor);
    }
    assert(arbol.cantidadNodos() > 2);
    assert(arbol.lecturas() > 0 && arbol.escrituras() > 0);
    arbol.guardar("prueba_arbol_b.bin");
    ArchivoNodos<NodoB> archivo("prueba_arbol_b.bin");
    for (int i = 0; i < 100; ++i) {
        const std::int32_t l = static_cast<std::int32_t>(gen() % 2500);
        const std::int32_t u = l + static_cast<std::int32_t>(gen() % 300);
        auto obtenido = ArbolB::buscarRango(archivo, l, u);
        std::vector<Par> esperado;
        for (auto it = referencia.lower_bound(l); it != referencia.end() && it->first <= u; ++it)
            esperado.push_back({it->first, it->second});
        const auto ordenar = [](const Par& a, const Par& b) {
            return a.llave < b.llave || (a.llave == b.llave && a.valor < b.valor);
        };
        std::sort(obtenido.begin(), obtenido.end(), ordenar);
        std::sort(esperado.begin(), esperado.end(), ordenar);
        assert(obtenido.size() == esperado.size());
        for (std::size_t j = 0; j < obtenido.size(); ++j) {
            assert(obtenido[j].llave == esperado[j].llave);
            assert(obtenido[j].valor == esperado[j].valor);
        }
    }
    assert(archivo.lecturas >= 100);

    Datos datos("datos.bin");
    ArbolB real;
    std::vector<Par> primeros;
    primeros.reserve(1u << 15);
    for (int i = 0; i < (1 << 15); ++i) {
        Par par{};
        assert(datos.siguiente(par));
        primeros.push_back(par);
        real.insertar(par);
    }
    real.guardar("prueba_arbol_b.bin");
    ArchivoNodos<NodoB> archivoReal("prueba_arbol_b.bin");
    for (int i = 0; i < 10; ++i) {
        const std::int32_t l = 1546300800 + i * 20000000;
        const std::int32_t u = l + 604800;
        const auto hallados = ArbolB::buscarRango(archivoReal, l, u);
        const auto cuenta = std::count_if(primeros.begin(), primeros.end(), [=](Par par) {
            return l <= par.llave && par.llave <= u;
        });
        assert(hallados.size() == static_cast<std::size_t>(cuenta));
    }
    probarBe<ArbolBe<4, 506>>();
    probarBe<ArbolBe<126, 384>>();
    probarBeOrdenado<ArbolBe<4, 506>>();
    probarBeOrdenado<ArbolBe<126, 384>>();
}
