#include "lexer.h"

lexer_token_t lexer_lex(char const *src, size_t length)
{
    lexer_token_t tok = { 0 };
    size_t i;
    for (i = 0; i < length; i++) {
        tok.begin = i;
        if (src[i++] == '"') {
            tok.kind = LEXER_TOKEN_STRING;
            while (src[i++] != '"' && i < length) {
                continue;
            }
            tok.end = i;
            break;
        } else {
            while (src[i] != '"' && i < length) {
                i++;
            }
            tok.end = i;
            break;
        }
    }
    if (i == length) {
        tok.kind = LEXER_TOKEN_EOF;
    }
    return tok;
}
