#include <SDL2/SDL.h>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include "timing_helpers.h"

// Estructura de una partícula
struct Particle {
    float x, y;           // Posición
    float vx, vy;         // Velocidad
    float ax, ay;         // Aceleración
    int r;                // Radio
    Uint8 cr, cg, cb;     // Color RGB
    Uint8 alpha;          // Opacidad
};

// Configuración del programa
struct Config {
    int N = 200;      // Número de partículas
    int width = 800;  // Ancho de ventana
    int height = 600; // Alto de ventana
    int threads = 4;  // Hilos para OpenMP
    int fps = 60;     // Cuadros por segundo
};

// Parseo de argumentos desde terminal
static Config parseArgs(int argc, char** argv) {
    Config cfg;
    if (argc > 1) cfg.N = std::stoi(argv[1]);
    if (argc > 2) cfg.width = std::stoi(argv[2]);
    if (argc > 3) cfg.height = std::stoi(argv[3]);
    if (argc > 4) cfg.threads = std::stoi(argv[4]);
    if (argc > 5) cfg.fps = std::stoi(argv[5]);
    if (cfg.width < 640) cfg.width = 640;
    if (cfg.height < 480) cfg.height = 480;
    if (cfg.threads < 1) cfg.threads = 1;
    if (cfg.fps < 1) cfg.fps = 60;
    return cfg;
}

// Genera una textura circular con canal alpha (transparencia)
SDL_Texture* createCircleTexture(SDL_Renderer* ren, int r) {
    int size = r * 2;
    Uint32 pixel_format = SDL_PIXELFORMAT_RGBA32;
    void* pixels = malloc(size * size * 4);
    if (!pixels) return nullptr;
    Uint8* px = (Uint8*)pixels;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - r;
            int dy = y - r;
            float dist = std::sqrt(dx * dx + dy * dy);
            Uint8 a = 0;
            if (dist <= r) {
                float t = (1.0f - dist / (float)r);
                a = (Uint8)(255 * t * t); // gradiente suave
            }
            int idx = (y * size + x) * 4;
            px[idx + 0] = 255; // R
            px[idx + 1] = 255; // G
            px[idx + 2] = 255; // B
            px[idx + 3] = a;   // Alpha
        }
    }

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, pixel_format);
    if (!surf) { free(pixels); return nullptr; }
    memcpy(surf->pixels, pixels, size * size * 4);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surf);
    free(pixels);
    return tex;
}

