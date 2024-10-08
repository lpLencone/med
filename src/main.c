#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "cursor_renderer.h"
#include "editor.h"
#include "freetype_renderer.h"
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
        eprintf("%d::SDL ERROR: %s\n", line, SDL_GetError());
        exit(1);
    }
}
#define scc(code) scc_(code, __LINE__)

void *scp_(void *ptr, int line)
{
    if (ptr == NULL) {
        eprintf("%d::SDL ERROR: %s\n", line, SDL_GetError());
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

editor_t editor = { 0 };
v2f_t camera_pos = { 0 };
v2f_t resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
float const MAX_SCALE = 1.0;
float const MIN_SCALE = 0.25;
float g_scale = MAX_SCALE;

static ft_renderer_t ftr = { 0 };
static cursor_renderer_t cr = { 0 };

void render_scene(float dt)
{
    float max_line_width = 0.0;
    {
        size_t line_size = ftr.atlas_h * g_scale;
        size_t line_count = resolution.y / line_size;
        size_t line_start =
                max((int) editor_get_cursor_row(&editor) - (int) (line_count / 2), 0);
        size_t line_end =
                min(editor_get_cursor_row(&editor) + line_count / 2,
                    editor_get_line_count(&editor));
        size_t start = editor_nth_char_index(&editor, '\n', line_start);
        size_t end = editor_nth_char_index(&editor, '\n', line_end + 1);

        max_line_width =
                ftr_get_max_line_width(&ftr, editor.string.data + start, end - start);
        float g_scale_target =
                max(MIN_SCALE, min(MAX_SCALE, 0.6 * resolution.x / max_line_width));
        float g_scale_vel = g_scale_target - g_scale;
        g_scale += g_scale_vel * 2.0 * dt;
    }

    v2f_t cur_pos = { 0 }, cur_size = { 0 };
    {
        cur_pos = ftr_cursor_pos(&ftr, editor.string.data, editor.cursor);
        char c = editor_get_char(&editor);
        float cur_width = ftr_char_width(&ftr, (c != '\0' && c != '\n') ? c : ' ');
        cur_size = v2f(cur_width, ftr.atlas_h);
        cur_pos.y -= ftr.atlas_low;
    }

    {
        float const line_half_width = max_line_width / 2.0;
        float const RIGHT_OFFSET = 0.15;
        float const LEFT_OFFSET = 0.05;
        float const rightmost =
                max(line_half_width,
                    max_line_width - (0.5 - RIGHT_OFFSET) * resolution.x / g_scale);
        float const leftmost =
                min(line_half_width, (0.5 - LEFT_OFFSET) * resolution.x / g_scale);

        float camera_target_x;
        {
            float a = max(cur_pos.x, leftmost);
            camera_target_x = (cur_pos.x < a) ? a : min(cur_pos.x, rightmost);
        }
        v2f_t camera_target = v2f(camera_target_x, cur_pos.y + cur_size.y / 2.0);
        v2f_t camera_vel = v2f_sub(camera_target, camera_pos);
        camera_pos = v2f_add(camera_pos, v2f_mulf(camera_vel, 2.0 * dt));
    }

    float const time = SDL_GetTicks() / 1000.0;
    {
        cr_set(&cr, CU_TIME, time);
        cr_set(&cr, CU_SCALE, g_scale);
        cr_set(&cr, CU_CAMERA, camera_pos);
        cr_draw(&cr, cur_pos, cur_size, v4fs(1.0));
    }

    {
        ftr_set(&ftr, FTU_TIME, time);
        ftr_set(&ftr, FTU_SCALE, g_scale);
        ftr_set(&ftr, FTU_CAMERA, camera_pos);
        ftr_render_text(&ftr, editor.string.data, editor.string.length, v2fs(0), v4fs(1));
        ftr_draw(&ftr);
    }
}

int main(int argc, char *argv[])
{
    assert(argc == 2);
    char const *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
        editor_load_file(&editor, fp);
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
            eprintf("Warning: GLEW_ARB_debug_output is not available.\n");
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
        panic("Could not initialize the FreeType library: %s", FT_Error_String(error));
    }

    FT_Face face = { 0 };
    error = FT_New_Face(library, FONT_FREE_FILENAME, 0, &face);
    if (error == FT_Err_Cannot_Open_Resource) {
        panic("Could not open font filename: %s", FONT_FREE_FILENAME);
    } else if (error != FT_Err_Ok) {
        panic("Could not create face: %d - %s", error, FT_Error_String(error));
    }

    FT_UInt pixel_size = PIXEL_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (FT_Err_Ok != error) {
        panic("Could not set pixel sizes: %s", FT_Error_String(error));
    }

    ftr_init(&ftr, face);
    cr_init(&cr);
    size_t cur_last_pos = editor.cursor;

    bool quit = false;
    float dt, now, last_frame = 0.0;
    while (!quit) {
        now = SDL_GetTicks() / 1000.0;
        dt = now - last_frame;
        last_frame = now;

        if (editor.cursor != cur_last_pos) {
            cur_last_pos = editor.cursor;
            cr_set(&cr, CU_TIME_MOVED, now);
        }

        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_WINDOWEVENT: {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        int width, height;
                        SDL_GetWindowSize(window, &width, &height);
                        glViewport(0, 0, width, height);
                        resolution = v2f(width, height);
                        ftr_set(&ftr, FTU_RESOLUTION, resolution);
                        cr_set(&cr, CU_RESOLUTION, resolution);
                    }
                } break;

                case SDL_QUIT: {
                    quit = true;
                } break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: {
                            editor_delete_backward_char(&editor);
                        } break;

                        case SDLK_DELETE: {
                            editor_delete_char(&editor);
                        } break;

                        case SDLK_RETURN: {
                            editor_newline(&editor);
                        } break;

                        case SDLK_l: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_forward_char(&editor);
                            }
                        } break;

                        case SDLK_h: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_backward_char(&editor);
                            }
                        } break;

                        case SDLK_j: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_next_line(&editor);
                            }
                        } break;

                        case SDLK_k: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_previous_line(&editor);
                            }
                        } break;

                        case SDLK_3: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_beginning_of_line(&editor);
                            }
                        } break;

                        case SDLK_4: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                editor_move_end_of_line(&editor);
                            }
                        } break;

                        case SDLK_s: {
                            if (event.key.keysym.mod & KMOD_CTRL) {
                                FILE *fp = fopen(filename, "w");
                                if (fp == NULL) {
                                    panic("Could not open file %s: %s", filename,
                                          strerror(errno));
                                }
                                editor_save_to_file(&editor, fp);
                                fclose(fp);
                            }
                        } break;
                    }
                } break;

                case SDL_TEXTINPUT: {
                    editor_insert_text(&editor, event.text.text, strlen(event.text.text));
                } break;
            }
        }

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        render_scene(dt);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    return 0;
}
