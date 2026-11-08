#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "datos.h"
#include "disco.h"

/** Bloque del árbol B: 4 + 4 + 340*8 + 341*4 + 4 = 4096 bytes. */
struct NodoB {
    std::int32_t es_interno = 0;
    std::int32_t k = 0;
    Par llaves_valores[340]{};
    std::int32_t hijos[341]{};
    std::int32_t padding = 0;
};
static_assert(sizeof(NodoB) == 4096, "NodoB debe ocupar 4096 bytes");

/** Árbol B con construcción sobre Disco y raíz fija en el bloque 0. */
class ArbolB {
public:
    static constexpr int b = 340;
    using Nodo = NodoB;

    /** Salida: árbol con una hoja vacía en el bloque 0. */
    ArbolB() { disco_.agregar(NodoB{}); }

    /** Entrada: par llave-valor. Salida: árbol actualizado; cada nivel se lee una sola vez. */
    void insertar(Par par) {
        NodoB raiz = disco_.leer(0);
        if (raiz.k < b) {
            insertarEn(0, raiz, par);
            return;
        }
        Division division = dividir(raiz);
        const auto izq = disco_.agregar(division.izquierdo);
        const auto der = disco_.agregar(division.derecho);
        NodoB nueva{};
        nueva.es_interno = 1;
        nueva.k = 1;
        nueva.llaves_valores[0] = division.mediano;
        nueva.hijos[0] = static_cast<std::int32_t>(izq);
        nueva.hijos[1] = static_cast<std::int32_t>(der);
        disco_.escribir(0, nueva);
        // Paso h) de 3.4.2: las mitades ya están en memoria, así que no se vuelven a leer.
        if (par.llave <= division.mediano.llave) insertarEn(izq, division.izquierdo, par);
        else insertarEn(der, division.derecho, par);
    }

    /** Entrada: ruta. Salida: árbol serializado, un nodo por bloque. */
    void guardar(const std::string& ruta) const { disco_.guardar(ruta); }
    /** Salida: número de nodos existentes. */
    std::size_t cantidadNodos() const { return disco_.cantidadNodos(); }
    /** Salida: tamaño del árbol serializado en bytes. */
    std::uint64_t tamanoBytes() const { return disco_.tamanoBytes(); }
    /** Salida: lecturas simuladas durante la construcción. */
    std::uint64_t lecturas() const { return disco_.lecturas; }
    /** Salida: escrituras simuladas durante la construcción. */
    std::uint64_t escrituras() const { return disco_.escrituras; }

    /** Entrada: archivo serializado y rango inclusivo. Salida: todos los pares del rango. */
    static std::vector<Par> buscarRango(ArchivoNodos<NodoB>& archivo,
                                         std::int32_t inferior, std::int32_t superior) {
        if (inferior > superior) throw std::invalid_argument("Rango invertido");
        std::vector<Par> resultado;
        buscarEn(archivo, 0, inferior, superior, resultado);
        return resultado;
    }

private:
    /** Salida de dividir un nodo lleno: dos mitades y par que sube al padre. */
    struct Division { NodoB izquierdo; NodoB derecho; Par mediano; };

    /** Entrada: nodo con b pares. Salida: mitades de 169 y 170 pares. */
    static Division dividir(const NodoB& lleno) {
        if (lleno.k != b) throw std::logic_error("Se intentó dividir un nodo no lleno");
        Division d{};
        d.izquierdo.es_interno = d.derecho.es_interno = lleno.es_interno;
        constexpr int medio = b / 2 - 1;
        d.izquierdo.k = medio;
        d.derecho.k = b - medio - 1;
        std::copy_n(lleno.llaves_valores, medio, d.izquierdo.llaves_valores);
        d.mediano = lleno.llaves_valores[medio];
        std::copy_n(lleno.llaves_valores + medio + 1, d.derecho.k, d.derecho.llaves_valores);
        if (lleno.es_interno) {
            std::copy_n(lleno.hijos, medio + 1, d.izquierdo.hijos);
            std::copy_n(lleno.hijos + medio + 1, d.derecho.k + 1, d.derecho.hijos);
        }
        return d;
    }

    /**
     * Entrada: bloque, su nodo ya leído (no lleno) y par. Salida: inserción recursiva con split
     * anticipado. Quien llama entrega el nodo en memoria, así que cada nivel cuenta una sola lectura.
     */
    void insertarEn(std::size_t indice, NodoB& nodo, Par par) {
        int i = 0;
        while (i < nodo.k && nodo.llaves_valores[i].llave < par.llave) ++i;
        if (!nodo.es_interno) {
            for (int j = nodo.k; j > i; --j) nodo.llaves_valores[j] = nodo.llaves_valores[j - 1];
            nodo.llaves_valores[i] = par;
            ++nodo.k;
            disco_.escribir(indice, nodo);
            return;
        }
        std::size_t hijo = static_cast<std::size_t>(nodo.hijos[i]);
        NodoB actual = disco_.leer(hijo);
        if (actual.k == b) {
            const Division d = dividir(actual);
            disco_.escribir(hijo, d.izquierdo);
            const auto derecho = disco_.agregar(d.derecho);
            for (int j = nodo.k; j > i; --j) nodo.llaves_valores[j] = nodo.llaves_valores[j - 1];
            for (int j = nodo.k + 1; j > i + 1; --j) nodo.hijos[j] = nodo.hijos[j - 1];
            nodo.llaves_valores[i] = d.mediano;
            nodo.hijos[i + 1] = static_cast<std::int32_t>(derecho);
            ++nodo.k;
            disco_.escribir(indice, nodo);
            if (par.llave > d.mediano.llave) {
                hijo = derecho;
                actual = d.derecho;
            } else {
                actual = d.izquierdo;
            }
        }
        insertarEn(hijo, actual, par);
    }

    /** Entrada: bloque, rango inclusivo y resultado. Salida: agrega pares hallados. */
    static void buscarEn(ArchivoNodos<NodoB>& archivo, std::size_t indice,
                         std::int32_t inferior, std::int32_t superior,
                         std::vector<Par>& resultado) {
        const NodoB nodo = archivo.leer(indice);
        for (int i = 0; i < nodo.k; ++i)
            if (inferior <= nodo.llaves_valores[i].llave && nodo.llaves_valores[i].llave <= superior)
                resultado.push_back(nodo.llaves_valores[i]);
        if (!nodo.es_interno) return;
        for (int i = 0; i <= nodo.k; ++i) {
            // La igualdad a la izquierda admite llaves repetidas a ambos lados de un split.
            const bool tocaIzquierda = i == 0 || superior >= nodo.llaves_valores[i - 1].llave;
            const bool tocaDerecha = i == nodo.k || inferior <= nodo.llaves_valores[i].llave;
            if (tocaIzquierda && tocaDerecha)
                buscarEn(archivo, static_cast<std::size_t>(nodo.hijos[i]),
                         inferior, superior, resultado);
        }
    }

    Disco<NodoB> disco_;
};
