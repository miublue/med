#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *c_extensions[] = {
    ".c", ".h", ".cc", NULL,
};

const char *c_keywords[] = {
    "alignas", "alignof", "auto", "bool",
    "break", "case", "char", "const", "asm",
    "constexpr", "continue", "default",
    "do", "double", "else", "enum", "extern",
    "false", "float", "for", "goto", "if",
    "inline", "int", "long", "nullptr", "register",
    "restrict", "return", "short", "signed",
    "sizeof", "static", "static_assert", "struct",
    "switch", "thread_local", "true", "typedef",
    "typeof", "typeof_unqual", "union", "unsigned",
    "void", "volatile", "while", "_Alignas",
    "_Alignof", "_Atomic", "_BitInt", "_Bool",
    "_Complex", "_Decimal128", "_Decimal32",
    "_Decimal64", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local",
    NULL,
};

static bool
C_separator(char c)
{
    char *separators = "(){}[]\"\'.,!=><:;+-*/$%&*^~\\|";
    return (strchr(separators, c) != NULL);
}

static bool
C_special(char *tok)
{
    int sz = strlen(tok);

    // macros
    if (tok[0] == '#')
        return true;

    // custom types (word_t, word_T)
    if (sz > 2) {
        if (tok[sz-2] == '_' && tok[sz-1] == 't' || tok[sz-1] == 'T')
            return true;
    }

    // constants
    for (int i = 0; i < sz; ++i) {
        if (islower(tok[i]))
            return false;
    }
    return true;
}

highlight_t
C_syntax()
{
    return (highlight_t) {
        .extensions = c_extensions,
        .keywords = c_keywords,
        .comment = {
            "//",
            { "/*", "*/" },
        },
        .special = C_special,
        .separator = C_separator,
    };
}
