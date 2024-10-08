#include "freetype_renderer.h"

#include <assert.h>
#include <stddef.h>

#include "lib.h"
#include "stb_image.h"

static void ftr_init_texture_atlas(ft_renderer_t *ftr, FT_Face face);
static void ftr_init_buffers(ft_renderer_t *ftr);

void ftr_init(ft_renderer_t *ftr, FT_Face face)
{
    ftr->count = 0;

    char const *vert_filenames[] = { "shaders/free_glyph.vert", "shaders/project.glsl" };
    char const *frag_filename = "shaders/free_glyph.frag";

    if (!glslink_program(
                &ftr->shader, slice_from(vert_filenames, 2),
                slice_from(&frag_filename, 1))) {
        panic("Could not load freetype renderer shaders.");
    }
    glUseProgram(ftr->shader);
    ftr_init_texture_atlas(ftr, face);
    ftr_init_buffers(ftr);
}

void ftr_draw(ft_renderer_t *ftr)
{
    glUseProgram(ftr->shader);
    glBindVertexArray(ftr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ftr->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, ftr->count * sizeof *ftr->buffer, ftr->buffer);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) ftr->count);
    ftr->count = 0;
}

void ftr_render_text(
        ft_renderer_t *ftr, char const *text, size_t text_size, v2f_t pos, v4f_t fg)
{
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] == '\n') {
            pos.y -= ftr->atlas_h;
            pos.x = 0;
            continue;
        }

        ft_glyph_metrics_t metrics = ftr->metrics[(int) text[i]];
        float x2 = metrics.bl + pos.x;
        float y2 = -metrics.bt - pos.y;
        float w = metrics.bw;
        float h = metrics.bh;

        pos.x += metrics.ax;
        pos.y += metrics.ay;

        assert(ftr->count < GLYPH_BUFFER_CAP);
        ftr->buffer[ftr->count++] = (ft_glyph_t) {
            .pos = v2f(x2, -y2),
            .size = v2f(w, -h),
            .uv_pos = v2f(metrics.tx, 0.0),
            .uv_size = v2f(metrics.bw / ftr->atlas_w, metrics.bh / ftr->atlas_h),
            .fg = fg,
        };
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

void ftr_use(ft_renderer_t const *ftr)
{
    glUseProgram(ftr->shader);
}

static char const *uniform_name(enum ft_uniform ftu);

void ftr_set_float(ft_renderer_t const *ftr, enum ft_uniform u, float f)
{
    glUseProgram(ftr->shader);
    glUniform1f(glGetUniformLocation(ftr->shader, uniform_name(u)), f);
}

void ftr_set_v2f(ft_renderer_t const *ftr, enum ft_uniform u, v2f_t v)
{
    glUseProgram(ftr->shader);
    glUniform2f(glGetUniformLocation(ftr->shader, uniform_name(u)), v2(v));
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

// Initialize glyph vertex array, buffers, attributes, and whatnot

enum ft_glyph_attr {
    GLYPH_ATTR_POS = 0,
    GLYPH_ATTR_SIZE,
    GLYPH_ATTR_UV_POS,
    GLYPH_ATTR_UV_SIZE,
    GLYPH_ATTR_FG,
    GLYPH_ATTR_COUNT,
};

typedef struct {
    GLvoid *ptr;
    GLint size;
    GLenum type;
} ft_glyph_attr_t;

static ft_glyph_attr_t glyph_attrs[GLYPH_ATTR_COUNT] = {
    [GLYPH_ATTR_POS]   = {
        .ptr = (GLvoid *) offsetof(ft_glyph_t, pos),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_SIZE]   = {
        .ptr = (GLvoid *) offsetof(ft_glyph_t, size),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_UV_POS]    = {
        .ptr = (GLvoid *) offsetof(ft_glyph_t, uv_pos),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_UV_SIZE]    = {
        .ptr = (GLvoid *) offsetof(ft_glyph_t, uv_size),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_FG] = {
        .ptr = (GLvoid *) offsetof(ft_glyph_t, fg),
        .size = 4,
        .type = GL_FLOAT,
    },
};

static_assert(GLYPH_ATTR_COUNT == 5, "The amount of ft_glyph attributes has changed.");

static void ftr_init_buffers(ft_renderer_t *ftr)
{
    glGenVertexArrays(1, &ftr->vao);
    glBindVertexArray(ftr->vao);

    glGenBuffers(1, &ftr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ftr->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof ftr->buffer, ftr->buffer, GL_DYNAMIC_DRAW);

    for (enum ft_glyph_attr attr = 0; attr < GLYPH_ATTR_COUNT; attr++) {
        if (glyph_attrs[attr].type == GL_FLOAT) {
            glVertexAttribPointer(
                    attr, glyph_attrs[attr].size, GL_FLOAT, GL_FALSE, sizeof *ftr->buffer,
                    glyph_attrs[attr].ptr);
        } else if (glyph_attrs[attr].type == GL_INT) {
            glVertexAttribIPointer(
                    attr, glyph_attrs[attr].size, GL_INT, sizeof *ftr->buffer,
                    glyph_attrs[attr].ptr);
        } else {
            panic("Type not handled: %d\n", glyph_attrs[attr].type);
        }
        glVertexAttribDivisor(attr, 1);
        glEnableVertexAttribArray(attr);
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
