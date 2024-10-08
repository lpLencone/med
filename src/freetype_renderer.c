#include "freetype_renderer.h"

#include <assert.h>
#include <stddef.h>

#include "lib.h"
#include "program_object.h"
#include "stb_image.h"

static void ftr_init_texture_atlas(ft_renderer_t *ftr, FT_Face face);

void ftr_init(ft_renderer_t *ftr, FT_Face face)
{
    char const *vert_filenames[] = { "shaders/basic.vert", "shaders/project.glsl" };
    char const *rainbow_filename = "shaders/rainbow.frag";
    char const *image_filename = "shaders/basic_image.frag";

    renderer_init(&ftr->r);

    program_object_link(
            &ftr->program[FTP_RAINBOW], vert_filenames, 2, &rainbow_filename, 1);
    program_object_link(&ftr->program[FTP_COLOR], vert_filenames, 2, &image_filename, 1);

    ftr_init_texture_atlas(ftr, face);
    ftr->current_program = -1;
}

void ftr_draw(ft_renderer_t *ftr)
{
    ftr_use(ftr, ftr->current_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ftr->atlas);
    renderer_draw(&ftr->r);
}

void ftr_render_text(
        ft_renderer_t *ftr, char const *text, size_t text_size, v2f_t pos, v4f_t color)
{
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] == '\n') {
            pos.y -= ftr->atlas_h;
            pos.x = 0;
            continue;
        }

        ft_glyph_metrics_t metrics = ftr->metrics[(int) text[i]];
        float x = metrics.bl + pos.x;
        float y = metrics.bt + pos.y;
        float w = metrics.bw;
        float h = metrics.bh;

        pos.x += metrics.ax;
        pos.y += metrics.ay;

        renderer_image_rect(
                &ftr->r, v2f(x, y), v2f(w, -h), v2f(metrics.tx, 0.0),
                v2f(metrics.bw / ftr->atlas_w, metrics.bh / ftr->atlas_h), color);
    }
}

v2f_t ftr_cursor_pos(ft_renderer_t *ftr, char const *text, size_t text_size)
{
    v2f_t pos = v2fs(0.0);
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] == '\n') {
            pos.y -= ftr->atlas_h;
            pos.x = 0;
            continue;
        }
        ft_glyph_metrics_t metrics = ftr->metrics[(int) text[i]];
        pos.x += metrics.ax;
        pos.y += metrics.ay;
    }
    return pos;
}

float ftr_get_max_line_width(ft_renderer_t *ftr, char const *text, size_t text_size)
{
    float width = 0;
    float max_width = 0;
    for (size_t cursor = 0; cursor < text_size; cursor++) {
        if (text[cursor] == '\n') {
            if (width > max_width) {
                max_width = width;
            }
            width = 0;
            continue;
        }
        width += ftr_char_width(ftr, text[cursor]);
    }
    if (width > max_width) {
        max_width = width;
    }
    return max_width;
}

float ftr_char_width(ft_renderer_t *ftr, char c)
{
    ft_glyph_metrics_t metrics = ftr->metrics[(int) c];
    return metrics.ax;
}

void ftr_use(ft_renderer_t *ftr, enum ft_program p)
{
    if (p < 0 || FTP_COUNT <= p) {
        panic("Tried to set invalid program: %d. Maybe you forgot to set a shader before drawing or set a uniform?",
              p);
    }
    ftr->current_program = p;
    program_object_use(ftr->program[p]);
}

static char const *uniform_name(enum ft_uniform ftu);

void ftr_set_float(ft_renderer_t *ftr, enum ft_uniform u, float f)
{
    ftr_use(ftr, ftr->current_program);
    program_object_uniform1f(ftr->program[ftr->current_program], uniform_name(u), f);
}

void ftr_set_v2f(ft_renderer_t *ftr, enum ft_uniform u, v2f_t v)
{
    ftr_use(ftr, ftr->current_program);
    program_object_uniform2f(ftr->program[ftr->current_program], uniform_name(u), v2(v));
}

static void ftr_init_texture_atlas(ft_renderer_t *ftr, FT_Face face)
{
    ftr->atlas_w = 0;
    ftr->atlas_h = 0;
    ftr->atlas_low = 0;

    FT_Error error;
    for (int i = 32; i < 128; i++) {
        if ((error = FT_Load_Char(face, i, FT_LOAD_RENDER)) != FT_Err_Ok) {
            eprintf("Error loading char %c: %s\n", i, FT_Error_String(error));
            continue;
        }
        FT_GlyphSlot gs = face->glyph;
        ftr->atlas_w += gs->bitmap.width;
        if (ftr->atlas_h < gs->bitmap.rows) {
            ftr->atlas_h = gs->bitmap.rows;
        }
        int low = (int) gs->bitmap.rows - gs->bitmap_top;
        if ((int) ftr->atlas_low < low) {
            ftr->atlas_low = low;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &ftr->atlas);
    glBindTexture(GL_TEXTURE_2D, ftr->atlas);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED, (GLsizei) ftr->atlas_w, (GLsizei) ftr->atlas_h, 0,
            GL_RED, GL_UNSIGNED_BYTE,
            NULL // A null pointer is used to tell OpenGL that  the contents of the
                 // texture will be provided later.
    );

    int x = 0;
    for (int i = 32; i < 128; i++) {
        if ((error = FT_Load_Char(face, i, FT_LOAD_RENDER)) != FT_Err_Ok) {
            panic("Error loading char %c: %s\n", i, FT_Error_String(error));
        }
        FT_GlyphSlot gs = face->glyph;
        if ((error = FT_Render_Glyph(gs, FT_RENDER_MODE_NORMAL)) != FT_Err_Ok) {
            panic("Error rendering glyph: %s\n", FT_Error_String(error));
        }

        glTexSubImage2D(
                GL_TEXTURE_2D, 0, x, 0, face->glyph->bitmap.width,
                face->glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer);

        ft_glyph_metrics_t *gm = ftr->metrics + i;
        gm->ax = gs->advance.x >> 6;
        gm->ay = gs->advance.y >> 6;
        gm->bw = gs->bitmap.width;
        gm->bh = gs->bitmap.rows;
        gm->bl = gs->bitmap_left;
        gm->bt = gs->bitmap_top;
        gm->tx = (float) x / ftr->atlas_w;

        x += face->glyph->bitmap.width;
    }
}

static char const *uniform_name(enum ft_uniform ftu)
{
    switch (ftu) {
        case FTU_TIME:
            return "u_time";
            break;
        case FTU_RESOLUTION:
            return "u_resolution";
            break;
        case FTU_CAMERA:
            return "u_camera";
            break;
        case FTU_SCALE:
            return "u_scale";
            break;
        default:
            panic("Unreachable\n");
    }
}

static_assert(FTU_COUNT == 4, "The amount of freetype renderer uniforms has changed.");