int main(int argc, char** argv) {
    Config cfg = parseArgs(argc, argv);
    omp_set_num_threads(cfg.threads); // Configura número de hilos para OpenMP

    int frames = 500;
    if (argc >= 7) frames = atoi(argv[6]);

    double t_start = now_seconds(); // Tiempo inicial

    // Inicializa SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init error\n";
        return 1;
    }

    // Crear ventana y renderer
    SDL_Window* win = SDL_CreateWindow("Screensaver Paralelo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg.width, cfg.height, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Generadores de números aleatorios
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> ux(0.0f, (float)cfg.width);
    std::uniform_real_distribution<float> uy(0.0f, (float)cfg.height);
    std::uniform_real_distribution<float> uv(-120.0f, 120.0f);
    std::uniform_int_distribution<int> ur(3, 20);
    std::uniform_int_distribution<int> uc(0, 255);

    // Crear partículas
    std::vector<Particle> particles(cfg.N);
    for (auto& p : particles) {
        p.r = ur(rng);
        p.x = ux(rng); p.y = uy(rng);
        p.vx = uv(rng) * 0.01f; p.vy = uv(rng) * 0.01f;
        p.ax = p.ay = 0.0f;
        p.cr = uc(rng); p.cg = uc(rng); p.cb = uc(rng);
        p.alpha = 160 + uc(rng) % 96;
    }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    // Texturas por radio
    std::unordered_map<int, SDL_Texture*> tex_by_r;
    for (int r = 3; r <= 20; r++) tex_by_r[r] = createCircleTexture(ren, r);

    // Variables de control
    bool running = true;
    SDL_Event ev;
    auto last = std::chrono::steady_clock::now();
    double accumulator = 0.0;
    const double dt_fixed = 1.0 / 60.0;
    int mouseX = -1, mouseY = -1;
    bool mouseClick = false;
    double acc_update_time = 0.0;
    int frame_counter = 0;

    // Bucle principal
    while (running && frame_counter < frames) {
        // Manejo de eventos
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) running = false;
            else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                mouseClick = true;
                SDL_GetMouseState(&mouseX, &mouseY);
            }
        }

        // Control de tiempo
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last;
        last = now;
        accumulator += elapsed.count();

        // Actualización de simulación
        while (accumulator >= dt_fixed) {
            double update_s = now_seconds();

            // Actualizar partículas en paralelo
            #pragma omp parallel for schedule(dynamic)
            for (size_t i = 0; i < particles.size(); i++) {
                auto& p = particles[i];
                float cx = cfg.width * 0.5f;
                float cy = cfg.height * 0.5f;
                float dx = cx - p.x, dy = cy - p.y;
                float dist = std::sqrt(dx * dx + dy * dy) + 1e-5f;
                float pull = 20.0f / dist;
                p.ax = dx / dist * pull * 0.02f;
                p.ay = dy / dist * pull * 0.02f;

                // Repulsión por mouse
                if (mouseClick) {
                    float dxm = mouseX - p.x;
                    float dym = mouseY - p.y;
                    float distSq = dxm * dxm + dym * dym;
                    float maxDist = 100.0f;
                    if (distSq < maxDist * maxDist) {
                        float factor = (1.0f - std::sqrt(distSq) / maxDist) * 0.5f;
                        float angle = std::atan2(dym, dxm);
                        float push = factor * 8.0f;
                        p.vx -= std::cos(angle) * push;
                        p.vy -= std::sin(angle) * push;
                    }
                }

                // Movimiento
                p.vx += p.ax * dt_fixed;
                p.vy += p.ay * dt_fixed;
                p.vx *= 0.9995f;
                p.vy *= 0.9995f;
                p.x += p.vx * dt_fixed * 60.0f;
                p.y += p.vy * dt_fixed * 60.0f;

                // Rebotes
                if (p.x < p.r) { p.x = p.r; p.vx = -p.vx * 0.9f; }
                else if (p.x > cfg.width - p.r) { p.x = cfg.width - p.r; p.vx = -p.vx * 0.9f; }
                if (p.y < p.r) { p.y = p.r; p.vy = -p.vy * 0.9f; }
                else if (p.y > cfg.height - p.r) { p.y = cfg.height - p.r; p.vy = -p.vy * 0.9f; }
            }

            acc_update_time += (now_seconds() - update_s);
            accumulator -= dt_fixed;
            mouseClick = false;
        }

        // Fondo animado
        float tbg = SDL_GetTicks() / 2000.0f;
        Uint8 rbg = Uint8(60 + 40 * std::sin(tbg));
        Uint8 gbg = Uint8(30 + 30 * std::sin(tbg + 2.0f));
        Uint8 bbg = Uint8(80 + 50 * std::cos(tbg));
        SDL_SetRenderDrawColor(ren, rbg, gbg, bbg, 40);
        SDL_Rect full = { 0, 0, cfg.width, cfg.height };
        SDL_RenderFillRect(ren, &full);

        // Renderizado de partículas
        for (size_t i = 0; i < particles.size(); ++i) {
            auto& p = particles[i];
            float t = SDL_GetTicks() / 1000.0f;
            float speed = 0.9f;
            float hue = fmod(t * speed + i * 0.02f, 1.0f);
            float r = std::abs(std::sin(hue * 2 * M_PI));
            float g = std::abs(std::sin((hue + 0.33f) * 2 * M_PI));
            float b = std::abs(std::sin((hue + 0.66f) * 2 * M_PI));
            p.cr = Uint8(255 * r);
            p.cg = Uint8(255 * g);
            p.cb = Uint8(255 * b);

            SDL_Texture* tex = tex_by_r[p.r];
            SDL_SetTextureColorMod(tex, p.cr, p.cg, p.cb);
            SDL_SetTextureAlphaMod(tex, p.alpha);
            SDL_Rect dst = { int(p.x - p.r), int(p.y - p.r), p.r * 2, p.r * 2 };
            SDL_RenderCopy(ren, tex, nullptr, &dst);
        }

        SDL_RenderPresent(ren);
        frame_counter++;
    }

    // Medición de tiempo total y tiempo de actualización
    double t_end = now_seconds();
    double elapsed = t_end - t_start;
    printf("TIME_TOTAL %f\n", elapsed);
    printf("TIME_UPDATE %f\n", acc_update_time);

    // Bucle final: fondo y partículas animadas hasta que el usuario cierre
    bool keepRunning = true;
    SDL_Event finalEv;
    mouseClick = false;

    while (keepRunning) {
        while (SDL_PollEvent(&finalEv)) {
            if (finalEv.type == SDL_QUIT) keepRunning = false;
            else if (finalEv.type == SDL_KEYDOWN && finalEv.key.keysym.sym == SDLK_ESCAPE) keepRunning = false;
            else if (finalEv.type == SDL_MOUSEBUTTONDOWN && finalEv.button.button == SDL_BUTTON_LEFT) {
                mouseClick = true;
                SDL_GetMouseState(&mouseX, &mouseY);
            }
        }

        // Fondo animado
        float tbg = SDL_GetTicks() / 2000.0f;
        Uint8 rbg = Uint8(60 + 40 * std::sin(tbg));
        Uint8 gbg = Uint8(30 + 30 * std::sin(tbg + 2.0f));
        Uint8 bbg = Uint8(80 + 50 * std::cos(tbg));
        SDL_SetRenderDrawColor(ren, rbg, gbg, bbg, 40);
        SDL_RenderFillRect(ren, nullptr);

        // Actualizar partículas (sin cronómetro ni rendimiento)
        for (auto& p : particles) {
            float cx = cfg.width * 0.5f;
            float cy = cfg.height * 0.5f;
            float dx = cx - p.x, dy = cy - p.y;
            float dist = std::sqrt(dx * dx + dy * dy) + 1e-5f;
            float pull = 20.0f / dist;
            p.ax = dx / dist * pull * 0.02f;
            p.ay = dy / dist * pull * 0.02f;

            // Repulsión por mouse
            if (mouseClick) {
                float dxm = mouseX - p.x;
                float dym = mouseY - p.y;
                float distSq = dxm * dxm + dym * dym;
                float maxDist = 100.0f;
                if (distSq < maxDist * maxDist) {
                    float factor = (1.0f - std::sqrt(distSq) / maxDist) * 0.5f;
                    float angle = std::atan2(dym, dxm);
                    float push = factor * 8.0f;
                    p.vx -= std::cos(angle) * push;
                    p.vy -= std::sin(angle) * push;
                }
            }

            // Movimiento suave
            p.vx += p.ax * dt_fixed;
            p.vy += p.ay * dt_fixed;
            p.vx *= 0.9995f;
            p.vy *= 0.9995f;
            p.x += p.vx * dt_fixed * 60.0f;
            p.y += p.vy * dt_fixed * 60.0f;

            // Rebotes
            if (p.x < p.r) { p.x = p.r; p.vx = -p.vx * 0.9f; }
            else if (p.x > cfg.width - p.r) { p.x = cfg.width - p.r; p.vx = -p.vx * 0.9f; }
            if (p.y < p.r) { p.y = p.r; p.vy = -p.vy * 0.9f; }
            else if (p.y > cfg.height - p.r) { p.y = cfg.height - p.r; p.vy = -p.vy * 0.9f; }
        }

        // Render de partículas
        for (size_t i = 0; i < particles.size(); ++i) {
            auto& p = particles[i];
            float t = SDL_GetTicks() / 1000.0f;
            float speed = 0.9f;
            float hue = fmod(t * speed + i * 0.02f, 1.0f);
            float r = std::abs(std::sin(hue * 2 * M_PI));
            float g = std::abs(std::sin((hue + 0.33f) * 2 * M_PI));
            float b = std::abs(std::sin((hue + 0.66f) * 2 * M_PI));
            p.cr = Uint8(255 * r);
            p.cg = Uint8(255 * g);
            p.cb = Uint8(255 * b);

            SDL_Texture* tex = tex_by_r[p.r];
            SDL_SetTextureColorMod(tex, p.cr, p.cg, p.cb);
            SDL_SetTextureAlphaMod(tex, p.alpha);
            SDL_Rect dst = { int(p.x - p.r), int(p.y - p.r), p.r * 2, p.r * 2 };
            SDL_RenderCopy(ren, tex, nullptr, &dst);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);  // ~60 FPS
        mouseClick = false;
    }
    
}
