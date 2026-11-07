#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "datos.h"
#include "disco.h"

/*
 * Árbol Bε en memoria externa (secciones 3.2, 3.4.3, 3.4.4 y 3.4.6 del enunciado).
 * Las decisiones que el enunciado no especifica están en docs/DECISIONES_BE.md:
 *  - Un hijo que recibe un grupo sin espacio suficiente después de dividirse recibe
 *    solo lo que cabe; el resto vuelve al buffer del padre ("bajar por partes").
 *  - Una hoja que no está llena y debe dividirse durante un vaciado se corta en ⌈k/2⌉.
 *  - Un nodo interno que llega a c llaves durante un vaciado se divide en ese momento.
 *  - Si una pasada de vaciado no libera espacio, se repite hasta liberarlo o llenar el nodo.
 */

/**
 * Bloque del árbol Bε: 4 + 4 + 4 + 4C + 4(C+1) + 8L = 8(2 + C + L) bytes.
 * C es el máximo de llaves divisorias y L el máximo de pares en el buffer (l_max).
 * Con C + L = 510 ocupa exactamente 4096 bytes.
 */
template <int C, int L>
struct NodoBe {
    std::int32_t es_interno = 0;  // 1 si es interno, 0 si es hoja
    std::int32_t k_llaves = 0;    // llaves divisorias activas (0 en las hojas)
    std::int32_t k_buffer = 0;    // pares guardados en buffer
    std::int32_t llaves[C]{};     // llaves divisorias ordenadas (solo en nodos internos)
    std::int32_t hijos[C + 1]{};  // bloques de los hijos (solo en nodos internos)
    Par buffer[L]{};              // inserciones pendientes (interno) o datos (hoja), ordenado por llave
};
static_assert(sizeof(NodoBe<4, 506>) == 4096, "NodoBe con ε ≈ 0,25 debe ocupar 4096 bytes");
static_assert(sizeof(NodoBe<126, 384>) == 4096, "NodoBe con ε ≈ 0,75 debe ocupar 4096 bytes");

/** Contadores de eventos internos del árbol Bε, para el análisis del informe. */
struct EstadisticasBe {
    std::uint64_t vaciados = 0;        // pasadas de vaciado (flush) ejecutadas
    std::uint64_t splitsHoja = 0;      // divisiones de hojas
    std::uint64_t splitsInternos = 0;  // divisiones de nodos internos
    std::uint64_t lotesParciales = 0;  // veces que un hijo no pudo recibir todo su grupo
};

/**
 * Árbol Bε construido sobre Disco, con la raíz siempre en el bloque 0.
 * C: máximo de llaves divisorias por nodo interno. L: capacidad del buffer (l_max).
 */
template <int C, int L>
class ArbolBe {
    static_assert(C >= 4 && C % 2 == 0, "El split interno usa c/2: c debe ser par y al menos 4");
    static_assert(L >= 2 && L % 2 == 0, "El split de hoja usa l_max/2: l_max debe ser par");

public:
    using Nodo = NodoBe<C, L>;
    static constexpr int c = C;
    static constexpr int l_max = L;

    /** Entrada: α en (0, 1) del criterio de vaciado. Salida: árbol con una hoja vacía en el bloque 0. */
    explicit ArbolBe(double alfa) : alfa_(alfa) {
        if (!(alfa > 0.0 && alfa < 1.0)) throw std::invalid_argument("α debe estar en (0, 1)");
        disco_.agregar(Nodo{});
    }

