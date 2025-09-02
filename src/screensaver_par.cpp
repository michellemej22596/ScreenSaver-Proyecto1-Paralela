#include <SDL2/SDL.h>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <omp.h>

struct Particle {
    float x, y;
    float vx, vy;
    float ax, ay;
    int r;
    Uint8 cr, cg, cb;
    Uint8 alpha;
};

struct Config {
    int N = 200;
    int width = 800;
    int height = 600;
    int threads = 4;
    int fps = 60;
};

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

SDL_Texture* createCircleTexture(SDL_Renderer* ren, int r) {
    int size = r * 2;
    Uint32 pixel_format = SDL_PIXELFORMAT_RGBA32;
    void* pixels = malloc(size * size * 4);
    if (!pixels) return nullptr;
    Uint8* px = (Uint8*)pixels;
    for(int y=0; y<size; y++){
        for(int x=0; x<size; x++){
            int dx = x - r;
            int dy = y - r;
            float dist = std::sqrt(dx*dx + dy*dy);
            Uint8 a = 0;
            if(dist <= r){
                float t = (1.0f - dist/(float)r);
                a = (Uint8)(255*t*t);
            }
            int idx = (y*size + x)*4;
            px[idx+0] = 255; px[idx+1] = 255; px[idx+2] = 255; px[idx+3] = a;
        }
    }

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, pixel_format);
    if(!surf){ free(pixels); return nullptr; }
    memcpy(surf->pixels, pixels, size*size*4);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surf);
    free(pixels);
    return tex;
}

int main(int argc, char** argv){
    Config cfg = parseArgs(argc, argv);
    omp_set_num_threads(cfg.threads);

    if(SDL_Init(SDL_INIT_VIDEO) != 0){ std::cerr << "SDL_Init error\n"; return 1; }

    SDL_Window* win = SDL_CreateWindow("Screensaver Paralelo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg.width, cfg.height, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> ux(0.0f,(float)cfg.width);
    std::uniform_real_distribution<float> uy(0.0f,(float)cfg.height);
    std::uniform_real_distribution<float> uv(-120.0f,120.0f);
    std::uniform_int_distribution<int> ur(3,20);
    std::uniform_int_distribution<int> uc(0,255);

    std::vector<Particle> particles(cfg.N);
    for(auto &p : particles){
        p.r = ur(rng);
        p.x = ux(rng); p.y = uy(rng);
        p.vx = uv(rng)*0.01f; p.vy = uv(rng)*0.01f;
        p.ax = p.ay = 0.0f;
        p.cr = uc(rng); p.cg = uc(rng); p.cb = uc(rng);
        p.alpha = 160 + uc(rng)%96;
    }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    std::unordered_map<int,SDL_Texture*> tex_by_r;
    for(int r=3; r<=20; r++) tex_by_r[r] = createCircleTexture(ren,r);

    bool running = true;
    SDL_Event ev;
    auto last = std::chrono::steady_clock::now();
    double accumulator=0.0;
    const double dt_fixed=1.0/60.0;

    double cur_fps = 0.0; // inicialización para evitar error

    while(running){
        while(SDL_PollEvent(&ev)){
            if(ev.type==SDL_QUIT) running=false;
            else if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_ESCAPE) running=false;
        }

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now-last;
        last = now;
        accumulator += elapsed.count();

        while(accumulator>=dt_fixed){
            #pragma omp parallel for
            for(size_t i=0; i<particles.size(); i++){
                auto &p = particles[i];
                float cx = cfg.width*0.5f;
                float cy = cfg.height*0.5f;
                float dx = cx-p.x, dy = cy-p.y;
                float dist = std::sqrt(dx*dx + dy*dy)+1e-5f;
                float pull = 20.0f/dist;
                p.ax = dx/dist*pull*0.02f;
                p.ay = dy/dist*pull*0.02f;

                p.vx += p.ax*(float)dt_fixed; p.vy += p.ay*(float)dt_fixed;
                p.vx *= 0.9995f; p.vy *= 0.9995f;
                p.x += p.vx*dt_fixed*60.0f;
                p.y += p.vy*dt_fixed*60.0f;

                if(p.x < p.r){ p.x = p.r; p.vx = -p.vx*0.9f; }
                else if(p.x > cfg.width-p.r){ p.x = cfg.width-p.r; p.vx = -p.vx*0.9f; }
                if(p.y < p.r){ p.y = p.r; p.vy = -p.vy*0.9f; }
                else if(p.y > cfg.height-p.r){ p.y = cfg.height-p.r; p.vy = -p.vy*0.9f; }
            }
            accumulator -= dt_fixed;
        }

        // Fondo dinámico sutil
        float tbg = SDL_GetTicks() / 2000.0f;
        Uint8 rbg = Uint8(60 + 40 * std::sin(tbg));
        Uint8 gbg = Uint8(30 + 30 * std::sin(tbg + 2.0f));
        Uint8 bbg = Uint8(80 + 50 * std::cos(tbg));
        SDL_SetRenderDrawColor(ren, rbg, gbg, bbg, 40);
        SDL_Rect full = {0, 0, cfg.width, cfg.height};
        SDL_RenderFillRect(ren, &full);

        // Trail overlay
        SDL_SetRenderDrawColor(ren,0,0,0,40);
        SDL_RenderFillRect(ren,&full);

        // Render partículas
        for(auto &p:particles){
            SDL_Texture* tex = tex_by_r[p.r];
            SDL_SetTextureColorMod(tex, p.cr, p.cg, p.cb);
            SDL_SetTextureAlphaMod(tex, p.alpha);
            SDL_Rect dst = {int(p.x-p.r), int(p.y-p.r), p.r*2, p.r*2};
            SDL_RenderCopy(ren, tex, nullptr, &dst);
        }

        SDL_RenderPresent(ren);
    }

    for(auto &t:tex_by_r) SDL_DestroyTexture(t.second);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
