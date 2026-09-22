#!/usr/bin/env python3
"""
graficos.py — Genera los 12 gráficos y la tabla de tiempos a partir de los CSV de ./prim.

Uso (desde Tarea1/):
    python3 scripts/graficos.py                      # lee resultados/tiempos_*.csv (sin los _red)
    python3 scripts/graficos.py --reducidos          # incluye corridas con --reducir (pruebas)
    python3 scripts/graficos.py --entrada dir --salida figuras

Requiere: matplotlib (pip install matplotlib).

Gráficos:
  Series A y B (costo total), 4 gráficos: tiempo total vs e (A) o v (B), por cola.
  Series C y D (costo amortizado), 8 gráficos: tiempo acumulado de decreaseKey
  y conteo de operaciones vs cantidad de llamadas, por cola.
Cada gráfico incluye la cota teórica multiplicada por la constante c que mejor
ajusta por mínimos cuadrados (c = sum(y*f) / sum(f^2)). Gráficos de una misma
serie y medida comparten escala (mismo eje y) para comparar ambas colas.
"""
import argparse
import csv
import glob
import math
import os
import statistics
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

COLOR_MEDIDO = "#2a5caa"
COLOR_COTA = "#8a8a8a"

# Cotas teóricas: f(v, e, llamadas)
COTAS_TOTAL = {
    "binomial": (lambda v, e: e * math.log2(v), r"$c \cdot e \log v$"),
    "fibonacci": (lambda v, e: e + v * math.log2(v), r"$c \cdot (e + v \log v)$"),
}
COTAS_DK = {  # costo acumulado de decreaseKey en función de la cantidad de llamadas k
    "binomial": (lambda v, k: k * math.log2(v), r"$c \cdot k \log v$"),
    "fibonacci": (lambda v, k: k, r"$c \cdot k$  (O(1) amortizado)"),
}


def leer(entrada, reducidos):
    """Lee todos los tiempos_*.csv. Retorna lista de dicts con tipos numéricos."""
    filas = []
    for ruta in sorted(glob.glob(os.path.join(entrada, "tiempos_*.csv"))):
        if "_red" in os.path.basename(ruta) and not reducidos:
            continue
        with open(ruta, newline="") as f:
            for r in csv.DictReader(f):
                for k in ("i", "j", "v", "e", "rep", "dk_llamadas", "dk_tiempo_ns", "dk_ops"):
                    r[k] = int(r[k])
                for k in ("tiempo_ms", "peso_mst"):
                    r[k] = float(r[k])
                filas.append(r)
    return filas


def agrupar(filas):
    """Agrupa por (serie, i, j, cola) y calcula promedio y desviación estándar."""
    g = defaultdict(list)
    for r in filas:
        g[(r["serie"], r["i"], r["j"], r["cola"])].append(r)
    res = {}
    for clave, rs in g.items():
        def prom(k):
            return statistics.mean(x[k] for x in rs)

        def desv(k):
            return statistics.stdev(x[k] for x in rs) if len(rs) > 1 else 0.0

        res[clave] = {
            "v": rs[0]["v"], "e": rs[0]["e"], "n": len(rs),
            "tiempo_ms": prom("tiempo_ms"), "tiempo_ms_sd": desv("tiempo_ms"),
            "dk_llamadas": prom("dk_llamadas"),
            "dk_tiempo_ms": prom("dk_tiempo_ns") / 1e6, "dk_tiempo_ms_sd": desv("dk_tiempo_ns") / 1e6,
            "dk_ops": prom("dk_ops"), "dk_ops_sd": desv("dk_ops"),
        }
    return res


def ajustar(ys, fs):
    """Constante c que minimiza sum (y - c f)^2."""
    den = sum(f * f for f in fs)
    return sum(y * f for y, f in zip(ys, fs)) / den if den else 0.0


def estilo(ax, xlabel, ylabel, titulo):
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(titulo, fontsize=11)
    ax.grid(True, color="#dddddd", linewidth=0.6)
    for lado in ("top", "right"):
        ax.spines[lado].set_visible(False)
    ax.legend(frameon=False)


def serie_datos(agr, serie, cola, xkey):
    """Puntos (x, y...) de una serie/cola ordenados por x."""
    pts = [(k, d) for k, d in agr.items() if k[0] == serie and k[3] == cola]
    pts.sort(key=lambda kd: kd[1][xkey])
    return pts


def graficar(salida, nombre, xs, ys, sds, cota_ys, cota_label, c, xlabel, ylabel, titulo, ylim, log2x):
    fig, ax = plt.subplots(figsize=(6, 4))
    ax.errorbar(xs, ys, yerr=sds, color=COLOR_MEDIDO, linewidth=2, marker="o", markersize=5,
                capsize=3, label="medido (promedio)")
    ax.plot(xs, cota_ys, color=COLOR_COTA, linewidth=2, linestyle="--",
            label=f"{cota_label},  c = {c:.3g}")
    if log2x:
        ax.set_xscale("log", base=2)
    ax.set_ylim(0, ylim)
    estilo(ax, xlabel, ylabel, titulo)
    fig.tight_layout()
    ruta = os.path.join(salida, nombre)
    fig.savefig(ruta, dpi=200)
    plt.close(fig)
    print("  ", ruta)


