#pragma once
/** Cola de Fibonacci con listas circulares, cortes y cortes en cascada. */
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

/** Nodo de Fibonacci; las raíces y los hijos usan anillos doblemente enlazados. */
struct NodoFibonacci {
    double clave;
    int vertice, grado;
    bool marca;
    NodoFibonacci *padre, *hijo, *izq, *der;
};

class ColaFibonacci {
public:
    static constexpr const char* nombre = "fibonacci";
    static constexpr bool implementada = true;
    int64_t ops = 0;  // cortes realizados únicamente por decreaseKey
    int64_t opsCascada = 0;  // solo los cortes hechos por cascadingCut (sin el corte inicial)

    /** Entrada: cantidad de vértices 0..n-1. */
    explicit ColaFibonacci(int n) : nodoDe(n, nullptr) {}
    /** Libera los nodos que siguen presentes en la cola. */
    ~ColaFibonacci() { for (NodoFibonacci* x : nodoDe) delete x; }
    ColaFibonacci(const ColaFibonacci&) = delete;
    ColaFibonacci& operator=(const ColaFibonacci&) = delete;

    /** Entrada: v ausente, costo key. Inserta una nueva raíz en O(1). */
    void insert(int v, double key) {
        if (v < 0 || v >= static_cast<int>(nodoDe.size()) || nodoDe[v])
            throw std::invalid_argument("insert: vertice invalido o repetido");
        NodoFibonacci* x = new NodoFibonacci{key, v, 0, false, nullptr, nullptr, nullptr, nullptr};
        x->izq = x->der = x;
        nodoDe[v] = x;
        agregarRaiz(x);
        ++tam;
    }

    /** Salida: par mínimo. Promueve hijos a raíces y consolida por grado. */
    std::pair<double, int> extractMin() {
        if (!minimo) throw std::logic_error("extractMin sobre cola vacia");
        NodoFibonacci* x = minimo;
        if (x->hijo) {
            NodoFibonacci* h = x->hijo;
            int cantidad = x->grado;
            for (int i = 0; i < cantidad; ++i) {
                NodoFibonacci* sig = h->der;
                quitar(h);
                h->padre = nullptr;
                h->marca = false;
                insertarAntes(x, h);
                h = sig;
            }
            x->hijo = nullptr;
        }
        NodoFibonacci* siguiente = x->der;
        bool unico = siguiente == x;
        quitar(x);
        --tam;
        minimo = unico ? nullptr : siguiente;
        if (minimo) consolidar();
        nodoDe[x->vertice] = nullptr;
        std::pair<double, int> resultado{x->clave, x->vertice};
        delete x;
        return resultado;
    }

    /**
     * Entrada: v presente y key <= clave; corta si viola el orden y propaga cortes.
     * Compara solo claves y en forma estricta (línea 3 de decreaseKey-Fibonacci).
     * El mínimo solo se actualiza si x es raíz: con claves iguales x no se corta
     * y queda como hijo, así que no puede ser el mínimo de la lista de raíces.
     */
    void decreaseKey(int v, double key) {
        if (v < 0 || v >= static_cast<int>(nodoDe.size()) || !nodoDe[v] || key > nodoDe[v]->clave)
            throw std::invalid_argument("decreaseKey: vertice ausente o aumento de clave");
        NodoFibonacci* x = nodoDe[v];
        x->clave = key;
        NodoFibonacci* p = x->padre;
        if (p && x->clave < p->clave) {
            cortar(x, p);
            corteCascada(p);
        }
        if (x->padre == nullptr && menor(x, minimo)) minimo = x;
    }

    /** Salida: true si la cola no contiene nodos. */
    bool empty() const { return tam == 0; }
    /** Entrada: vértice v en 0..n-1. Salida: true si v todavía está en la cola. */
    bool contiene(int v) const { return nodoDe[v] != nullptr; }
    /** Salida: bytes de un nodo, excluido nodoDe. */
    static size_t bytesPorNodo() { return sizeof(NodoFibonacci); }

private:
    std::vector<NodoFibonacci*> nodoDe;
    NodoFibonacci* minimo = nullptr;
    int tam = 0;

    /** Orden total para empates reproducibles (elección del mínimo y enlaces). */
    static bool menor(const NodoFibonacci* a, const NodoFibonacci* b) {
        return a->clave < b->clave || (a->clave == b->clave && a->vertice < b->vertice);
    }
    /** Inserta x inmediatamente antes de pos en un anillo circular. */
    static void insertarAntes(NodoFibonacci* pos, NodoFibonacci* x) {
        x->izq = pos->izq;
        x->der = pos;
        pos->izq->der = x;
        pos->izq = x;
    }
    /** Saca x de su anillo, dejándolo como anillo unitario. */
    static void quitar(NodoFibonacci* x) {
        x->izq->der = x->der;
        x->der->izq = x->izq;
        x->izq = x->der = x;
    }
    /** Agrega x a la lista de raíces y actualiza el mínimo. */
    void agregarRaiz(NodoFibonacci* x) {
        x->padre = nullptr;
        x->marca = false;
        if (!minimo) minimo = x;
        else {
            insertarAntes(minimo, x);
            if (menor(x, minimo)) minimo = x;
        }
    }
    /** Une dos árboles del mismo grado, dejando al menor como padre. */
    static void enlazar(NodoFibonacci* hijo, NodoFibonacci* padre) {
        hijo->padre = padre;
        hijo->marca = false;
        if (!padre->hijo) padre->hijo = hijo;
        else insertarAntes(padre->hijo, hijo);
        ++padre->grado;
    }
    /** Reúne las raíces por grado tras extractMin y reconstruye su anillo. */
    void consolidar() {
        std::vector<NodoFibonacci*> raices;
        NodoFibonacci* r = minimo;
        do {
            raices.push_back(r);
            r = r->der;
        } while (r != minimo);
        std::vector<NodoFibonacci*> porGrado;
        for (NodoFibonacci* x : raices) {
            quitar(x);
            while (true) {
                if (x->grado >= static_cast<int>(porGrado.size()))
                    porGrado.resize(static_cast<size_t>(x->grado) + 1, nullptr);
                NodoFibonacci*& otro = porGrado[x->grado];
                if (!otro) { otro = x; break; }
                NodoFibonacci* y = otro;
                otro = nullptr;
                if (menor(y, x)) std::swap(x, y);
                enlazar(y, x);
            }
        }
        minimo = nullptr;
        for (NodoFibonacci* x : porGrado) if (x) agregarRaiz(x);
    }
    /** Quita x de los hijos de p y lo agrega a las raíces; cuenta un corte. */
    void cortar(NodoFibonacci* x, NodoFibonacci* p) {
        if (p->hijo == x) p->hijo = (x->der == x) ? nullptr : x->der;
        quitar(x);
        --p->grado;
        agregarRaiz(x);
        ++ops;
    }
    /** Marca al primer ancestro que pierde un hijo; corta los ya marcados (cuenta en opsCascada). */
    void corteCascada(NodoFibonacci* x) {
        NodoFibonacci* p = x->padre;
        if (!p) return;
        if (!x->marca) x->marca = true;
        else {
            cortar(x, p);
            ++opsCascada;
            corteCascada(p);
        }
    }
};
