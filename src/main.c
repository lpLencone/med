#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "buffer.h"
#include "cursor_renderer.h"
#include "free_glyph.h"
#include "la.h"
#include "lib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define PIXEL_SIZE 128

// #define FONT_FREE_FILENAME "fonts/VictorMono-Regular.ttf"
// #define FONT_FREE_FILENAME "fonts/Qdbettercomicsans-jEEeG.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Mx (mixed outline+bitmap)/Mx437_Mindset.ttf"
#define FONT_FREE_FILENAME "fonts/ttf - Px (pixel outline)/Px437_IBM_VGA_8x16.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel outline)/Px437_IBM_EGA_8x8.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel outline)/Px437_Compaq_Port3.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel
// outline)/PxPlus_Rainbow100_re_132.ttf"

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

buffer_t buffer = { 0 };
v2f_t camera_pos = { 0 };
v2f_t resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
float const MAX_SCALE = 1.0;
float const MIN_SCALE = 0.2;
float g_scale = MAX_SCALE;

static free_glyph_buffer_t fgb = { 0 };
static cursor_renderer_t cr = { 0 };

void render_fgb(float dt)
{
    size_t line_size = fgb.atlas_h * g_scale;
    int num_lines_displayed = resolution.y / line_size;
    size_t line_start =
            max((int) buffer_get_cursor_row(&buffer) - (num_lines_displayed / 2), 0);
    size_t widest_line_width = fgb_get_widest_line_width(
            &fgb, buffer.string.data, buffer.string.length, line_start,
            num_lines_displayed);
    float g_scale_target =
            max(MIN_SCALE, min(MAX_SCALE, 0.5 * resolution.x / widest_line_width));
    float g_scale_vel = g_scale_target - g_scale;
    g_scale += g_scale_vel * 2.0 * dt;

    v2f_t cur_pos = fgb_cursor_pos(&fgb, buffer.string.data, buffer.cursor);
    char c = buffer_get_char(&buffer);
    size_t cur_width = fgb_char_width(&fgb, (c != '\0' && c != '\n') ? c : ' ');
    v2f_t cur_size = v2f(cur_width, fgb.atlas_h);
    cur_pos.y -= fgb.atlas_low;

    v2f_t camera_target = v2f(widest_line_width / 2.0, cur_pos.y + cur_size.y / 2.0);
    v2f_t camera_vel = v2f_sub(camera_target, camera_pos);
    camera_pos = v2f_add(camera_pos, v2f_mulf(camera_vel, 2.0 * dt));

    glUseProgram(cr.shader);
    glUniform2f(cr.u[CRU_POS], v2(cur_pos));
    glUniform2f(cr.u[CRU_SIZE], v2(cur_size));
    glUniform1f(cr.u[CRU_TIME], SDL_GetTicks() / 1000.0);
    glUniform2f(cr.u[CRU_CAMERA], v2(camera_pos));
    glUniform1f(cr.u[CRU_SCALE], g_scale);

    cr_render();

    glUseProgram(fgb.shader);
    glUniform2f(fgb.u[FTU_CAMERA], v2(camera_pos));
    glUniform1f(fgb.u[FTU_TIME], SDL_GetTicks() / 1000.0);
    glUniform1f(fgb.u[FTU_SCALE], g_scale);

    fgb_render_text(
            &fgb, buffer.string.data, buffer.string.length, v2fs(0.0), v4fs(1.0),
            v4fs(0.0));
    fgb_flush(&fgb);
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

    FT_Library library = { 0 };
    FT_Error error = FT_Init_FreeType(&library);
    if (FT_Err_Ok != error) {
        panic("Could not initialize the FreeType library: %s\n", FT_Error_String(error));
    }

    FT_Face face = { 0 };
    error = FT_New_Face(library, FONT_FREE_FILENAME, 0, &face);
    if (error == FT_Err_Cannot_Open_Resource) {
        panic("Could not open font filename: " FONT_FREE_FILENAME "\n");
    } else if (error != FT_Err_Ok) {
        panic("Could not create face: %d - %s\n", error, FT_Error_String(error));
    }

    FT_UInt pixel_size = PIXEL_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (FT_Err_Ok != error) {
        panic("Could not set pixel sizes: %s\n", FT_Error_String(error));
    }

    fgb_init(&fgb, face, "shaders/free_glyph.vert", "shaders/free_glyph.frag");
    cr = cr_init("shaders/cursor.vert", "shaders/cursor.frag");
    size_t cur_last_pos = buffer.cursor;

    bool quit = false;
    float dt, now, last_frame = 0.0;
    while (!quit) {
        now = SDL_GetTicks() / 1000.0;
        dt = now - last_frame;
        last_frame = now;

        if (buffer.cursor != cur_last_pos) {
            cur_last_pos = buffer.cursor;
            glUseProgram(cr.shader);
            glUniform1f(cr.u[CRU_LAST_MOVED], now);
        }

        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_WINDOWEVENT: {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        int width, height;
                        SDL_GetWindowSize(window, &width, &height);
                        glViewport(0, 0, width, height);
                        glUseProgram(fgb.shader);
                        glUniform2f(fgb.u[FTU_RESOLUTION], (float) width, (float) height);
                        glUseProgram(cr.shader);
                        glUniform2f(cr.u[CRU_RESOLUTION], (float) width, (float) height);
                        resolution = v2f(width, height);
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

                        case SDLK_3: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                buffer_move_beginning_of_line(&buffer);
                            }
                        } break;

                        case SDLK_4: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                buffer_move_end_of_line(&buffer);
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
                        } break;
                    }
                } break;

                case SDL_TEXTINPUT: {
                    buffer_insert_text(&buffer, event.text.text, strlen(event.text.text));
                } break;
            }
        }

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        render_fgb(dt);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    return 0;
}
