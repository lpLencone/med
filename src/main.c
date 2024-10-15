#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "cursor_renderer.h"
#include "editor.h"
#include "freetype_renderer.h"
#include "la.h"
#include "lib.h"
#include "program_object.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define MAX_SCALE     1
#define MIN_SCALE     0.225
#define PIXEL_SIZE    128

#define FONT_FREE_FILENAME "fonts/VictorMono-Regular.ttf"
// #define FONT_FREE_FILENAME "fonts/Qdbettercomicsans-jEEeG.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Mx (mixed outline+bitmap)/Mx437_Mindset.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel outline)/Px437_IBM_VGA_8x16.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel outline)/Px437_IBM_EGA_8x8.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel outline)/Px437_Compaq_Port3.ttf"
// #define FONT_FREE_FILENAME "fonts/ttf - Px (pixel
// outline)/PxPlus_Rainbow100_re_132.ttf"

void MessageCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        GLchar const *message, void const *userParam)
{
    (void) source, (void) id, (void) length, (void) userParam, (void) severity;
    if (type == GL_DEBUG_TYPE_ERROR) {
        fprintf(stderr, "OpenGL Error :: %s\n", message);
    }
}

static editor_t editor = { 0 };
static ft_renderer_t ftr = { 0 };
static cursor_renderer_t cr = { 0 };
static renderer_t r = { 0 };
static GLFWwindow *window = NULL;

