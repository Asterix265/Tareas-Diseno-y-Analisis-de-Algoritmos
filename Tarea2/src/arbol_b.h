#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "datos.h"
#include "disco.h"

/**
 * arbol_b.h — Árbol B de la sección 3.1 en memoria externa.
 *
 * Traducción del esqueleto del enunciado: split (3.4.1), inserción con split anticipado (3.4.2)
 * y búsqueda por rango (3.4.5). Cada bloque se lee y escribe solo a través de Disco (construcción)
 * o ArchivoNodos (búsqueda), que cuentan los I/Os. La raíz siempre ocupa el bloque 0.
 */

/** Bloque del árbol B: 4 + 4 + 340*8 + 341*4 + 4 = 4096 bytes. */
struct NodoB {
    std::int32_t es_interno = 0;  // 1 si es interno, 0 si es hoja
    std::int32_t k = 0;  // pares activos; en todo momento ⌈b/2⌉ − 1 <= k <= b (salvo la raíz)
    Par llaves_valores[340]{};  // pares ordenados ascendentemente por llave
    std::int32_t hijos[341]{};  // hijos[i] guarda llaves > llaves_valores[i-1] y <= llaves_valores[i]
    std::int32_t padding = 0;  // sin uso (fijado en 0), solo para completar 4096 bytes
};
static_assert(sizeof(NodoB) == 4096, "NodoB debe ocupar 4096 bytes");

/** Árbol B con construcción sobre Disco y raíz fija en el bloque 0. */
class ArbolB {
public:
    static constexpr int b = 340;  // máximo de pares por nodo
    using Nodo = NodoB;  // tipo de bloque, para ArchivoNodos<Arbol::Nodo> en main.cpp

    /** Salida: árbol con una hoja vacía en el bloque 0. */
    ArbolB() { disco_.agregar(NodoB{}); }

