#include "tile_glyph.h"

#include <assert.h>
#include <stddef.h>

#include "lib.h"
#include "stb_image.h"

static void tgb_push(tile_glyph_buffer_t *tgb, glyph_t glyph);
static void tgb_sync(tile_glyph_buffer_t *tgb);

enum glyph_attr {
    GLYPH_ATTR_TILE = 0,
    GLYPH_ATTR_CH,
    GLYPH_ATTR_FG,
    GLYPH_ATTR_BG,
    GLYPH_ATTR_COUNT,
};

typedef struct {
    GLvoid *ptr;
    GLint size;
    GLenum type;
} glyph_attr_t;


static glyph_attr_t glyph_attrs[GLYPH_ATTR_COUNT] = {
    [GLYPH_ATTR_TILE]   = {
        .ptr = (GLvoid *) offsetof(glyph_t, tile),
        .size = 2,
        .type = GL_INT,
    },
    [GLYPH_ATTR_CH]    = {
        .ptr = (GLvoid *) offsetof(glyph_t, ch),
        .size = 1,
        .type = GL_INT,
    },
    [GLYPH_ATTR_FG] = {
        .ptr = (GLvoid *) offsetof(glyph_t, fg),
        .size = 4,
        .type = GL_FLOAT,
    },
    [GLYPH_ATTR_BG] = {
        .ptr = (GLvoid *) offsetof(glyph_t, bg),
        .size = 4,
        .type = GL_FLOAT,
    },
};

static_assert(GLYPH_ATTR_COUNT == 4, "The amount of glyph attributes has changed.");

void tgb_init(
        tile_glyph_buffer_t *tgb, char const *atlas_filename, char const *vert_filename,
        char const *frag_filename)
{
    tgb->count = 0;

    if (!shader_init(&tgb->shader, vert_filename, frag_filename)) {
        panic("Could not load glyph shaders.");
    }
    glUseProgram(tgb->shader);

    // Initialize textures
    {
        int width, height, n;
        unsigned char *pixels =
                stbi_load(atlas_filename, &width, &height, &n, STBI_rgb_alpha);
        if (pixels == NULL) {
            panic("STBI ERROR: Could not load \"%s\": %s", atlas_filename,
                  stbi_failure_reason());
        }

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &tgb->atlas);
        glBindTexture(GL_TEXTURE_2D, tgb->atlas);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                pixels);
    }

    // Initialize Arrays and Buffers
    {
        glGenVertexArrays(1, &tgb->vao);
        glBindVertexArray(tgb->vao);

        glGenBuffers(1, &tgb->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, tgb->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof tgb->buffer, tgb->buffer, GL_DYNAMIC_DRAW);

        for (enum glyph_attr attr = 0; attr < GLYPH_ATTR_COUNT; attr++) {
            if (glyph_attrs[attr].type == GL_FLOAT) {
                glVertexAttribPointer(
                        attr, glyph_attrs[attr].size, GL_FLOAT, GL_FALSE,
                        sizeof *tgb->buffer, glyph_attrs[attr].ptr);
            } else if (glyph_attrs[attr].type == GL_INT) {
                glVertexAttribIPointer(
                        attr, glyph_attrs[attr].size, GL_INT, sizeof *tgb->buffer,
                        glyph_attrs[attr].ptr);
            } else {
                assert(false && "Type not handled");
                exit(1);
            }
            glVertexAttribDivisor(attr, 1);
            glEnableVertexAttribArray(attr);
        }
    }

    // Obtain Uniform Locations
    {
        tgb->u_time = glGetUniformLocation(tgb->shader, "u_time");
        tgb->u_resolution = glGetUniformLocation(tgb->shader, "u_resolution");
        tgb->u_scale = glGetUniformLocation(tgb->shader, "u_scale");
        tgb->u_camera = glGetUniformLocation(tgb->shader, "u_camera");
    }
}

void tgb_flush(tile_glyph_buffer_t *tgb)
{
    tgb_sync(tgb);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, tgb->count);
    tgb->count = 0;
}

static void tgb_sync(tile_glyph_buffer_t *tgb)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, tgb->count * sizeof *tgb->buffer, tgb->buffer);
}

void tgb_render_text(
        tile_glyph_buffer_t *tgb, char const *text, size_t text_size, v2i_t tile,
        v4f_t fg, v4f_t bg)
{
    long row = 0;
    long col = 0;
    for (size_t i = 0; i < text_size; i++, col++) {
        if (text[i] == '\n') {
            row--;
            col = -1;
            continue;
        }
        tgb_push(
                tgb, (glyph_t) {
                             .tile = v2i_add(tile, v2i(col, row)),
                             .ch = text[i],
                             .fg = fg,
                             .bg = bg,
                     });
    }
}

static void tgb_push(tile_glyph_buffer_t *tgb, glyph_t glyph)
{
    assert(tgb->count < GLYPH_BUFFER_CAP);
    tgb->buffer[tgb->count++] = glyph;
}
