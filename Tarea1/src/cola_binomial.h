#pragma once
/**
 * cola_binomial.h — Cola binomial (Persona A).
 *
 * ESQUELETO: la interfaz pública está congelada (ver docs/ACUERDOS.md).
 * Lo privado (struct de nodo, helpers) es libre: cámbienlo como quieran.
 *
 * Complejidades esperadas: insert O(1) amortizado, extractMin O(log n),
 * decreaseKey O(log n) peor caso. Construcción con n inserciones: O(n).
 *
 * TODO(A):
 *  - insert, extractMin (unión de árboles B_k), decreaseKey (sección 3.2).
 *  - Decidir: intercambiar contenido y actualizar nodoDe[], o reconectar
 *    punteros del árbol. Documentar la decisión (va en el informe).
 *  - `ops += 1` por cada intercambio del while de decreaseKey (SOLO ahí).
 *  - Liberar todos los nodos en el destructor.
 *  - Cambiar `implementada` a true.
 */
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/** Nodo de un árbol binomial. Sugerencia: hijo más a la izquierda + hermano derecho. */
struct NodoBinomial {
    double clave;
    int vertice;
    int grado;
    NodoBinomial* padre;
    NodoBinomial* hijo;     // hijo de mayor grado (o menor, a elección)
    NodoBinomial* hermano;  // siguiente en la lista de hermanos / raíces
};

class ColaBinomial {
public:
    static constexpr const char* nombre = "binomial";
    static constexpr bool implementada = false;  // TODO(A): true cuando funcione

    /** Cantidad de intercambios hechos por decreaseKey (conteo de operaciones). */
    int64_t ops = 0;

    /** Entrada: n = |V|. Prepara el arreglo vértice -> nodo. */
    explicit ColaBinomial(int n) : nodoDe(n, nullptr) {}
    ~ColaBinomial() { /* TODO(A): liberar todos los nodos */ }
    ColaBinomial(const ColaBinomial&) = delete;
    ColaBinomial& operator=(const ColaBinomial&) = delete;

    /** Inserta el vértice v con costo key. */
    void insert(int v, double key) { (void)v; (void)key; noImplementado("insert"); }

    /** Extrae y retorna (costo, vértice) de menor costo. */
    std::pair<double, int> extractMin() { noImplementado("extractMin"); }

    /** Reduce el costo del vértice v a key (key <= costo actual). */
    void decreaseKey(int v, double key) { (void)v; (void)key; noImplementado("decreaseKey"); }

    /** Salida: true si la cola está vacía. */
    bool empty() const { return tam == 0; }

    /** Bytes por nodo (para la estimación de memoria de la sección 6.2). */
    static size_t bytesPorNodo() { return sizeof(NodoBinomial); }

private:
    std::vector<NodoBinomial*> nodoDe;  // nodoDe[v] = nodo que representa al vértice v
    NodoBinomial* raices = nullptr;     // lista de raíces
    NodoBinomial* minimo = nullptr;
    int tam = 0;

    [[noreturn]] static void noImplementado(const char* f) {
        throw std::logic_error(std::string("ColaBinomial::") + f + " no implementado");
    }
};
