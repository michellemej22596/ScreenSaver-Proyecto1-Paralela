#include <SDL2/SDL.h>
#include <omp.h> 
#include <cstdio>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <string>
#include <stdexcept>

static void drawFilledCircle(SDL_Renderer* ren, int cx, int cy, int r) {
    for (int dy = -r; dy <= r; ++dy) {
        int dx = static_cast<int>(std::floor(std::sqrt((double)r*r - dy*dy)));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

struct Circle {
    float x, y;
    float vx, vy;
    int   r;
    SDL_Color color;
};

struct Params {
    int N = 100;
    int width = 800;
    int height = 600;
};

static SDL_Color randomCoolColor(std::mt19937& rng) {
    std::uniform_int_distribution<int> base(0, 255);
    int b = 180 + base(rng) % 76;
    int g = 160 + base(rng) % 96;
    int r = base(rng) % 80;
    if ((base(rng) % 10) == 0) { r = g = b = 230 + base(rng) % 26; }
    return SDL_Color{ Uint8(r), Uint8(g), Uint8(b), 255 };
}

static Params parseArgs(int argc, char** argv) {
    Params p;
    if (argc < 2) {
        std::fprintf(stderr, "Uso: %s N [width height]\n", argv[0]);
        std::fprintf(stderr, "Ej:  %s 200 1024 768\n", argv[0]);
        throw std::runtime_error("Argumentos insuficientes");
    }
    try {
        p.N = std::stoi(argv[1]);
        if (p.N <= 0) throw std::runtime_error("N debe ser > 0");
        if (argc >= 3) p.width = std::stoi(argv[2]);
        if (argc >= 4) p.height = std::stoi(argv[3]);
        if (p.width < 640 || p.height < 480)
            throw std::runtime_error("Resolución mínima: 640x480");
    } catch (...) {
        throw std::runtime_error("Argumentos inválidos");
    }
    return p;
}

int main(int argc, char** argv) {
    Params cfg;
    try {
        cfg = parseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(
        "Screensaver Paralelo (OpenMP)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg.width, cfg.height, SDL_WINDOW_SHOWN
    );
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!ren) {
        std::fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    std::mt19937 rng(
        (uint32_t)std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );
    std::uniform_real_distribution<float> distX(20.0f, cfg.width  - 20.0f);
    std::uniform_real_distribution<float> distY(20.0f, cfg.height - 20.0f);
    std::uniform_real_distribution<float> distV(80.0f, 220.0f);
    std::uniform_int_distribution<int>   distR(6, 14);

    std::vector<Circle> circles;
    circles.reserve(cfg.N);
    for (int i = 0; i < cfg.N; ++i) {
        Circle c;
        c.x = distX(rng);
        c.y = distY(rng);
        float angle = std::uniform_real_distribution<float>(0.0f, 6.2831853f)(rng);
        float speed = distV(rng);
        c.vx = std::cos(angle) * speed;
        c.vy = std::sin(angle) * speed;
        c.r = distR(rng);
        c.color = randomCoolColor(rng);
        circles.push_back(c);
    }

    bool running = true;
    Uint32 lastTicks = SDL_GetTicks();
    const float targetFPS = 60.0f;
    const float maxDelta = 1.0f / 30.0f;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        lastTicks = now;
        if (dt > maxDelta) dt = maxDelta;

        #pragma omp parallel for
        for (int i = 0; i < (int)circles.size(); ++i) {
            Circle& c = circles[i];
            c.x += c.vx * dt;
            c.y += c.vy * dt;

            if (c.x < c.r) {
                c.x = (float)c.r;
                c.vx = -c.vx;
            } else if (c.x > cfg.width - c.r) {
                c.x = (float)(cfg.width - c.r);
                c.vx = -c.vx;
            }
            if (c.y < c.r) {
                c.y = (float)c.r;
                c.vy = -c.vy;
            } else if (c.y > cfg.height - c.r) {
                c.y = (float)(cfg.height - c.r);
                c.vy = -c.vy;
            }
        }

        SDL_SetRenderDrawColor(ren, 5, 10, 20, 255);
        SDL_RenderClear(ren);

        for (const auto& c : circles) {
            SDL_SetRenderDrawColor(ren, c.color.r, c.color.g, c.color.b, 255);
            drawFilledCircle(ren, (int)std::lround(c.x), (int)std::lround(c.y), c.r);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
