#include "free_glyph.h"

#include <assert.h>
#include <stddef.h>

#include "lib.h"
#include "stb_image.h"

static void fgb_init_texture_atlas(free_glyph_buffer_t *fgb, FT_Face face);
static void fgb_init_buffers(free_glyph_buffer_t *fgb);
static void fgb_get_uniform_locations(GLuint program, GLint locations[FTU_COUNT]);

void fgb_init(
        free_glyph_buffer_t *fgb, FT_Face face, char const *vert_filename,
        char const *frag_filename)
{
    fgb->count = 0;

    if (!glslink_program(
                &fgb->shader,
                slice_from((char const *[]) { vert_filename, "shaders/project.glsl" }, 2),
                slice_from(&frag_filename, 1))) {
        panic("Could not load ftglyph shaders.");
    }
    glUseProgram(fgb->shader);

    fgb_init_texture_atlas(fgb, face);
    fgb_init_buffers(fgb);
    fgb_get_uniform_locations(fgb->shader, fgb->u);
}

void fgb_flush(free_glyph_buffer_t *fgb)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, fgb->count * sizeof *fgb->buffer, fgb->buffer);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) fgb->count);
    fgb->count = 0;
}

void fgb_render_text(
        free_glyph_buffer_t *fgb, char const *text, size_t text_size, v2f_t pos, v4f_t fg,
        v4f_t bg)
{
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] == '\n') {
            pos.y -= fgb->atlas_h;
            pos.x = 0;
            continue;
        }

        ftglyph_metrics_t metrics = fgb->metrics[(int) text[i]];
        float x2 = metrics.bl + pos.x;
        float y2 = -metrics.bt - pos.y;
        float w = metrics.bw;
        float h = metrics.bh;

        pos.x += metrics.ax;
        pos.y += metrics.ay;

        assert(fgb->count < GLYPH_BUFFER_CAP);
        fgb->buffer[fgb->count++] = (ftglyph_t) {
            .pos = v2f(x2, -y2),
            .size = v2f(w, -h),
            .uv_pos = v2f(metrics.tx, 0.0),
            .uv_size = v2f(metrics.bw / fgb->atlas_w, metrics.bh / fgb->atlas_h),
            .fg = fg,
            .bg = bg,
        };
    }
}

v2f_t fgb_cursor_pos(free_glyph_buffer_t *fgb, char const *text, size_t text_size)
{
    v2f_t pos = v2fs(0.0);
    for (size_t i = 0; i < text_size; i++) {
        if (text[i] == '\n') {
            pos.y -= fgb->atlas_h;
            pos.x = 0;
            continue;
        }
        ftglyph_metrics_t metrics = fgb->metrics[(int) text[i]];
        pos.x += metrics.ax;
        pos.y += metrics.ay;
    }
    return pos;
}

float fgb_get_widest_line_width(
        free_glyph_buffer_t *fgb, char const *text, size_t text_size, size_t line_start,
        size_t line_count)
{
    size_t line;
    size_t cursor;
    for (cursor = 0, line = 0; cursor < text_size && line < line_start; cursor++) {
        if (text[cursor] == '\n') {
            line++;
        }
    }
    float width = 0;
    float widest_width = 0;
    for (; cursor < text_size && line < line_start + line_count; cursor++) {
        if (text[cursor] == '\n') {
            line++;
            width = 0;
            continue;
        }
        width += fgb_char_width(fgb, text[cursor]);
        if (width > widest_width) {
            widest_width = width;
        }
    } 
    return widest_width;
}

float fgb_char_width(free_glyph_buffer_t *fgb, char c)
{
    ftglyph_metrics_t metrics = fgb->metrics[(int) c];
    return metrics.ax;
}

