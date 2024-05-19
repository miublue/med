#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *miu_extensions[] = {
    ".miu", NULL,
};

const char *miu_keywords[] = {
    // keywords
    "def", "return", "if", "else", "while",
    "for", "break", "next", "mut", "struct",

    // base types
    "string", "bool", "float", "double",
    "byte", "ubyte", "short", "ushort",
    "int", "uint", "long", "ulong",
    "i8", "u8", "i16", "u16",
    "i32", "u32", "i64", "u64",
    "f32", "f64", "f128", "f256",
    "array", "true", "false", "null",

    NULL,
};

static bool
miu_separator(char c)
{
    char *separators = "(){}[]\"\'.,!=><:;+-*/$%&*^~\\|";
    return (strchr(separators, c) != NULL);
}

static bool
miu_special(char *tok)
{
    int sz = strlen(tok);

    // compiler directives
    if (tok[0] == '@')
        return true;

    return false;
}

highlight_t
Miu_syntax()
{
    return (highlight_t) {
        .extensions = miu_extensions,
        .keywords = miu_keywords,
        .comment = {
            "//",
            { "/*", "*/" },
        },
        .special = miu_special,
        .separator = miu_separator,
    };
}
