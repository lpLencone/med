#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "buffer.h"
#include "la.h"
#include "lib.h"
#include "shader.h"
#include "tile_glyph.h"

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

void MessageCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        GLchar const *message, void const *userParam)
{
    (void) source, (void) id, (void) length, (void) userParam, (void) severity;
    fprintf(stderr, "%s | message = %s\n",
            type == GL_DEBUG_TYPE_ERROR ? "GL ERROR" : "GL INFO ", message);
}

typedef struct {
    v2f_t pos;
    v2f_t size;
    int code;
    v2f_t fg;
    v2f_t bg;
} ftglyph_t;

buffer_t buffer = { 0 };
v2f_t cam_pos = { 0 }, cam_vel = { 0 };

void render_cursor(tile_glyph_buffer_t *tgb)
{
    v2i_t tile = {
        .x = buffer_get_cursor_col(&buffer),
        .y = buffer_get_cursor_row(&buffer),
    };
    char c;
    if (buffer.cursor < buffer.string.length) {
        c = buffer.string.data[buffer.cursor];
        c = (c != '\n') ? c : ' '; 
    } else {
        c = ' ';
    }
    tgb_render_text(
            tgb, &c, 1, v2i(tile.x, -tile.y), v4fs(0.0),
            v4fs(1.0));
}

static tile_glyph_buffer_t tgb = {0};

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
            SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));

    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        int major, minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("GL Version %d.%d\n", major, minor);

        scp(SDL_GL_CreateContext(window));

        GLenum code;
        if ((code = glewInit()) != GLEW_OK) {
            panic("Could not initialize glew: %s", glewGetErrorString(code));
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (GLEW_ARB_debug_output) {
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(MessageCallback, 0);
        } else {
            eprintln("Warning: GLEW_ARB_debug_output is not available.");
        }

        if (!GLEW_ARB_draw_instanced) {
            panic("Error: GLEW_ARB_draw_instanced not supported.");
        }

        if (!GLEW_ARB_instanced_arrays) {
            panic("Error: GLEW_ARB_instanced_arrays not supported.");
        }
    }

    tgb_init(&tgb, FONT_FILENAME, "shaders/tile_glyph.vert", "shaders/tile_glyph.frag");

    bool quit = false;
    float dt, now, last_frame = 0.0;
    while (!quit) {
        now = SDL_GetTicks() / 1000.0;
        dt = now - last_frame;
        last_frame = now;

        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_WINDOWEVENT: {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        int width, height;
                        SDL_GetWindowSize(window, &width, &height);
                        glViewport(0, 0, width, height);
                        glUniform2f(tgb.u_resolution, (float) width, (float) height);
                    }
                } break;

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

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        v2f_t cur_pos = {
            .x = buffer_get_cursor_col(&buffer),
            .y = buffer_get_cursor_row(&buffer),
        };
        cur_pos =
                v2f_mul(cur_pos,
                        v2f(FONT_SCALE * FONT_CHAR_WIDTH, FONT_SCALE * FONT_CHAR_HEIGHT));
        cam_vel = v2f_sub(cur_pos, cam_pos);
        cam_pos = v2f_add(cam_pos, v2f_mulf(cam_vel, 2.0 * dt));

        glUseProgram(tgb.shader);
        glUniform2f(tgb.u_camera, cam_pos.x, -cam_pos.y);
        glUniform1f(tgb.u_time, SDL_GetTicks() / 1000.0);
        glUniform1f(tgb.u_scale, FONT_SCALE);

        tgb_render_text(&tgb,
                buffer.string.data, buffer.string.length, v2i(0, 0), v4fs(1.0),
                v4fs(0.0));
        render_cursor(&tgb);
        tgb_flush(&tgb);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    return 0;
}