static void fgb_init_texture_atlas(free_glyph_buffer_t *fgb, FT_Face face)
{
    fgb->atlas_w = 0;
    fgb->atlas_h = 0;
    fgb->atlas_low = 0;

    FT_Error error;
    for (int i = 32; i < 128; i++) {
        if ((error = FT_Load_Char(face, i, FT_LOAD_RENDER)) != FT_Err_Ok) {
            eprintln("Error loading char %c: %s\n", i, FT_Error_String(error));
            continue;
        }
        FT_GlyphSlot gs = face->glyph;
        fgb->atlas_w += gs->bitmap.width;
        if (fgb->atlas_h < gs->bitmap.rows) {
            fgb->atlas_h = gs->bitmap.rows;
        }
        int low = (int) gs->bitmap.rows - gs->bitmap_top;
        if ((int) fgb->atlas_low < low) {
            fgb->atlas_low = low;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &fgb->atlas);
    glBindTexture(GL_TEXTURE_2D, fgb->atlas);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED, (GLsizei) fgb->atlas_w, (GLsizei) fgb->atlas_h, 0,
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

        ftglyph_metrics_t *gm = fgb->metrics + i;
        gm->ax = gs->advance.x >> 6;
        gm->ay = gs->advance.y >> 6;
        gm->bw = gs->bitmap.width;
        gm->bh = gs->bitmap.rows;
        gm->bl = gs->bitmap_left;
        gm->bt = gs->bitmap_top;
        gm->tx = (float) x / fgb->atlas_w;

        x += face->glyph->bitmap.width;
    }
}

// Initialize glyph vertex array, buffers, attributes, and whatnot

enum ftglyph_attr {
    GLYPH_ATTR_POS = 0,
    GLYPH_ATTR_SIZE,
    GLYPH_ATTR_UV_POS,
    GLYPH_ATTR_UV_SIZE,
    GLYPH_ATTR_FG,
    GLYPH_ATTR_BG,
    GLYPH_ATTR_COUNT,
};

typedef struct {
    GLvoid *ptr;
    GLint size;
    GLenum type;
} ftglyph_attr_t;

static ftglyph_attr_t glyph_attrs[GLYPH_ATTR_COUNT] = {
    [GLYPH_ATTR_POS]   = {
        .ptr = (GLvoid *) offsetof(ftglyph_t, pos),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_SIZE]   = {
        .ptr = (GLvoid *) offsetof(ftglyph_t, size),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_UV_POS]    = {
        .ptr = (GLvoid *) offsetof(ftglyph_t, uv_pos),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_UV_SIZE]    = {
        .ptr = (GLvoid *) offsetof(ftglyph_t, uv_size),
        .size = 2,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_FG] = {
        .ptr = (GLvoid *) offsetof(ftglyph_t, fg),
        .size = 4,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_BG] = {
        .ptr = (GLvoid *) offsetof(ftglyph_t, bg),
        .size = 4,
        .type = GL_FLOAT,
    },
};

static_assert(GLYPH_ATTR_COUNT == 6, "The amount of ftglyph attributes has changed.");

static void fgb_init_buffers(free_glyph_buffer_t *fgb)
{
    glGenVertexArrays(1, &fgb->vao);
    glBindVertexArray(fgb->vao);

    glGenBuffers(1, &fgb->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, fgb->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof fgb->buffer, fgb->buffer, GL_DYNAMIC_DRAW);

    for (enum ftglyph_attr attr = 0; attr < GLYPH_ATTR_COUNT; attr++) {
        if (glyph_attrs[attr].type == GL_FLOAT) {
            glVertexAttribPointer(
                    attr, glyph_attrs[attr].size, GL_FLOAT, GL_FALSE, sizeof *fgb->buffer,
                    glyph_attrs[attr].ptr);
        } else if (glyph_attrs[attr].type == GL_INT) {
            glVertexAttribIPointer(
                    attr, glyph_attrs[attr].size, GL_INT, sizeof *fgb->buffer,
                    glyph_attrs[attr].ptr);
        } else {
            panic("Type not handled: %d\n", glyph_attrs[attr].type);
        }
        glVertexAttribDivisor(attr, 1);
        glEnableVertexAttribArray(attr);
    }
}

static char const *uniform_name(enum ftuniform ftu)
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

static void fgb_get_uniform_locations(GLuint program, GLint locations[FTU_COUNT])
{
    for (enum ftuniform ftu = 0; ftu < FTU_COUNT; ftu++) {
        locations[ftu] = glGetUniformLocation(program, uniform_name(ftu));
    }
}
