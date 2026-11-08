#pragma once
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * disco.h — Uso de disco de la sección 3.3 del enunciado.
 *
 * Disco<Nodo>: durante la construcción, el árbol es un arreglo de nodos en RAM cuyo primer
 * elemento es la raíz. Cada acceso de lectura al arreglo cuenta una lectura y cada escritura
 * (modificar una posición o agregar al final) cuenta una escritura. Al terminar, guardar()
 * escribe los nodos de forma secuencial a un archivo binario, un bloque de 4096 bytes por nodo.
 *
 * ArchivoNodos<Nodo>: durante las búsquedas, el nodo i se lee desde el archivo en la posición
 * i · 4096 y cada lectura cuenta un I/O.
 *
 * Ningún árbol toca el arreglo ni el archivo directamente: todo pasa por estas dos clases, así
 * que los I/Os se cuentan en un solo lugar y de la misma forma para los tres árboles.
 */

/** Simula un disco de bloques durante la construcción del árbol. */
template <class Nodo> class Disco {
    static_assert(sizeof(Nodo) == 4096, "Cada nodo debe ocupar un bloque de 4096 bytes");
public:
    /** Entrada: índice válido. Salida: copia del nodo; cuenta una lectura. */
    Nodo leer(std::size_t i) {
        if (i >= nodos_.size()) throw std::out_of_range("Índice de bloque inválido");
        ++lecturas;
        return nodos_[i];
    }

    /** Entrada: índice válido y nodo. Salida: ninguna; cuenta una escritura. */
    void escribir(std::size_t i, const Nodo& nodo) {
        if (i >= nodos_.size()) throw std::out_of_range("Índice de bloque inválido");
        nodos_[i] = nodo;
        ++escrituras;
    }

    /** Entrada: nodo nuevo. Salida: índice asignado; cuenta una escritura. */
    std::size_t agregar(const Nodo& nodo) {
        nodos_.push_back(nodo);
        ++escrituras;
        return nodos_.size() - 1;
    }

    /** Entrada: ruta. Salida: archivo con los bloques consecutivos; no altera los I/Os de inserción. */
    void guardar(const std::string& ruta) const {
        std::ofstream salida(ruta, std::ios::binary | std::ios::trunc);
        if (!salida) throw std::runtime_error("No se pudo crear " + ruta);
        for (const Nodo& nodo : nodos_)
            salida.write(reinterpret_cast<const char*>(&nodo), sizeof(Nodo));
        if (!salida) throw std::runtime_error("Error al escribir " + ruta);
    }

    /** Salida: número de bloques creados. */
    std::size_t cantidadNodos() const { return nodos_.size(); }
    /** Salida: bytes que ocupará el árbol serializado. */
    std::uint64_t tamanoBytes() const { return nodos_.size() * sizeof(Nodo); }

    std::uint64_t lecturas = 0;    // lecturas simuladas durante la construcción
    std::uint64_t escrituras = 0;  // escrituras simuladas (incluye agregar al final)
private:
    std::vector<Nodo> nodos_;  // el "disco": nodos_[0] es siempre la raíz
};

/** Lee un árbol serializado por bloques, durante las búsquedas. */
template <class Nodo> class ArchivoNodos {
    static_assert(sizeof(Nodo) == 4096, "Cada nodo debe ocupar un bloque de 4096 bytes");
public:
    /** Entrada: ruta de un árbol guardado. Salida: lector abierto. */
    explicit ArchivoNodos(const std::string& ruta) : entrada_(ruta, std::ios::binary) {
        if (!entrada_) throw std::runtime_error("No se pudo abrir " + ruta);
    }

    /** Entrada: índice de bloque. Salida: nodo leído; cuenta una lectura. */
    Nodo leer(std::size_t i) {
        Nodo nodo{};
        entrada_.clear();
        entrada_.seekg(static_cast<std::streamoff>(i) * sizeof(Nodo));
        entrada_.read(reinterpret_cast<char*>(&nodo), sizeof(Nodo));
        if (!entrada_) throw std::runtime_error("No se pudo leer el bloque " + std::to_string(i));
        ++lecturas;
        return nodo;
    }

    std::uint64_t lecturas = 0;  // bloques leídos desde el archivo (I/Os de búsqueda)
private:
    std::ifstream entrada_;  // archivo binario del árbol, abierto en modo binario
};
