# ScreenSaver-Proyecto1-Paralela
Silvia Illescas, Isabella Miralles y Michelle Mejia

````markdown
# 🎆 Screensaver Interactivo en C++ con SDL2 (Versión Secuencial y Paralela)

Este proyecto implementa un *screensaver* animado con partículas en movimiento, desarrollado en **C++** utilizando la biblioteca **SDL2**. Se presentan dos versiones: una **secuencial** y otra **paralela con OpenMP**, permitiendo analizar diferencias de rendimiento entre ambas.

---

## 📂 Archivos principales

- `main.cpp`: versión secuencial del screensaver
- `main_par.cpp`: versión paralela del screensaver con OpenMP
- `Makefile`: permite compilar ambas versiones fácilmente
- `README.md`: documentación del proyecto

---

## ⚙️ Compilación

Usa el siguiente comando para compilar ambas versiones:

```bash
make
````

Esto generará los ejecutables `screensaver_seq` y `screensaver_par`.

También puedes compilar por separado:

```bash
make seq      # Compila versión secuencial
make par      # Compila versión paralela
make clean    # Limpia archivos .o y ejecutables
```

---

## 🚀 Ejecución

Ambos programas aceptan parámetros desde línea de comandos:

```bash
./screensaver_seq N ANCHO ALTO [FPS]
./screensaver_par N ANCHO ALTO HILOS [FPS]
```

* `N`: número de partículas (default: 200)
* `ANCHO`: ancho de la ventana (mínimo 640)
* `ALTO`: alto de la ventana (mínimo 480)
* `HILOS`: número de threads (solo en versión paralela)
* `FPS`: cuadros por segundo (opcional)

---

### 🧪 Ejemplos

```bash
./screensaver_seq 300 1024 768 60
./screensaver_par 600 1280 720 8 60
```

---

## ✨ Funcionalidades implementadas

* Fondo animado con cambio cíclico de color (suave y dinámico)
* Simulación física de partículas con rebote en los bordes
* Atracción al centro de la ventana
* Repulsión desde el mouse al hacer clic (interacción)
* Transparencia de partículas usando texturas circulares
* Gradiente de color RGB animado por partícula
* Parametrización completa desde la línea de comandos
* Versión paralela con OpenMP y control de hilos

---

## 📌 Requisitos

* Compilador `g++` compatible con C++17
* Librería `SDL2` instalada en el sistema
* Para la versión paralela: soporte OpenMP

---