def graficos_total(agr, colas, salida):
    for serie, xkey, xlabel in (("A", "e", "e (aristas), v fijo"), ("B", "v", "v (vértices), e fijo")):
        datos = {}
        for cola in colas:
            pts = serie_datos(agr, serie, cola, xkey)
            if not pts or cola not in COTAS_TOTAL:
                continue
            f, lab = COTAS_TOTAL[cola]
            xs = [d[xkey] for _, d in pts]
            ys = [d["tiempo_ms"] for _, d in pts]
            sds = [d["tiempo_ms_sd"] for _, d in pts]
            fs = [f(d["v"], d["e"]) for _, d in pts]
            c = ajustar(ys, fs)
            datos[cola] = (xs, ys, sds, [c * x for x in fs], lab, c)
        if not datos:
            continue
        ylim = 1.08 * max(max(max(y + s for y, s in zip(d[1], d[2])), max(d[3])) for d in datos.values())
        for cola, (xs, ys, sds, cy, lab, c) in datos.items():
            graficar(salida, f"total_{cola}_serie{serie}.png", xs, ys, sds, cy, lab, c, xlabel,
                     "tiempo total de Prim [ms]", f"Serie {serie} — Prim con cola {cola}", ylim, True)


def graficos_amortizado(agr, colas, salida):
    medidas = (("tiempo", "dk_tiempo_ms", "dk_tiempo_ms_sd", "tiempo acumulado de decreaseKey [ms]"),
               ("ops", "dk_ops", "dk_ops_sd", "operaciones (intercambios / cortes)"))
    for serie in ("C", "D"):
        for medida, ykey, sdkey, ylabel in medidas:
            datos = {}
            for cola in colas:
                pts = serie_datos(agr, serie, cola, "dk_llamadas")
                if not pts or cola not in COTAS_DK:
                    continue
                f, lab = COTAS_DK[cola]
                xs = [d["dk_llamadas"] for _, d in pts]
                ys = [d[ykey] for _, d in pts]
                sds = [d[sdkey] for _, d in pts]
                fs = [f(d["v"], d["dk_llamadas"]) for _, d in pts]
                c = ajustar(ys, fs)
                datos[cola] = (xs, ys, sds, [c * x for x in fs], lab, c)
            if not datos:
                continue
            ylim = 1.08 * max(max(max(y + s for y, s in zip(d[1], d[2])), max(d[3])) for d in datos.values())
            if ylim == 0:
                ylim = 1
            for cola, (xs, ys, sds, cy, lab, c) in datos.items():
                graficar(salida, f"dk_{medida}_{cola}_serie{serie}.png", xs, ys, sds, cy, lab, c,
                         "llamadas a decreaseKey", ylabel, f"Serie {serie} — decreaseKey, cola {cola}",
                         ylim, True)


def tabla(agr, salida):
    """Escribe tabla_tiempos.csv y tabla_tiempos.tex (series A y B) e imprime en markdown."""
    filas = sorted((k, d) for k, d in agr.items() if k[0] in ("A", "B"))
    if not filas:
        return
    with open(os.path.join(salida, "tabla_tiempos.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["serie", "i", "j", "cola", "reps", "tiempo_ms_prom", "tiempo_ms_desv"])
        for (s, i, j, cola), d in filas:
            w.writerow([s, i, j, cola, d["n"], f"{d['tiempo_ms']:.3f}", f"{d['tiempo_ms_sd']:.3f}"])
    with open(os.path.join(salida, "tabla_tiempos.tex"), "w") as f:
        f.write("\\begin{tabular}{cccrr}\n\\hline\nSerie & $i$ & $j$ & Cola & Tiempo [ms] \\\\\n\\hline\n")
        for (s, i, j, cola), d in filas:
            f.write(f"{s} & {i} & {j} & {cola} & ${d['tiempo_ms']:.1f} \\pm {d['tiempo_ms_sd']:.1f}$ \\\\\n")
        f.write("\\hline\n\\end{tabular}\n")
    print("\n| serie | i | j | cola | reps | tiempo [ms] |\n|---|---|---|---|---|---|")
    for (s, i, j, cola), d in filas:
        print(f"| {s} | {i} | {j} | {cola} | {d['n']} | {d['tiempo_ms']:.2f} ± {d['tiempo_ms_sd']:.2f} |")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--entrada", default="resultados")
    ap.add_argument("--salida", default="figuras")
    ap.add_argument("--reducidos", action="store_true", help="incluir archivos *_red*.csv")
    a = ap.parse_args()

    filas = leer(a.entrada, a.reducidos)
    if not filas:
        print(f"No hay datos en {a.entrada}/tiempos_*.csv")
        return
    os.makedirs(a.salida, exist_ok=True)
    colas = sorted({r["cola"] for r in filas})
    agr = agrupar(filas)
    print("Gráficos:")
    graficos_total(agr, colas, a.salida)
    graficos_amortizado(agr, colas, a.salida)
    tabla(agr, a.salida)


if __name__ == "__main__":
    main()
