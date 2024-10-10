#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>

enum lexer_token {
    LEXER_TOKEN_INVALID = 0,
    LEXER_TOKEN_STRING,
    LEXER_TOKEN_EOF,
};

typedef struct {
    enum lexer_token kind;
    size_t begin; // Inclusive
    size_t end; // Exclusive
} lexer_token_t;

lexer_token_t lexer_lex(const char *src, size_t length);

#endif // LEXER_H_