    /** Entrada: par llave-valor. Salida: árbol actualizado; el par entra por la raíz (3.4.4). */
    void insertar(Par par) {
        Nodo raiz = disco_.leer(0);
        if (!raiz.es_interno) {
            if (raiz.k_buffer == L) {
                ++est_.splitsHoja;
                raiz = nuevaRaiz(dividirHoja(raiz));
            }
            insertarOrdenado(raiz, par);
            disco_.escribir(0, raiz);
            return;
        }
        if (raiz.k_buffer == L) {
            while (raiz.k_buffer == L && raiz.k_llaves < C) vaciar(raiz);
            if (raiz.k_llaves == C) {
                ++est_.splitsInternos;
                raiz = nuevaRaiz(dividirInterno(raiz));
            }
        }
        insertarOrdenado(raiz, par);
        disco_.escribir(0, raiz);
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
    /** Salida: cantidad de niveles (1 si la raíz es hoja). */
    int altura() const { return altura_; }
    /** Salida: α usado en el criterio de vaciado. */
    double alfa() const { return alfa_; }
    /** Salida: contadores de vaciados, divisiones y lotes parciales. */
    const EstadisticasBe& estadisticas() const { return est_; }

    /** Entrada: archivo serializado y rango inclusivo. Salida: todos los pares del rango (3.4.6). */
    static std::vector<Par> buscarRango(ArchivoNodos<Nodo>& archivo,
                                        std::int32_t inferior, std::int32_t superior) {
        if (inferior > superior) throw std::invalid_argument("Rango invertido");
        std::vector<Par> resultado;
        buscarEn(archivo, 0, inferior, superior, resultado);
        return resultado;
    }

    /** Entrada: archivo serializado. Salida: cantidad de pares guardados en todo el árbol (Σ k_buffer). */
    static std::uint64_t contarPares(ArchivoNodos<Nodo>& archivo) { return contarEn(archivo, 0); }

private:
    /** Resultado de dividir un nodo: dos mitades y la llave que sube al padre. */
    struct Division {
        Nodo izquierdo;
        Nodo derecho;
        std::int32_t divisoria = 0;
    };

    /** Entrada: buffer ordenado y llave. Salida: primera posición con llave mayor a la dada. */
    static int posicionMayor(const Par* inicio, int cantidad, std::int32_t llave) {
        return static_cast<int>(std::upper_bound(inicio, inicio + cantidad, llave,
            [](std::int32_t k, const Par& p) { return k < p.llave; }) - inicio);
    }

    /** Entrada: nodo con buffer no lleno y par. Salida: par insertado manteniendo el orden. */
    static void insertarOrdenado(Nodo& nodo, Par par) {
        const int pos = posicionMayor(nodo.buffer, nodo.k_buffer, par.llave);
        std::copy_backward(nodo.buffer + pos, nodo.buffer + nodo.k_buffer,
                           nodo.buffer + nodo.k_buffer + 1);
        nodo.buffer[pos] = par;
        ++nodo.k_buffer;
    }

    /** Entrada: nodo y m pares ordenados que caben en su buffer. Salida: buffer mezclado y ordenado. */
    static void mezclar(Nodo& nodo, const Par* pares, int m) {
        int i = nodo.k_buffer - 1;
        int j = m - 1;
        int destino = nodo.k_buffer + m - 1;
        while (j >= 0) {
            if (i >= 0 && nodo.buffer[i].llave > pares[j].llave) nodo.buffer[destino--] = nodo.buffer[i--];
            else nodo.buffer[destino--] = pares[j--];
        }
        nodo.k_buffer += m;
    }

    /**
     * Entrada: nodo destino, m pares ordenados y vector de sobrantes.
     * Salida: el destino recibe los pares que caben en su buffer; los demás se agregan a sobrantes.
     */
    static void moverLoQueQuepa(Nodo& destino, const Par* pares, int m, std::vector<Par>& sobrantes) {
        const int caben = std::min(m, L - destino.k_buffer);
        mezclar(destino, pares, caben);
        sobrantes.insert(sobrantes.end(), pares + caben, pares + m);
    }

    /**
     * Split de hoja (3.4.3). Entrada: hoja con k ≥ 1 pares. Salida: la izquierda guarda los
     * primeros ⌈k/2⌉ pares (l_max/2 si está llena) y la llave del último sube como divisoria.
     */
    static Division dividirHoja(const Nodo& hoja) {
        if (hoja.es_interno || hoja.k_buffer < 1) throw std::logic_error("Split de hoja inválido");
        Division d;
        const int izquierda = (hoja.k_buffer + 1) / 2;
        d.izquierdo.k_buffer = izquierda;
        d.derecho.k_buffer = hoja.k_buffer - izquierda;
        std::copy_n(hoja.buffer, izquierda, d.izquierdo.buffer);
        std::copy_n(hoja.buffer + izquierda, d.derecho.k_buffer, d.derecho.buffer);
        d.divisoria = hoja.buffer[izquierda - 1].llave;
        return d;
    }

    /**
     * Split de nodo interno (3.4.3). Entrada: nodo con c llaves. Salida: c/2 − 1 llaves y c/2 hijos
     * a la izquierda, c/2 llaves y c/2 + 1 hijos a la derecha, la llave c/2 sube y el buffer se
     * reparte (≤ mediana a la izquierda, > mediana a la derecha).
     */
    static Division dividirInterno(const Nodo& nodo) {
        if (!nodo.es_interno || nodo.k_llaves != C) throw std::logic_error("Split interno inválido");
        constexpr int medio = C / 2 - 1;
        Division d;
        d.izquierdo.es_interno = d.derecho.es_interno = 1;
        d.izquierdo.k_llaves = medio;
        d.derecho.k_llaves = C - medio - 1;
        std::copy_n(nodo.llaves, medio, d.izquierdo.llaves);
        std::copy_n(nodo.hijos, medio + 1, d.izquierdo.hijos);
        std::copy_n(nodo.llaves + medio + 1, d.derecho.k_llaves, d.derecho.llaves);
        std::copy_n(nodo.hijos + medio + 1, d.derecho.k_llaves + 1, d.derecho.hijos);
        d.divisoria = nodo.llaves[medio];
        const int corte = posicionMayor(nodo.buffer, nodo.k_buffer, d.divisoria);
        d.izquierdo.k_buffer = corte;
        d.derecho.k_buffer = nodo.k_buffer - corte;
        std::copy_n(nodo.buffer, corte, d.izquierdo.buffer);
        std::copy_n(nodo.buffer + corte, d.derecho.k_buffer, d.derecho.buffer);
        return d;
    }

    /**
     * Entrada: división de la raíz. Salida: nueva raíz interna con la divisoria y el buffer vacío;
     * las dos mitades se escriben al final del arreglo. La llama quien escribe el bloque 0.
     */
    Nodo nuevaRaiz(const Division& d) {
        const auto izquierdo = disco_.agregar(d.izquierdo);
        const auto derecho = disco_.agregar(d.derecho);
        Nodo raiz{};
        raiz.es_interno = 1;
        raiz.k_llaves = 1;
        raiz.llaves[0] = d.divisoria;
        raiz.hijos[0] = static_cast<std::int32_t>(izquierdo);
        raiz.hijos[1] = static_cast<std::int32_t>(derecho);
        ++altura_;
        return raiz;
    }

    /** Entrada: nodo con menos de c llaves, posición i, llave e hijo nuevo. Salida: llave en i e hijo en i + 1. */
    static void agregarDivisoria(Nodo& nodo, int i, std::int32_t llave, std::size_t hijo) {
        for (int j = nodo.k_llaves; j > i; --j) nodo.llaves[j] = nodo.llaves[j - 1];
        for (int j = nodo.k_llaves + 1; j > i + 1; --j) nodo.hijos[j] = nodo.hijos[j - 1];
        nodo.llaves[i] = llave;
        nodo.hijos[i + 1] = static_cast<std::int32_t>(hijo);
        ++nodo.k_llaves;
    }

    /**
     * Una pasada de vaciado (3.4.3) sobre un nodo interno en memoria; quien lo tiene lo escribe.
     * Entrada: nodo con buffer no vacío y menos de c llaves. Salida: bajan los grupos con al menos
     * α·f pares, con f = ⌈k_buffer / (k_llaves + 1)⌉, de derecha a izquierda. La pasada se detiene
     * si el nodo llega a c llaves, porque ya no puede recibir otro hijo.
     */
    void vaciar(Nodo& nodo) {
        ++est_.vaciados;
        const int hijos = nodo.k_llaves + 1;
        int inicio[C + 1];
        int fin[C + 1];
        int pos = 0;
        for (int i = 0; i < hijos; ++i) {
            inicio[i] = pos;
            pos = i < nodo.k_llaves
                ? pos + posicionMayor(nodo.buffer + pos, nodo.k_buffer - pos, nodo.llaves[i])
                : nodo.k_buffer;
            fin[i] = pos;
        }
        const int f = (nodo.k_buffer + hijos - 1) / hijos;
        const double umbral = alfa_ * f;
        for (int i = hijos - 1; i >= 0; --i) {
            const int tamano = fin[i] - inicio[i];
            if (tamano == 0 || tamano < umbral) continue;
            if (nodo.k_llaves == C) break;
            bajarGrupo(nodo, i, inicio[i], fin[i]);
        }
    }

    /**
     * Entrada: padre en memoria con menos de c llaves, hijo i y su grupo [a, b) del buffer.
     * Salida: el grupo baja al hijo (que puede dividirse y agregar una llave al padre); lo que no
     * cabe vuelve al buffer del padre en la misma posición. Escribe el hijo, no el padre.
     */
    void bajarGrupo(Nodo& padre, int i, int a, int b) {
        const std::vector<Par> grupo(padre.buffer + a, padre.buffer + b);
        std::copy(padre.buffer + b, padre.buffer + padre.k_buffer, padre.buffer + a);
        padre.k_buffer -= b - a;
        const int m = b - a;
        const auto indice = static_cast<std::size_t>(padre.hijos[i]);
        Nodo hijo = disco_.leer(indice);
        if (hijo.es_interno) {
            while (hijo.k_buffer + m > L && hijo.k_llaves < C) vaciar(hijo);
        }
        const bool divide = hijo.es_interno ? hijo.k_llaves == C : hijo.k_buffer + m > L;
        if (!divide) {
            mezclar(hijo, grupo.data(), m);
            disco_.escribir(indice, hijo);
            return;
        }
        if (hijo.es_interno) ++est_.splitsInternos;
        else ++est_.splitsHoja;
        Division d = hijo.es_interno ? dividirInterno(hijo) : dividirHoja(hijo);
        const int corte = posicionMayor(grupo.data(), m, d.divisoria);
        std::vector<Par> sobrantes;
        moverLoQueQuepa(d.izquierdo, grupo.data(), corte, sobrantes);
        moverLoQueQuepa(d.derecho, grupo.data() + corte, m - corte, sobrantes);
        disco_.escribir(indice, d.izquierdo);
        const auto derecho = disco_.agregar(d.derecho);
        agregarDivisoria(padre, i, d.divisoria, derecho);
        if (sobrantes.empty()) return;
        ++est_.lotesParciales;
        const int r = static_cast<int>(sobrantes.size());
        std::copy_backward(padre.buffer + a, padre.buffer + padre.k_buffer,
                           padre.buffer + padre.k_buffer + r);
        std::copy(sobrantes.begin(), sobrantes.end(), padre.buffer + a);
        padre.k_buffer += r;
    }

    /**
     * Entrada: archivo, bloque, rango inclusivo y resultado. Salida: agrega los pares del buffer
     * del nodo que están en el rango y sigue por los hijos cuyo rango intersecta [inferior, superior].
     */
    static void buscarEn(ArchivoNodos<Nodo>& archivo, std::size_t indice,
                         std::int32_t inferior, std::int32_t superior, std::vector<Par>& resultado) {
        const Nodo nodo = archivo.leer(indice);
        const Par* fin = nodo.buffer + nodo.k_buffer;
        const Par* p = std::lower_bound(nodo.buffer, fin, inferior,
            [](const Par& par, std::int32_t k) { return par.llave < k; });
        for (; p != fin && p->llave <= superior; ++p) resultado.push_back(*p);
        if (!nodo.es_interno) return;
        for (int i = 0; i <= nodo.k_llaves; ++i) {
            // La igualdad a la izquierda admite llaves repetidas a ambos lados de un split.
            const bool tocaIzquierda = i == 0 || superior >= nodo.llaves[i - 1];
            const bool tocaDerecha = i == nodo.k_llaves || inferior <= nodo.llaves[i];
            if (tocaIzquierda && tocaDerecha)
                buscarEn(archivo, static_cast<std::size_t>(nodo.hijos[i]), inferior, superior, resultado);
        }
    }

    /** Entrada: archivo y bloque. Salida: pares guardados en los buffers del subárbol. */
    static std::uint64_t contarEn(ArchivoNodos<Nodo>& archivo, std::size_t indice) {
        const Nodo nodo = archivo.leer(indice);
        std::uint64_t total = static_cast<std::uint64_t>(nodo.k_buffer);
        if (nodo.es_interno)
            for (int i = 0; i <= nodo.k_llaves; ++i)
                total += contarEn(archivo, static_cast<std::size_t>(nodo.hijos[i]));
        return total;
    }

    double alfa_;
    int altura_ = 1;
    EstadisticasBe est_;
    Disco<Nodo> disco_;
};

/** Configuración ε ≈ 0,25: 4 llaves divisorias (5 hijos) y buffer de 506 pares. */
using ArbolBe025 = ArbolBe<4, 506>;
/** Configuración ε ≈ 0,75: 126 llaves divisorias (127 hijos) y buffer de 384 pares. */
using ArbolBe075 = ArbolBe<126, 384>;
