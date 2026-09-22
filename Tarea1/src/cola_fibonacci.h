#pragma once
/**
 * cola_fibonacci.h — Cola de Fibonacci (Persona A).
 *
 * ESQUELETO: la interfaz pública está congelada (ver docs/ACUERDOS.md).
 * Lo privado es libre. Se puede partir del código de la tarea anterior
 * (Basic_definitionsFibonacci.h en el proyecto), adaptándolo a esta interfaz.
 *
 * Complejidades esperadas: insert O(1), extractMin O(log n) amortizado,
 * decreaseKey O(1) amortizado (cortes en cascada, sección 3.3).
 *
 * TODO(A):
 *  - insert, extractMin (consolidación reutilizando la unión de la binomial),
 *    decreaseKey con cut + cascadingCut.
 *  - `ops += 1` por cada cut hecho dentro de decreaseKey, incluido el primero.
 *  - Liberar todos los nodos en el destructor.
 *  - Cambiar `implementada` a true.
 */
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/** Nodo de la cola de Fibonacci: listas circulares doblemente enlazadas. */
struct NodoFibonacci {
    double clave;
    int vertice;
    int grado;
    bool marca;
    NodoFibonacci* padre;
    NodoFibonacci* hijo;
    NodoFibonacci* izq;
    NodoFibonacci* der;
};

class ColaFibonacci {
public:
    static constexpr const char* nombre = "fibonacci";
    static constexpr bool implementada = false;  // TODO(A): true cuando funcione

    /** Cantidad de cortes hechos por decreaseKey (conteo de operaciones). */
    int64_t ops = 0;

    /** Entrada: n = |V|. Prepara el arreglo vértice -> nodo. */
    explicit ColaFibonacci(int n) : nodoDe(n, nullptr) {}
    ~ColaFibonacci() { /* TODO(A): liberar todos los nodos */ }
    ColaFibonacci(const ColaFibonacci&) = delete;
    ColaFibonacci& operator=(const ColaFibonacci&) = delete;

    /** Inserta el vértice v con costo key. */
    void insert(int v, double key) { (void)v; (void)key; noImplementado("insert"); }

    /** Extrae y retorna (costo, vértice) de menor costo. */
    std::pair<double, int> extractMin() { noImplementado("extractMin"); }

    /** Reduce el costo del vértice v a key (key <= costo actual). */
    void decreaseKey(int v, double key) { (void)v; (void)key; noImplementado("decreaseKey"); }

    /** Salida: true si la cola está vacía. */
    bool empty() const { return tam == 0; }

    /** Bytes por nodo (para la estimación de memoria de la sección 6.2). */
    static size_t bytesPorNodo() { return sizeof(NodoFibonacci); }

private:
    std::vector<NodoFibonacci*> nodoDe;  // nodoDe[v] = nodo del vértice v
    NodoFibonacci* minimo = nullptr;     // puntero al mínimo (entrada a la lista de raíces)
    int tam = 0;

    [[noreturn]] static void noImplementado(const char* f) {
        throw std::logic_error(std::string("ColaFibonacci::") + f + " no implementado");
    }
};
