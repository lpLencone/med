#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "buffer.h"
#include "la.h"
#include "lib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define FONT_FILENAME    "charmap-oldschool_white.png"
#define FONT_WIDTH       128
#define FONT_HEIGHT      64
#define FONT_ROWS        7
#define FONT_COLS        18
#define FONT_CHAR_WIDTH  (int) (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (int) (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE       4.0

#define UNHEX(color)                                                                 \
    (color) >> (0 * 8) & 0xff, (color) >> (0 * 1) & 0xff, (color) >> (0 * 2) & 0xff, \
            (color) >> (0 * 3) & 0xff

void scc_(int code, int line)
{
    if (code < 0) {
        eprintln("%d::SDL ERROR: %s", line, SDL_GetError());
        exit(1);
    }
}
#define scc(code) scc_(code, __LINE__)

void *scp_(void *ptr, int line)
{
    if (ptr == NULL) {
        eprintln("%d::SDL ERROR: %s", line, SDL_GetError());
        exit(1);
    }
    return ptr;
}
#define scp(ptr) scp_(ptr, __LINE__)

SDL_Surface *surface_from_file(char const *filename)
{
    int width, height, n;
    unsigned char *pixels = stbi_load(filename, &width, &height, &n, STBI_rgb_alpha);
    if (pixels == NULL) {
        eprintln(
                "STBI ERROR: Could not load \"%s\": %s", filename, stbi_failure_reason());
        exit(1);
    }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 const rmask = 0xff00'0000;
    Uint32 const gmask = 0x00ff'0000;
    Uint32 const bmask = 0x0000'ff00;
    Uint32 const amask = 0x0000'00ff;
#else // little endian, like x86
    Uint32 const rmask = 0x0000'00ff;
    Uint32 const gmask = 0x0000'ff00;
    Uint32 const bmask = 0x00ff'0000;
    Uint32 const amask = 0xff00'0000;
#endif

    int const depth = 32;
    int const pitch = 4 * width;

    return scp(SDL_CreateRGBSurfaceFrom(
            pixels, width, height, depth, pitch, rmask, gmask, bmask, amask));
}

#define ASCII_DISPLAY_LOW  32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
    SDL_Texture *sprite;
    SDL_Rect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} font_t;

font_t font_from_file(SDL_Renderer *renderer, char const *filename)
{
    font_t font = { 0 };
    SDL_Surface *font_surface = surface_from_file(filename);
    scc(SDL_SetColorKey(font_surface, SDL_TRUE, 0xFF00'0000));
    font.sprite = scp(SDL_CreateTextureFromSurface(renderer, font_surface));
    SDL_FreeSurface(font_surface);

    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ascii++) {
        size_t const index = ascii - ASCII_DISPLAY_LOW;
        size_t const row = index / FONT_COLS;
        size_t const col = index % FONT_COLS;
        font.glyph_table[index] = (SDL_Rect) {
            .x = col * FONT_CHAR_WIDTH,
            .y = row * FONT_CHAR_HEIGHT,
            .w = FONT_CHAR_WIDTH,
            .h = FONT_CHAR_HEIGHT,
        };
    }

    return font;
}

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(
            texture, (color >> (8 * 0)) & 0xFF, (color >> (8 * 1)) & 0xFF,
            (color >> (8 * 2)) & 0xFF));
    scc(SDL_SetTextureAlphaMod(texture, (color >> (8 * 3)) & 0xFF));
}

void render_char(
        SDL_Renderer *renderer, font_t const *font, char c, v2f_t cam_pos, float scale)
{
    assert(c >= ASCII_DISPLAY_LOW && c <= ASCII_DISPLAY_HIGH);
    SDL_Rect const dst = { .x = (int) floorf(cam_pos.x),
                           .y = (int) floorf(cam_pos.y),
                           .w = (int) floorf(scale * FONT_CHAR_WIDTH),
                           .h = (int) floorf(scale * FONT_CHAR_HEIGHT) };

    size_t const index = c - ASCII_DISPLAY_LOW;
    scc(SDL_RenderCopy(renderer, font->sprite, &font->glyph_table[index], &dst));
}

