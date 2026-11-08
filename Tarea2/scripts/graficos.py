#!/usr/bin/env python3
"""Genera gráficos y tablas a partir de resultados/metricas.csv y de las búsquedas guardadas."""

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.dates as mdates  # noqa: E402
import matplotlib.pyplot as plt  # noqa: E402

try:  # Hora de Chile para el gráfico de temperatura; si no hay base de zonas, se usa UTC.
    from zoneinfo import ZoneInfo

    ZONA = ZoneInfo("America/Santiago")
    NOMBRE_ZONA = "hora de Chile"
except Exception:  # noqa: BLE001
    ZONA = timezone.utc
    NOMBRE_ZONA = "UTC"

# Nombre mostrado, color y marcador de cada árbol (columna "arbol" de metricas.csv).
ARBOLES = {
    "B": ("Árbol B (b = 340)", "#2a5caa", "o"),
    "Be025": ("Bε, ε ≈ 0,25 (c = 4)", "#e07b39", "s"),
    "Be075": ("Bε, ε ≈ 0,75 (c = 126)", "#3a9e5c", "^"),
}
COLOR_PUNTOS = "#2a5caa"

# Cada gráfico: función que obtiene el valor de una fila, etiqueta del eje, archivo y escala log en y.
METRICAS = (
    (lambda f: float(f["construccion_ms"]) / 1000, "tiempo de construcción [s]", "construccion.png", True),
    (lambda f: float(f["ios_construccion"]), "I/Os de construcción (lecturas + escrituras)",
     "ios_construccion.png", True),
    (lambda f: float(f["ios_construccion"]) / float(f["N"]), "I/Os de construcción por inserción",
     "ios_por_insercion.png", False),
    (lambda f: float(f["bytes"]) / 2**20, "tamaño del árbol [MiB]", "tamano.png", True),
    (lambda f: float(f["busqueda_promedio_ms"]), "tiempo promedio de búsqueda [ms]", "busqueda.png", True),
    (lambda f: float(f["ios_busqueda_promedio"]), "I/Os promedio de búsqueda", "ios_busqueda.png", True),
)


def leer_metricas(ruta):
    """Recibe la ruta de metricas.csv y devuelve sus filas agrupadas por árbol y ordenadas por N."""
    agrupadas = {}
    with ruta.open(newline="", encoding="utf-8") as archivo:
        for fila in csv.DictReader(archivo):
            agrupadas.setdefault(fila["arbol"], []).append(fila)
    for filas in agrupadas.values():
        filas.sort(key=lambda fila: int(fila["exponente"]))
    return agrupadas


def estilo(ax, xlabel, ylabel, titulo):
    """Añade etiquetas, título, grilla y leyenda a los ejes ax usando los textos recibidos."""
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(titulo, fontsize=11)
    ax.grid(True, color="#dddddd", linewidth=0.6, which="major")
    for lado in ("top", "right"):
        ax.spines[lado].set_visible(False)
    if ax.get_legend_handles_labels()[0]:
        ax.legend(frameon=False)


def graficar_metricas(agrupadas, salida):
    """Recibe las series por árbol y guarda en salida un gráfico por métrica, con N en escala log2."""
    for valor, ylabel, nombre, logy in METRICAS:
        fig, ax = plt.subplots(figsize=(6, 4))
        for arbol in [a for a in ARBOLES if a in agrupadas] + [a for a in agrupadas if a not in ARBOLES]:
            filas = agrupadas[arbol]
            etiqueta, color, marcador = ARBOLES.get(arbol, (arbol, None, "o"))
            ax.plot([int(f["N"]) for f in filas], [valor(f) for f in filas], color=color,
                    marker=marcador, markersize=5, linewidth=2, label=etiqueta)
        ax.set_xscale("log", base=2)
        if logy:
            ax.set_yscale("log")
        else:
            ax.set_ylim(bottom=0)
        estilo(ax, "N (pares insertados)", ylabel, ylabel[0].upper() + ylabel[1:].split(" [")[0])
        fig.tight_layout()
        fig.savefig(salida / nombre, dpi=200)
        plt.close(fig)
        print("  ", salida / nombre)


