#pragma once
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>

/**
 * datos.h — Lectura de datos.bin (sección 2 del enunciado).
 *
 * datos.bin guarda 2^26 pares desordenados: un int de 4 bytes con el tiempo Unix de la medición
 * y un float de 4 bytes con la temperatura. datos_sample.txt trae los primeros 50 pares en texto
 * y sirve para verificar que el binario se lee correctamente.
 */

/** Par llave-valor del dataset y de los nodos. */
struct Par {
    std::int32_t llave;  // tiempo Unix de la medición, en segundos
    float valor;         // temperatura medida en ese instante, en °C
};
static_assert(sizeof(Par) == 8, "Un par debe ocupar 8 bytes");
static_assert(std::is_trivially_copyable<Par>::value, "Par debe poder serializarse");

/** Lee pares consecutivos de datos.bin sin cargar el dataset completo en RAM. */
class Datos {
public:
    /** Entrada: ruta del binario. Salida: lector abierto. */
    explicit Datos(const std::string& ruta) : entrada_(ruta, std::ios::binary) {
        if (!entrada_) throw std::runtime_error("No se pudo abrir " + ruta);
    }

    /** Salida: siguiente par, o false si terminó el archivo; falla ante un par incompleto. */
    bool siguiente(Par& par) {
        entrada_.read(reinterpret_cast<char*>(&par), sizeof(Par));
        if (entrada_) return true;
        if (entrada_.eof() && entrada_.gcount() == 0) return false;
        throw std::runtime_error("datos.bin termina con un par incompleto");
    }

private:
    std::ifstream entrada_;  // datos.bin abierto en modo binario
};

/**
 * Compara los primeros pares del binario con la muestra textual (redondeada a 4 decimales).
 * Entrada: rutas de datos.bin y datos_sample.txt. Salida: cantidad de pares verificados;
 * lanza runtime_error si algún par no coincide.
 */
inline std::size_t verificarMuestra(const std::string& binario, const std::string& muestra) {
    Datos datos(binario);
    std::ifstream texto(muestra);
    if (!texto) throw std::runtime_error("No se pudo abrir " + muestra);
    std::string encabezado;
    std::getline(texto, encabezado);
    std::size_t verificados = 0;
    std::int32_t llave;
    float valor;
    while (texto >> llave >> valor) {
        Par par{};
        if (!datos.siguiente(par) || par.llave != llave || std::fabs(par.valor - valor) > 0.000051f)
            throw std::runtime_error("La muestra no coincide en el par " + std::to_string(verificados));
        ++verificados;
    }
    if (verificados == 0 || (!texto.eof() && texto.fail()))
        throw std::runtime_error("Muestra vacía o mal formada");
    return verificados;
}
