#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *masm_extensions[] = {
    ".masm", NULL,
};

const char *masm_keywords[] = {
    "push", "pop", "copy", "move",
    "halt", "call", "jmp", "jnz", "jz",
    "eq", "neq", "gt", "lt", "or",
    "add", "sub", "mul", "div",
    NULL,
};

static bool
Masm_separator(char c)
{
    char *sep = "(){}[]\"\'";
    return strchr(sep, c) != NULL;
}

static bool
Masm_special(char *tok)
{
    int sz = strlen(tok);

    // types
    if (tok[0] == '.')
        return true;

    // labels
    if (tok[sz-1] == ':')
        return true;

    return false;
}

highlight_t
Masm_syntax()
{
    return (highlight_t) {
        .extensions = masm_extensions,
        .keywords = masm_keywords,
        .comment = {
            "#",
            { 0, 0 },
        },
        .special = Masm_special,
        .separator = Masm_separator,
    };
}
