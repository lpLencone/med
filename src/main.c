#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "la.h"
#include "lib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FONT_FILENAME    "charmap-oldschool_white.png"
#define FONT_WIDTH       128
#define FONT_HEIGHT      64
#define FONT_ROWS        7
#define FONT_COLS        18
#define FONT_CHAR_WIDTH  (int) (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (int) (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE       5.0

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
        eprintln("STBI ERROR: Could not load \"%s\": %s", filename, stbi_failure_reason());
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

void render_char(SDL_Renderer *renderer, font_t const *font, char c, v2f_t pos, float scale)
{
    assert(c >= ASCII_DISPLAY_LOW && c <= ASCII_DISPLAY_HIGH);

    SDL_Rect const destine = { .x = (int) floorf(pos.x),
                               .y = (int) floorf(pos.y),
                               .w = (int) floorf(scale * FONT_CHAR_WIDTH),
                               .h = (int) floorf(scale * FONT_CHAR_HEIGHT) };

    size_t const index = c - ASCII_DISPLAY_LOW;
    scc(SDL_RenderCopy(renderer, font->sprite, &font->glyph_table[index], &destine));
}

void render_text(
        SDL_Renderer *renderer, font_t *font, char const *text, size_t text_size, v2f_t pos,
        Uint32 color, float scale)
{
    set_texture_color(font->sprite, color);
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] < ASCII_DISPLAY_LOW || text[i] > ASCII_DISPLAY_HIGH) {
            continue;
        }
        render_char(renderer, font, text[i], pos, scale);
        pos.x += FONT_CHAR_WIDTH * scale;
    }
}

#define render_cstr(renderer, font, cstr, pos, color, scale) \
    render_text(renderer, font, cstr, strlen(cstr), pos, color, scale)

#define BUFFER_CAPACITY 1024
char buffer[BUFFER_CAPACITY];
size_t buffer_size = 0;
size_t buffer_cursor = 0;

void buffer_insert_text(char const *text, size_t text_size)
{
    if (text_size > BUFFER_CAPACITY - buffer_size) {
        text_size = BUFFER_CAPACITY - buffer_size;
    }
    memmove(buffer + buffer_cursor + text_size, buffer + buffer_cursor,
            buffer_size - buffer_cursor);
    memcpy(buffer + buffer_cursor, text, text_size);
    buffer_size += text_size;
}

void buffer_delete_backward_char(void)
{
    assert(buffer_cursor <= buffer_size);
    if (buffer_cursor > 0) {
        memmove(buffer + buffer_cursor - 1, buffer + buffer_cursor, buffer_size - buffer_cursor);
        buffer_cursor--;
        buffer_size--;
    }
}

void buffer_delete_char(void)
{
    if (buffer_cursor < buffer_size) {
        memmove(buffer + buffer_cursor, buffer + buffer_cursor + 1, buffer_size - buffer_cursor - 1);
        buffer_size--;
    }
}

void render_cursor(SDL_Renderer *renderer, font_t const *font, float scale)
{
    v2f_t pos = v2f(buffer_cursor * scale * FONT_CHAR_WIDTH, 0.0);
    SDL_Rect rect = {
        .x = pos.x,
        .y = pos.y,
        .w = FONT_CHAR_WIDTH * scale,
        .h = FONT_CHAR_HEIGHT * scale,
    };
    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFF'FFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    set_texture_color(font->sprite, 0xFF00'0000);
    if (buffer_cursor < buffer_size) {
        render_char(renderer, font, buffer[buffer_cursor], pos, scale);
    }
}

int main(void)
{
    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = scp(SDL_CreateWindow(
            "med", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer *renderer = scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    font_t font = font_from_file(renderer, FONT_FILENAME);

    bool quit = false;
    while (!quit) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                } break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: {
                            buffer_delete_backward_char();
                        } break;

                        case SDLK_DELETE: {
                            buffer_delete_char();
                        } break;

                        case SDLK_l: {
                            if (buffer_cursor < buffer_size) {
                                buffer_cursor++;
                            }
                        } break;

                        case SDLK_h: {
                            if (buffer_cursor > 0) {
                                buffer_cursor--;
                            }
                        } break;
                    }
                } break;

                case SDL_TEXTINPUT: {
                    buffer_insert_text(event.text.text, strlen(event.text.text));
                } break;
            }
        }

        if (buffer_cursor > buffer_size) {
            buffer_cursor = buffer_size;
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        render_text(renderer, &font, buffer, buffer_size, v2fs(0.0), 0xFFFF'FFFF, FONT_SCALE);
        render_cursor(renderer, &font, FONT_SCALE);

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
}
