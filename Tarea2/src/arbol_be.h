#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "datos.h"
#include "disco.h"

/** Bloque Bε: tres enteros, c divisorias, c+1 hijos y l pares. */
template <int C, int L> struct NodoBe {
    std::int32_t es_interno = 0;
    std::int32_t k_llaves = 0;
    std::int32_t k_buffer = 0;
    std::int32_t llaves[C]{};
    std::int32_t hijos[C + 1]{};
    Par buffer[L]{};
};

/** Árbol Bε con inserciones amortizadas mediante buffers y lectura por bloque. */
template <int C, int L> class ArbolBe {
    static_assert(C > 0 && L > 0 && C + L == 510, "Se requiere c + l_max = 510");
    static_assert(sizeof(NodoBe<C, L>) == 4096, "El nodo Bε debe ocupar 4096 bytes");
public:
    using Nodo = NodoBe<C, L>;

    /** Entrada: 0 < alfa < 1. Salida: árbol con hoja raíz en el bloque 0. */
    explicit ArbolBe(double alfa = 0.5) : alfa_(alfa) {
        if (!(alfa > 0.0 && alfa < 1.0)) throw std::invalid_argument("alfa debe estar en (0,1)");
        disco_.agregar(Nodo{});
    }

    /** Entrada: par llave-valor. Salida: inserción por la raíz y vaciados necesarios. */
    void insertar(Par par) {
        const auto partes = transferir(0, std::vector<Par>{par});
        if (partes.indices.size() == 1) return;
        Nodo izquierda = disco_.leer(0);
        partesCaben(partes);
        const auto nuevaIzquierda = disco_.agregar(izquierda);
        Nodo raiz{};
        raiz.es_interno = 1;
        raiz.k_llaves = static_cast<std::int32_t>(partes.pivotes.size());
        for (std::size_t i = 0; i < partes.pivotes.size(); ++i) raiz.llaves[i] = partes.pivotes[i];
        raiz.hijos[0] = static_cast<std::int32_t>(nuevaIzquierda);
        for (std::size_t i = 1; i < partes.indices.size(); ++i)
            raiz.hijos[i] = static_cast<std::int32_t>(partes.indices[i]);
        disco_.escribir(0, raiz);
    }

    /** Entrada: ruta. Salida: nodos serializados, con raíz en el bloque 0. */
    void guardar(const std::string& ruta) const { disco_.guardar(ruta); }
    /** Salida: número de nodos. */
    std::size_t cantidadNodos() const { return disco_.cantidadNodos(); }
    /** Salida: tamaño del archivo serializado. */
    std::uint64_t tamanoBytes() const { return disco_.tamanoBytes(); }
    /** Salida: lecturas simuladas durante la construcción. */
    std::uint64_t lecturas() const { return disco_.lecturas; }
    /** Salida: escrituras simuladas durante la construcción. */
    std::uint64_t escrituras() const { return disco_.escrituras; }

    /** Entrada: archivo y rango inclusivo. Salida: todos los pares dentro del rango. */
    static std::vector<Par> buscarRango(ArchivoNodos<Nodo>& archivo,
                                         std::int32_t inferior, std::int32_t superior) {
        if (inferior > superior) throw std::invalid_argument("Rango invertido");
        std::vector<Par> resultado;
        buscarEn(archivo, 0, inferior, superior, resultado);
        return resultado;
    }

    /** Entrada: archivo serializado. Salida: cantidad de pares alcanzables desde la raíz. */
    static std::uint64_t contarPares(ArchivoNodos<Nodo>& archivo) {
        return contarEn(archivo, 0);
    }

private:
    /** Estado transitorio que admite desbordes antes de repartirlos en bloques. */
    struct Estado {
        bool interno = false;
        std::vector<std::int32_t> llaves;
        std::vector<std::int32_t> hijos;
        std::vector<Par> buffer;
    };

    /** Bloques que reemplazan al hijo anterior y divisorias entre ellos. */
    struct Partes {
        std::vector<std::size_t> indices;
        std::vector<std::int32_t> pivotes;
    };

    /** Entrada: partes de una división de raíz. Salida: error si no caben en una raíz. */
    static void partesCaben(const Partes& partes) {
        if (partes.pivotes.size() > static_cast<std::size_t>(C))
            throw std::logic_error("Demasiadas divisorias en la nueva raíz");
    }

    /** Entrada: nodo leído. Salida: estado editable. */
    static Estado expandir(const Nodo& nodo) {
        if (nodo.k_llaves < 0 || nodo.k_llaves > C || nodo.k_buffer < 0 || nodo.k_buffer > L)
            throw std::logic_error("Nodo Bε corrupto");
        Estado estado;
        estado.interno = nodo.es_interno != 0;
        estado.llaves.assign(nodo.llaves, nodo.llaves + nodo.k_llaves);
        if (estado.interno) estado.hijos.assign(nodo.hijos, nodo.hijos + nodo.k_llaves + 1);
        estado.buffer.assign(nodo.buffer, nodo.buffer + nodo.k_buffer);
        return estado;
    }

    /** Entrada: estado dentro de capacidad. Salida: bloque de 4096 bytes. */
    static Nodo compactar(const Estado& estado) {
        if (estado.llaves.size() > static_cast<std::size_t>(C) ||
            estado.buffer.size() > static_cast<std::size_t>(L) ||
            (estado.interno && estado.hijos.size() != estado.llaves.size() + 1))
            throw std::logic_error("Estado Bε no cabe en un bloque");
        Nodo nodo{};
        nodo.es_interno = estado.interno;
        nodo.k_llaves = static_cast<std::int32_t>(estado.llaves.size());
        nodo.k_buffer = static_cast<std::int32_t>(estado.buffer.size());
        std::copy(estado.llaves.begin(), estado.llaves.end(), nodo.llaves);
        std::copy(estado.hijos.begin(), estado.hijos.end(), nodo.hijos);
        std::copy(estado.buffer.begin(), estado.buffer.end(), nodo.buffer);
        return nodo;
    }

    /** Entrada: divisorias y llave. Salida: primer hijo cuyo límite superior admite la llave. */
    static std::size_t hijoDe(const Estado& estado, std::int32_t llave) {
        return static_cast<std::size_t>(std::lower_bound(
            estado.llaves.begin(), estado.llaves.end(), llave) - estado.llaves.begin());
    }

    /** Entrada: dos secuencias ordenadas. Salida: mezcla ordenada por llave. */
    static void mezclar(std::vector<Par>& destino, const std::vector<Par>& lote) {
        std::vector<Par> mezcla;
        mezcla.reserve(destino.size() + lote.size());
        std::merge(destino.begin(), destino.end(), lote.begin(), lote.end(),
                   std::back_inserter(mezcla), [](const Par& a, const Par& b) {
                       return a.llave < b.llave;
                   });
        destino.swap(mezcla);
    }

    /** Entrada: estado lleno y un lote. Salida: uno o más bloques, manteniendo el primer índice. */
    Partes transferir(std::size_t indice, const std::vector<Par>& lote) {
        Estado estado = expandir(disco_.leer(indice));
        mezclar(estado.buffer, lote);
        if (estado.interno) vaciarHastaCaber(estado);
        std::vector<Estado> fragmentos;
        std::vector<std::int32_t> pivotes;
        if (estado.interno) dividirInterno(estado, fragmentos, pivotes);
        else dividirHoja(estado, fragmentos, pivotes);
        Partes partes;
        partes.pivotes = std::move(pivotes);
        for (std::size_t i = 0; i < fragmentos.size(); ++i) {
            const Nodo nodo = compactar(fragmentos[i]);
            if (i == 0) {
                disco_.escribir(indice, nodo);
                partes.indices.push_back(indice);
            } else partes.indices.push_back(disco_.agregar(nodo));
        }
        return partes;
    }

    /** Entrada: nodo interno con buffer desbordado. Salida: baja grupos hasta que quepan. */
    void vaciarHastaCaber(Estado& estado) {
        while (estado.buffer.size() > static_cast<std::size_t>(L)) {
            const std::size_t cantidadHijos = estado.hijos.size();
            const std::size_t f = (estado.buffer.size() + cantidadHijos - 1) / cantidadHijos;
            const std::size_t umbral = static_cast<std::size_t>(std::ceil(f * alfa_));
            std::vector<std::size_t> grupos(cantidadHijos, 0);
            for (const Par& par : estado.buffer) ++grupos[hijoDe(estado, par.llave)];
            std::size_t elegido = cantidadHijos;
            for (std::size_t i = cantidadHijos; i-- > 0;)
                if (grupos[i] >= umbral) { elegido = i; break; }
            if (elegido == cantidadHijos) throw std::logic_error("Flush sin hijo elegible");

            std::vector<Par> lote;
            std::vector<Par> restantes;
            lote.reserve(grupos[elegido]);
            restantes.reserve(estado.buffer.size() - grupos[elegido]);
            for (const Par& par : estado.buffer) {
                if (hijoDe(estado, par.llave) == elegido) lote.push_back(par);
                else restantes.push_back(par);
            }
            estado.buffer.swap(restantes);
            const Partes partes = transferir(static_cast<std::size_t>(estado.hijos[elegido]), lote);
            estado.hijos[elegido] = static_cast<std::int32_t>(partes.indices[0]);
            estado.hijos.insert(estado.hijos.begin() + static_cast<std::ptrdiff_t>(elegido + 1),
                               partes.indices.begin() + 1, partes.indices.end());
            estado.llaves.insert(estado.llaves.begin() + static_cast<std::ptrdiff_t>(elegido),
                                 partes.pivotes.begin(), partes.pivotes.end());
        }
    }

    /** Entrada: hoja con buffer posiblemente desbordado. Salida: hojas y sus pivotes. */
    static void dividirHoja(const Estado& estado, std::vector<Estado>& fragmentos,
                            std::vector<std::int32_t>& pivotes) {
        const std::size_t cantidad = std::max<std::size_t>(1,
            (estado.buffer.size() + static_cast<std::size_t>(L) - 1) / static_cast<std::size_t>(L));
        const std::size_t base = estado.buffer.size() / cantidad;
        const std::size_t extra = estado.buffer.size() % cantidad;
        std::size_t inicio = 0;
        for (std::size_t i = 0; i < cantidad; ++i) {
            const std::size_t fin = inicio + base + (i < extra ? 1 : 0);
            Estado hoja;
            hoja.buffer.assign(estado.buffer.begin() + static_cast<std::ptrdiff_t>(inicio),
                              estado.buffer.begin() + static_cast<std::ptrdiff_t>(fin));
            if (i + 1 < cantidad) pivotes.push_back(hoja.buffer.back().llave);
            fragmentos.push_back(std::move(hoja));
            inicio = fin;
        }
    }

    /** Entrada: estado interno. Salida: nodos con a lo más C divisorias. */
    static void dividirInterno(const Estado& estado, std::vector<Estado>& fragmentos,
                               std::vector<std::int32_t>& pivotes) {
        if (estado.llaves.size() <= static_cast<std::size_t>(C)) {
            fragmentos.push_back(estado);
            return;
        }
        const std::size_t medio = estado.llaves.size() / 2;
        const std::int32_t pivote = estado.llaves[medio];
        Estado izquierda, derecha;
        izquierda.interno = derecha.interno = true;
        izquierda.llaves.assign(estado.llaves.begin(), estado.llaves.begin() + static_cast<std::ptrdiff_t>(medio));
        derecha.llaves.assign(estado.llaves.begin() + static_cast<std::ptrdiff_t>(medio + 1), estado.llaves.end());
        izquierda.hijos.assign(estado.hijos.begin(), estado.hijos.begin() + static_cast<std::ptrdiff_t>(medio + 1));
        derecha.hijos.assign(estado.hijos.begin() + static_cast<std::ptrdiff_t>(medio + 1), estado.hijos.end());
        const auto corte = std::upper_bound(estado.buffer.begin(), estado.buffer.end(), pivote,
                                          [](std::int32_t llave, const Par& par) { return llave < par.llave; });
        izquierda.buffer.assign(estado.buffer.begin(), corte);
        derecha.buffer.assign(corte, estado.buffer.end());
        dividirInterno(izquierda, fragmentos, pivotes);
        pivotes.push_back(pivote);
        dividirInterno(derecha, fragmentos, pivotes);
    }

    /** Entrada: bloque y rango. Salida: agrega pares del buffer y de hijos intersectados. */
    static void buscarEn(ArchivoNodos<Nodo>& archivo, std::size_t indice,
                         std::int32_t inferior, std::int32_t superior,
                         std::vector<Par>& resultado) {
        const Nodo nodo = archivo.leer(indice);
        for (int i = 0; i < nodo.k_buffer; ++i)
            if (inferior <= nodo.buffer[i].llave && nodo.buffer[i].llave <= superior)
                resultado.push_back(nodo.buffer[i]);
        if (!nodo.es_interno) return;
        for (int i = 0; i <= nodo.k_llaves; ++i) {
            const bool tocaIzquierda = i == 0 || superior >= nodo.llaves[i - 1];
            const bool tocaDerecha = i == nodo.k_llaves || inferior <= nodo.llaves[i];
            if (tocaIzquierda && tocaDerecha)
                buscarEn(archivo, static_cast<std::size_t>(nodo.hijos[i]),
                         inferior, superior, resultado);
        }
    }

    /** Entrada: bloque. Salida: pares en todos los buffers del subárbol. */
    static std::uint64_t contarEn(ArchivoNodos<Nodo>& archivo, std::size_t indice) {
        const Nodo nodo = archivo.leer(indice);
        std::uint64_t total = static_cast<std::uint64_t>(nodo.k_buffer);
        if (nodo.es_interno)
            for (int i = 0; i <= nodo.k_llaves; ++i)
                total += contarEn(archivo, static_cast<std::size_t>(nodo.hijos[i]));
        return total;
    }

    double alfa_;
    Disco<Nodo> disco_;
};
