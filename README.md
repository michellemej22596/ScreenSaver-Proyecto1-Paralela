# ScreenSaver-Proyecto1-Paralela
Silvia Illescas, Isabella Miralles y Michelle Mejia

Proyecto base para un screensaver de círculos con rebotes, colores fríos y física simple.
Versión inicial **secuencial** en C++/SDL2. Posteriormente se paralelizará con OpenMP.

## Requisitos

### Ubuntu / WSL
```bash
sudo apt update
sudo apt install -y build-essential libsdl2-dev
````

> Nota: No usamos SDL2\_gfx para evitar dependencias extra; el círculo se rasteriza con función propia.

### Compilación

```bash
make
```

El binario se genera en `build/screensaver`.

### Ejecución

```bash
./build/screensaver N [width height]
# Ejemplos:
./build/screensaver 100
./build/screensaver 250 1024 768
```

**Parámetros**

* `N` = número de círculos (obligatorio, entero > 0)
* `width`, `height` = tamaño de ventana (opcionales, por defecto 800x600)

### Controles

* Cerrar ventana o `ESC` para salir.

## Estructura

```
src/main.cpp          # App principal
include/              # (para headerss)
assets/
tests/                # (scripts de medición)
```

## Notas

* Programación defensiva: valida argumentos, evita hardcodear constantes.
* Paleta de colores: azules/celestes/blancos, con variación aleatoria.
* FPS objetivo \~60 con delta time para animación fluida.
