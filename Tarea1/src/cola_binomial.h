#pragma once
/** Cola binomial: raíces ordenadas por grado y acceso directo vértice -> nodo. */
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

/** Nodo binomial: la lista hijo/hermano almacena hijos de grado decreciente. */
struct NodoBinomial {
    double clave;
    int vertice, grado;
    NodoBinomial *padre, *hijo, *hermano;
};

class ColaBinomial {
public:
    static constexpr const char* nombre = "binomial";
    static constexpr bool implementada = true;
    int64_t ops = 0;  // intercambios hechos en decreaseKey

    /** Entrada: cantidad de vértices numerados 0..n-1. */
    explicit ColaBinomial(int n) : nodoDe(n, nullptr) {}
    /** Libera los nodos restantes. */
    ~ColaBinomial() { for (NodoBinomial* x : nodoDe) delete x; }
    ColaBinomial(const ColaBinomial&) = delete;
    ColaBinomial& operator=(const ColaBinomial&) = delete;

    /** Entrada: vértice ausente v y costo key; inserta B0 con acarreo de grados. */
    void insert(int v, double key) {
        if (v < 0 || v >= static_cast<int>(nodoDe.size()) || nodoDe[v])
            throw std::invalid_argument("insert: vertice invalido o repetido");
        NodoBinomial* x = new NodoBinomial{key, v, 0, nullptr, nullptr, nullptr};
        nodoDe[v] = x;
        // Solo se recorren los grados ocupados consecutivos desde B0.
        // El costo total de n inserciones es O(n), como en un contador binario.
        while (raices && raices->grado == x->grado) {
            NodoBinomial* y = raices;
            raices = y->hermano;
            y->hermano = nullptr;
            if (menor(y, x)) std::swap(x, y);
            enlazar(y, x);
        }
        x->hermano = raices;
        raices = x;
        if (!minimo || menor(x, minimo)) minimo = x;
        ++tam;
    }

    /** Salida: par (costo, vértice) mínimo; retira y libera ese nodo. */
    std::pair<double, int> extractMin() {
        if (!minimo) throw std::logic_error("extractMin sobre cola vacia");
        NodoBinomial *prev = nullptr, *antes = nullptr;
        for (NodoBinomial* r = raices; r; r = r->hermano) {
            if (r == minimo) { antes = prev; break; }
            prev = r;
        }
        NodoBinomial* x = minimo;
        if (antes) antes->hermano = x->hermano;
        else raices = x->hermano;
        // Los hijos están en grado decreciente: invertir para unir listas.
        NodoBinomial* hijos = nullptr;
        for (NodoBinomial* h = x->hijo; h;) {
            NodoBinomial* sig = h->hermano;
            h->padre = nullptr;
            h->hermano = hijos;
            hijos = h;
            h = sig;
        }
        raices = unirListas(raices, hijos);
        consolidar();
        minimo = nullptr;
        for (NodoBinomial* r = raices; r; r = r->hermano)
            if (!minimo || menor(r, minimo)) minimo = r;
        --tam;
        nodoDe[x->vertice] = nullptr;
        std::pair<double, int> resultado{x->clave, x->vertice};
        delete x;
        return resultado;
    }

    /** Entrada: v presente y key <= clave actual. Sube contenido y repara nodoDe. */
    void decreaseKey(int v, double key) {
        if (v < 0 || v >= static_cast<int>(nodoDe.size()) || !nodoDe[v] || key > nodoDe[v]->clave)
            throw std::invalid_argument("decreaseKey: vertice ausente o aumento de clave");
        NodoBinomial* x = nodoDe[v];
        x->clave = key;
        while (x->padre && menor(x, x->padre)) {
            NodoBinomial* p = x->padre;
            std::swap(x->clave, p->clave);
            std::swap(x->vertice, p->vertice);
            nodoDe[x->vertice] = x;
            nodoDe[p->vertice] = p;
            ++ops;
            x = p;
        }
        if (x->padre == nullptr && menor(x, minimo)) minimo = x;
    }

    /** Salida: true si la cola está vacía. */
    bool empty() const { return tam == 0; }
    /** Salida: memoria de un nodo, excluido nodoDe. */
    static size_t bytesPorNodo() { return sizeof(NodoBinomial); }

private:
    std::vector<NodoBinomial*> nodoDe;
    NodoBinomial* raices = nullptr;
    NodoBinomial* minimo = nullptr;
    int tam = 0;

    /** Orden total para resolver empates de manera reproducible. */
    static bool menor(const NodoBinomial* a, const NodoBinomial* b) {
        return a->clave < b->clave || (a->clave == b->clave && a->vertice < b->vertice);
    }
    /** Une dos árboles del mismo grado; entrada: hijo y padre; salida: árbol padre. */
    static void enlazar(NodoBinomial* hijo, NodoBinomial* padre) {
        hijo->padre = padre;
        hijo->hermano = padre->hijo;
        padre->hijo = hijo;
        ++padre->grado;
    }
    /** Fusiona dos listas de raíces ordenadas por grado. */
    static NodoBinomial* unirListas(NodoBinomial* a, NodoBinomial* b) {
        NodoBinomial cabeza{};
        NodoBinomial* cola = &cabeza;
        while (a && b) {
            NodoBinomial*& elegido = a->grado <= b->grado ? a : b;
            cola->hermano = elegido;
            elegido = elegido->hermano;
            cola = cola->hermano;
        }
        cola->hermano = a ? a : b;
        return cabeza.hermano;
    }
    /** Enlaza raíces del mismo grado hasta dejar a lo sumo una por grado. */
    void consolidar() {
        if (!raices) return;
        NodoBinomial *prev = nullptr, *x = raices, *sig = x->hermano;
        while (sig) {
            if (x->grado != sig->grado || (sig->hermano && sig->hermano->grado == x->grado)) {
                prev = x; x = sig;
            } else if (menor(x, sig)) {
                x->hermano = sig->hermano;
                enlazar(sig, x);
            } else {
                if (prev) prev->hermano = sig;
                else raices = sig;
                enlazar(x, sig);
                x = sig;
            }
            sig = x->hermano;
        }
    }
};