static void terminate(void)
{
    editor_free(&editor);
    ftr_free(&ftr);
    cr_free(&cr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

v2f_t camera_pos = { 0 };
v2f_t resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
float cursor_time_moved = 0;
float g_scale = MAX_SCALE;
GLuint basic_program;

void render_scene(float dt)
{
    float const VEL = 3;
    dt *= VEL;

    float max_line_width = 0;
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
        /////////////////////////////////////////////////////////////////////////////////
        max_line_width = ftr_get_max_line_width(
                &ftr, editor.text_buffer.data + start, end - start);
        float g_scale_target =
                max(MIN_SCALE, min(MAX_SCALE, 0.6 * resolution.x / max_line_width));
        float g_scale_vel = g_scale_target - g_scale;
        g_scale += g_scale_vel * dt;
    }

    v2f_t cur_pos = { 0 }, cur_size = { 0 };
    {
        cur_pos = ftr_cursor_pos(&ftr, editor.text_buffer.data, editor.text_cursor);
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
        ///////////////////////////////////////////////////////////////////////////////////
        float camera_target_x;
        {
            float a = max(cur_pos.x, leftmost);
            camera_target_x = (cur_pos.x < a) ? a : min(cur_pos.x, rightmost);
        }
        v2f_t camera_target = v2f(camera_target_x, cur_pos.y + cur_size.y / 2.0);
        v2f_t camera_vel = v2f_sub(camera_target, camera_pos);
        camera_pos = v2f_add(camera_pos, v2f_mulf(camera_vel, dt));
    }

    float const time = glfwGetTime();
    {
        cr_set(&cr, CU_TIME, time);
        cr_set(&cr, CU_SCALE, g_scale);
        cr_set(&cr, CU_CAMERA, camera_pos);
        cr_set(&cr, CU_RESOLUTION, resolution);
        cr_set(&cr, CU_TIME_MOVED, cursor_time_moved);
        cr_draw(&cr, cur_pos, cur_size, v4fs(1.0));
        ///////////////////////////////////////////////////////////////////////////////////
        ftr_use(&ftr, FTP_RAINBOW);
        ftr_set(&ftr, FTU_TIME, time);
        ftr_set(&ftr, FTU_SCALE, g_scale);
        ftr_set(&ftr, FTU_CAMERA, camera_pos);
        ftr_set(&ftr, FTU_RESOLUTION, resolution);

        ftr_render_text(
                &ftr, editor.text_buffer.data, editor.text_buffer.length, v2fs(0),
                v4fs(1));
        ftr_draw(&ftr);
    }

    // Render minibuffer
    if (editor.mini) {
        ftr_set(&ftr, FTU_SCALE, (float) MIN_SCALE);
        ftr_set(&ftr, FTU_CAMERA, v2fs(0));
        v2f_t pos = ftr_render_text(
                &ftr, editor.miniprompt, strlen(editor.miniprompt),
                v2f_add(v2f_divf(v2f_neg(resolution), 2 * MIN_SCALE), v2fs(100)),
                v4fs(1));
        ftr_render_text(
                &ftr, editor.minibuffer.data, editor.minibuffer.length,
                v2f(pos.x + 100, pos.y), v4fs(1));
        ftr_draw(&ftr);
    }

    // Render selection
    if (editor.mark_set) {
        program_object_use(basic_program);
        program_object_uniform1f(basic_program, "u_scale", g_scale);
        program_object_uniform2f(basic_program, "u_camera", v2(camera_pos));
        program_object_uniform2f(basic_program, "u_resolution", v2(resolution));
        size_t mark_begin = editor.mark;
        size_t mark_end = editor.text_cursor;
        if (mark_begin > mark_end) {
            swap(mark_begin, mark_end);
        }

        while (mark_begin < mark_end) {
            size_t end_line = str_find_char(&editor.text_buffer, '\n', mark_begin);
            if (end_line > mark_end) {
                end_line = mark_end;
            }
            v2f_t start_pos = ftr_cursor_pos(&ftr, editor.text_buffer.data, mark_begin);
            v2f_t end_pos = ftr_cursor_pos(&ftr, editor.text_buffer.data, end_line);
            end_pos.x += ftr_char_width(&ftr, ' ') * (end_line != mark_end);
            assert(start_pos.y == end_pos.y);
            start_pos.y -= ftr.atlas_low;
            renderer_solid_rect(
                    &r, start_pos, v2f(end_pos.x - start_pos.x, ftr.atlas_h),
                    v4f(1, 1, 1, 0.3));
            renderer_draw(&r);
            mark_begin = end_line + 1;
        }
    }
}
static void initialize_glfw(GLFWwindow **window);
static void initialize_glew(void);
static void initialize_freetype(FT_Face *face);

int main(int argc, char const *argv[])
{
    if (argc > 1) {
        editor_load_file(&editor, argv[1]);
    }

    FT_Face face = { 0 };
    atexit(terminate);
    initialize_glfw(&window);
    initialize_glew();
    initialize_freetype(&face);

    renderer_init(&r);
    if (!program_object_link(
                &basic_program, (char const *[]) { "shaders/camera.vert" }, 1,
                (char const *[]) { "shaders/basic_color.frag" }, 1)) {
        return 1;
    }

    if (!cr_init(&cr) || !ftr_init(&ftr, face)) {
        return 1;
    }
    ftr_use(&ftr, FTP_RAINBOW);

    editor_new(&editor);

    size_t cur_last_pos = editor.text_cursor;
    float dt, now, last_frame = 0.0;
    while (!glfwWindowShouldClose(window)) {
        now = glfwGetTime();
        dt = now - last_frame;
        last_frame = now;

        if (editor.text_cursor != cur_last_pos) {
            cur_last_pos = editor.text_cursor;
            cursor_time_moved = now;
        }

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        render_scene(dt);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void) window;
    (void) scancode;
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    //////////////////////////////////////////////////////////////////////

    if (editor.fsnav) {
        switch (key) {
            case GLFW_KEY_P:
                editor_previous_line(&editor);
                break;
            case GLFW_KEY_N:
                editor_next_line(&editor);
                break;
            case GLFW_KEY_ENTER:
                editor_fsnav_find_file(&editor);
                break;
        }
        return;
    }
    //////////////////////////////////////////////////////////////////////

    if (mods & GLFW_MOD_CONTROL) {
        switch (key) {
            case GLFW_KEY_S:
                editor_save_buffer(&editor);
                break;
            case GLFW_KEY_SLASH:
                editor_isearch(&editor);
                break;

            case GLFW_KEY_P:
                editor_previous_line(&editor);
                break;
            case GLFW_KEY_N:
                editor_next_line(&editor);
                break;
            case GLFW_KEY_B:
                editor_backward_char(&editor);
                break;
            case GLFW_KEY_F:
                editor_forward_char(&editor);
                break;

            case GLFW_KEY_D:
                editor_fsnav(&editor);
                break;

            case GLFW_KEY_SPACE:
                editor_set_mark(&editor);
                break;
        }
    }

    switch (key) {
        case GLFW_KEY_ENTER:
            if (editor.mini) {
                editor_minibuffer_terminate(&editor);
            } else {
                editor_newline(&editor);
            }
            break;
        case GLFW_KEY_BACKSPACE:
            editor_delete_backward_char(&editor);
            break;
        case GLFW_KEY_DELETE:
            editor_delete_char(&editor);
            break;
    }
}

static void character_callback(GLFWwindow *window, unsigned int codepoint)
{
    (void) window;
    //////////////////////////////////////////////////////////////////////
    if (editor.fsnav) {
        return;
    }
    //////////////////////////////////////////////////////////////////////
    assert(32 <= codepoint && codepoint < 127);
    editor_self_insert(&editor, codepoint);
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    (void) window;
    resolution = v2f(width, height);
    glViewport(0, 0, width, height);
}

static void error_callback(int error, char const *description)
{
    (void) error;
    panic("GLFW ERROR: %s\n", description);
}

static void initialize_glfw(GLFWwindow **window)
{
    glfwSetErrorCallback(error_callback);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    *window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    glfwMakeContextCurrent(*window);

    glfwSetKeyCallback(*window, key_callback);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
    glfwSetCharCallback(*window, character_callback);
}

static void initialize_glew(void)
{
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
        debugf("Warning: GLEW_ARB_debug_output is not available.\n");
    }

    if (!GLEW_ARB_draw_instanced) {
        panic("Error: GLEW_ARB_draw_instanced not supported.");
    }

    if (!GLEW_ARB_instanced_arrays) {
        panic("Error: GLEW_ARB_instanced_arrays not supported.");
    }
}

static void initialize_freetype(FT_Face *face)
{
    FT_Library library = { 0 };
    FT_Error error = FT_Init_FreeType(&library);
    if (FT_Err_Ok != error) {
        panic("Could not initialize the FreeType library: %s", FT_Error_String(error));
    }

    error = FT_New_Face(library, FONT_FREE_FILENAME, 0, face);
    if (error == FT_Err_Cannot_Open_Resource) {
        panic("Could not open font filename: %s", FONT_FREE_FILENAME);
    } else if (error != FT_Err_Ok) {
        panic("Could not create face: %d - %s", error, FT_Error_String(error));
    }

    FT_UInt pixel_size = PIXEL_SIZE;
    error = FT_Set_Pixel_Sizes(*face, 0, pixel_size);
    if (FT_Err_Ok != error) {
        panic("Could not set pixel sizes: %s", FT_Error_String(error));
    }
}