    /**
     * Inserción de un par desde la raíz (3.4.2, caso "nodo actual es la raíz").
     * Entrada: par llave-valor. Salida: árbol actualizado; cada nivel se lee una sola vez.
     */
    void insertar(Par par) {
        NodoB raiz = disco_.leer(0);
        // a) La raíz no está llena: se inserta como en cualquier nodo.
        if (raiz.k < b) {
            insertarEn(0, raiz, par);
            return;
        }
        // b) La raíz está llena. a) split; b) ambas mitades al final del arreglo.
        Division division = dividir(raiz);
        const auto izq = disco_.agregar(division.izquierdo);
        const auto der = disco_.agregar(division.derecho);
        // c)-f) Nueva raíz R con el par mediano y los índices de las dos mitades.
        NodoB nueva{};
        nueva.es_interno = 1;
        nueva.k = 1;
        nueva.llaves_valores[0] = division.mediano;
        nueva.hijos[0] = static_cast<std::int32_t>(izq);
        nueva.hijos[1] = static_cast<std::int32_t>(der);
        // g) R reemplaza a la raíz antigua en el bloque 0.
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

    /**
     * Split de un nodo (3.4.1).
     * Entrada: nodo con b pares. Salida: mitades de 169 y 170 pares y el par mediano.
     */
    static Division dividir(const NodoB& lleno) {
        if (lleno.k != b) throw std::logic_error("Se intentó dividir un nodo no lleno");
        Division d{};
        d.izquierdo.es_interno = d.derecho.es_interno = lleno.es_interno;
        // a) El par en la posición b/2 (contando desde 1), índice 169, es el mediano.
        constexpr int medio = b / 2 - 1;
        // b) b/2 − 1 pares a la izquierda del mediano y b/2 a la derecha.
        d.izquierdo.k = medio;
        d.derecho.k = b - medio - 1;
        std::copy_n(lleno.llaves_valores, medio, d.izquierdo.llaves_valores);
        d.mediano = lleno.llaves_valores[medio];
        std::copy_n(lleno.llaves_valores + medio + 1, d.derecho.k, d.derecho.llaves_valores);
        // c) Si es interno: los primeros b/2 hijos a la izquierda y los otros b/2 + 1 a la derecha.
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
        // i = primera posición cuya llave es >= la del par (hijos[i] admite llaves <= llaves_valores[i]).
        int i = 0;
        while (i < nodo.k && nodo.llaves_valores[i].llave < par.llave) ++i;
        // Nodo hoja: a) agregar el par en orden; b) actualizar k; c) escribir la hoja.
        if (!nodo.es_interno) {
            for (int j = nodo.k; j > i; --j) nodo.llaves_valores[j] = nodo.llaves_valores[j - 1];
            nodo.llaves_valores[i] = par;
            ++nodo.k;
            disco_.escribir(indice, nodo);
            return;
        }
        // Nodo interno: a) hijo C donde corresponde el par (una lectura).
        std::size_t hijo = static_cast<std::size_t>(nodo.hijos[i]);
        NodoB actual = disco_.leer(hijo);
        // c) C está lleno: split antes de bajar.
        if (actual.k == b) {
            const Division d = dividir(actual);
            // c) izq reemplaza a C en su índice y der va al final del arreglo.
            disco_.escribir(hijo, d.izquierdo);
            const auto derecho = disco_.agregar(d.derecho);
            // b), d), e) El mediano y el índice de der entran en orden al nodo actual.
            for (int j = nodo.k; j > i; --j) nodo.llaves_valores[j] = nodo.llaves_valores[j - 1];
            for (int j = nodo.k + 1; j > i + 1; --j) nodo.hijos[j] = nodo.hijos[j - 1];
            nodo.llaves_valores[i] = d.mediano;
            nodo.hijos[i + 1] = static_cast<std::int32_t>(derecho);
            ++nodo.k;
            // f) Se escribe el nodo actualizado.
            disco_.escribir(indice, nodo);
            // g) Se sigue por izq si la llave es <= al mediano, o por der si es mayor.
            if (par.llave > d.mediano.llave) {
                hijo = derecho;
                actual = d.derecho;
            } else {
                actual = d.izquierdo;
            }
        }
        // b) C no estaba lleno (o ya se dividió): inserción recursiva con el nodo en memoria.
        insertarEn(hijo, actual, par);
    }

    /**
     * Búsqueda por rango (3.4.5), leyendo cada nodo desde el archivo.
     * Entrada: bloque, rango inclusivo y resultado. Salida: agrega pares hallados.
     */
    static void buscarEn(ArchivoNodos<NodoB>& archivo, std::size_t indice,
                         std::int32_t inferior, std::int32_t superior,
                         std::vector<Par>& resultado) {
        const NodoB nodo = archivo.leer(indice);
        // Hoja a) o interno b): se guardan los pares del nodo que están en [inferior, superior].
        for (int i = 0; i < nodo.k; ++i)
            if (inferior <= nodo.llaves_valores[i].llave && nodo.llaves_valores[i].llave <= superior)
                resultado.push_back(nodo.llaves_valores[i]);
        if (!nodo.es_interno) return;
        // Interno a): se baja a todos los hijos cuyo rango puede tener pares en [inferior, superior].
        for (int i = 0; i <= nodo.k; ++i) {
            // La igualdad a la izquierda admite llaves repetidas a ambos lados de un split.
            const bool tocaIzquierda = i == 0 || superior >= nodo.llaves_valores[i - 1].llave;
            const bool tocaDerecha = i == nodo.k || inferior <= nodo.llaves_valores[i].llave;
            if (tocaIzquierda && tocaDerecha)
                buscarEn(archivo, static_cast<std::size_t>(nodo.hijos[i]),
                         inferior, superior, resultado);
        }
    }

    Disco<NodoB> disco_;  // arreglo de nodos en RAM que simula el disco y cuenta los I/Os
};