def escribir_tabla(agrupadas, salida):
    """Resume metricas.csv en salida/tabla_metricas.csv y .tex, con las I/Os por inserción."""
    filas = [(arbol, f) for arbol in ARBOLES if arbol in agrupadas for f in agrupadas[arbol]]
    filas.sort(key=lambda af: (int(af[1]["exponente"]), list(ARBOLES).index(af[0])))
    columnas = ["arbol", "exponente", "N", "construccion_s", "ios_por_insercion", "nodos", "mib",
                "busqueda_ms", "ios_busqueda", "hallados"]
    with (salida / "tabla_metricas.csv").open("w", newline="", encoding="utf-8") as archivo:
        w = csv.writer(archivo)
        w.writerow(columnas)
        for arbol, f in filas:
            n = float(f["N"])
            w.writerow([arbol, f["exponente"], f["N"], f"{float(f['construccion_ms']) / 1000:.3f}",
                        f"{float(f['ios_construccion']) / n:.3f}", f["nodos"],
                        f"{float(f['bytes']) / 2**20:.1f}", f"{float(f['busqueda_promedio_ms']):.4f}",
                        f"{float(f['ios_busqueda_promedio']):.2f}", f"{float(f['hallados_promedio']):.2f}"])
    with (salida / "tabla_metricas.tex").open("w", encoding="utf-8") as archivo:
        archivo.write("\\begin{tabular}{llrrrrrr}\n\\hline\n"
                      "Árbol & $N$ & Constr. [s] & I/O/ins. & Nodos & MiB & Búsq. [ms] & I/O búsq. \\\\\n"
                      "\\hline\n")
        for arbol, f in filas:
            n = float(f["N"])
            archivo.write(f"{arbol} & $2^{{{f['exponente']}}}$ & {float(f['construccion_ms']) / 1000:.3f} & "
                          f"{float(f['ios_construccion']) / n:.3f} & {f['nodos']} & "
                          f"{float(f['bytes']) / 2**20:.1f} & {float(f['busqueda_promedio_ms']):.4f} & "
                          f"{float(f['ios_busqueda_promedio']):.2f} \\\\\n")
        archivo.write("\\hline\n\\end{tabular}\n")
    print("  ", salida / "tabla_metricas.csv")


def leer_curva(ruta):
    """Recibe el CSV de una búsqueda y devuelve las fechas (hora de Chile) y las temperaturas."""
    with ruta.open(newline="", encoding="utf-8") as archivo:
        filas = list(csv.DictReader(archivo))
    return ([datetime.fromtimestamp(int(f["llave"]), ZONA) for f in filas],
            [float(f["temperatura"]) for f in filas])


def graficar_temperatura(carpeta, salida):
    """Compara la primera búsqueda del menor y el mayor N disponibles, con el mismo eje de fechas."""
    curvas = sorted(carpeta.glob("busqueda_b_*.csv"), key=lambda ruta: int(ruta.stem.rsplit("_", 1)[1]))
    if len(curvas) < 2:
        return
    fig, ejes = plt.subplots(2, 1, figsize=(8, 6), sharex=True, sharey=True)
    for ax, ruta in zip(ejes, (curvas[0], curvas[-1])):
        fechas, temperaturas = leer_curva(ruta)
        exponente = ruta.stem.rsplit("_", 1)[1]
        ax.scatter(fechas, temperaturas, s=6 if len(fechas) < 1000 else 0.5, color=COLOR_PUNTOS,
                   alpha=0.9 if len(fechas) < 1000 else 0.35, linewidths=0)
        estilo(ax, "", "temperatura [°C]", f"$N = 2^{{{exponente}}}$: {len(fechas):,} pares".replace(",", "."))
    ejes[-1].xaxis.set_major_locator(mdates.DayLocator(tz=ZONA))
    ejes[-1].xaxis.set_major_formatter(mdates.DateFormatter("%d-%m", tz=ZONA))
    ejes[-1].set_xlabel(f"fecha de la medición ({NOMBRE_ZONA})")
    fig.tight_layout()
    fig.savefig(salida / "temperaturas_dos_n.png", dpi=200)
    plt.close(fig)
    print("  ", salida / "temperaturas_dos_n.png")


def graficar_bonus(carpeta, salida):
    """Recibe la carpeta con bonus.csv (si existe) y compara carga y búsqueda para cada α."""
    ruta = carpeta / "bonus.csv"
    if not ruta.exists():
        return
    with ruta.open(newline="", encoding="utf-8") as archivo:
        filas = sorted(csv.DictReader(archivo), key=lambda fila: float(fila["alfa"]))
    if not filas:
        return
    alfa = [float(f["alfa"]) for f in filas]
    fig, ejes = plt.subplots(2, 1, figsize=(6, 6), sharex=True)
    ejes[0].plot(alfa, [float(f["construccion_ms"]) / 1000 for f in filas], marker="o", color=COLOR_PUNTOS)
    estilo(ejes[0], "", "construcción [s]", f"Bonus: {filas[0]['arbol']}, N = {filas[0]['N']}")
    ejes[1].plot(alfa, [float(f["busqueda_promedio_ms"]) for f in filas], marker="o", color=COLOR_PUNTOS)
    estilo(ejes[1], "α", "búsqueda promedio [ms]", "")
    fig.tight_layout()
    fig.savefig(salida / "bonus_alfa.png", dpi=200)
    plt.close(fig)
    print("  ", salida / "bonus_alfa.png")


def main():
    """Lee los argumentos y coordina la generación de gráficos y tablas en la carpeta de salida."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--metricas", type=Path, default=Path("resultados/metricas.csv"))
    parser.add_argument("--salida", type=Path, default=Path("figuras"))
    args = parser.parse_args()
    args.salida.mkdir(parents=True, exist_ok=True)
    agrupadas = leer_metricas(args.metricas)
    print("Gráficos:")
    graficar_metricas(agrupadas, args.salida)
    graficar_temperatura(args.metricas.parent, args.salida)
    graficar_bonus(args.metricas.parent, args.salida)
    escribir_tabla(agrupadas, args.salida)


if __name__ == "__main__":
    main()