void render_text(
        SDL_Renderer *renderer, font_t *font, char const *text, size_t text_size,
        v2f_t cam_pos, Uint32 color, float scale)
{
    v2f_t render_pos = cam_pos;
    set_texture_color(font->sprite, color);
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] == '\n') {
            render_pos.y += FONT_CHAR_HEIGHT * scale;
            render_pos.x = cam_pos.x;
            continue;
        }
        render_char(renderer, font, text[i], render_pos, scale);
        render_pos.x += FONT_CHAR_WIDTH * scale;
    }
}

buffer_t buffer = { 0 };
v2f_t cam_pos = { 0 }, cam_vel = { 0 };

void render_cursor(SDL_Renderer *renderer, font_t const *font, v2f_t cam_pos, float scale)
{
    SDL_Rect rect = {
        .x = cam_pos.x,
        .y = cam_pos.y,
        .w = FONT_CHAR_WIDTH * scale,
        .h = FONT_CHAR_HEIGHT * scale,
    };
    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFF'FFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    set_texture_color(font->sprite, 0xFF00'0000);
    if (buffer.cursor < buffer.string.length) {
        if (buffer.string.data[buffer.cursor] != '\n' &&
            buffer.string.data[buffer.cursor] != '\0') {
            render_char(
                    renderer, font, buffer.string.data[buffer.cursor], cam_pos, scale);
        }
    }
}

v2f_t camera_project_point(SDL_Window *window, v2f_t camera_pos, v2f_t point)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    return v2f_add(v2f_sub(point, camera_pos), v2f_mulf(v2f(w, h), 0.5));
}

int main(int argc, char *argv[])
{
    assert(argc == 2);
    char const *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
        buffer_load_file(&buffer, fp);
        fclose(fp);
    }

    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = scp(SDL_CreateWindow(
            "med", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
            SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE));
    SDL_Renderer *renderer =
            scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    font_t font = font_from_file(renderer, FONT_FILENAME);

    bool quit = false;
    float dt, now, last_frame = 0.0;
    while (!quit) {
        now = (float) SDL_GetTicks() / 1000.0;
        dt = now - last_frame;
        last_frame = now;

        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                } break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: {
                            buffer_delete_backward_char(&buffer);
                        } break;

                        case SDLK_DELETE: {
                            buffer_delete_char(&buffer);
                        } break;

                        case SDLK_RETURN: {
                            buffer_newline(&buffer);
                        } break;

                        case SDLK_l: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                buffer_forward_char(&buffer);
                            }
                        } break;

                        case SDLK_h: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                buffer_backward_char(&buffer);
                            }
                        } break;

                        case SDLK_j: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                buffer_next_line(&buffer);
                            }
                        } break;

                        case SDLK_k: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                buffer_previous_line(&buffer);
                            }
                        } break;

                        case SDLK_s: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                FILE *fp = fopen(filename, "w");
                                if (fp == NULL) {
                                    eprintln(
                                            "Could not open file %s: %s\n", filename,
                                            strerror(errno));
                                    exit(1);
                                }
                                buffer_save_to_file(&buffer, fp);
                                fclose(fp);
                            }
                        }
                    }
                } break;

                case SDL_TEXTINPUT: {
                    buffer_insert_text(&buffer, event.text.text, strlen(event.text.text));
                } break;
            }
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        v2f_t cur_pos = {
            .x = buffer_get_cursor_col(&buffer),
            .y = buffer_get_cursor_row(&buffer),
        };
        cur_pos =
                v2f_mul(cur_pos,
                        v2f(FONT_SCALE * FONT_CHAR_WIDTH, FONT_SCALE * FONT_CHAR_HEIGHT));

        cam_vel = v2f_sub(cur_pos, cam_pos);
        cam_pos = v2f_add(cam_pos, v2f_mulf(cam_vel, 2.0 * dt));
        printf("cur_pos: %.2f %.2f\n", v2(cur_pos));
        printf("cam_pos: %.2f %.2f\n", v2(cam_pos));

        render_text(
                renderer, &font, buffer.string.data, buffer.string.length, camera_project_point(window, cam_pos, v2fs(0.0)),
                0xFFFF'FFFF, FONT_SCALE);
        render_cursor(renderer, &font, camera_project_point(window, cam_pos, cur_pos), FONT_SCALE);

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
}
