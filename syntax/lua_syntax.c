#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *lua_extensions[] = {
    ".lua", NULL,
};

const char *lua_keywords[] = {
    "and", "break", "do", "else", "elseif", "end",
    "false", "for", "function", "if", "in", "local",
    "nil", "not", "or", "repeat", "return", "then",
    "true", "until", "while",
    NULL,
};

static bool
Lua_separator(char c)
{
    char *separators = "(){}[]\"\'.,!=><:;+-*/$%&*^~\\|#";
    return (strchr(separators, c) != NULL);
}

static bool
Lua_special(char *tok)
{
    int sz = strlen(tok);

    // constants
    for (int i = 0; i < sz; ++i) {
        if (islower(tok[i]))
            return false;
    }
    return true;
}

highlight_t
Lua_syntax()
{
    return (highlight_t) {
        .extensions = lua_extensions,
        .keywords = lua_keywords,
        .comment = {
            "--",
            { 0, 0 },
        },
        .special = Lua_special,
        .separator = Lua_separator,
    };
}
