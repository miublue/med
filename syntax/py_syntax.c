#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *py_extensions[] = {
    ".py", "pyw", NULL,
};

const char *py_keywords[] = {
    "False", "None", "True", "and", "as",
    "assert", "async", "await", "break",
    "class", "continue", "def", "del",
    "elif", "else", "except", "finally",
    "for", "from", "global", "if", "import",
    "in", "is", "lambda", "nonlocal", "not",
    "or", "pass", "raise", "return", "try",
    "while", "with", "yield",
    NULL,
};

static bool
Python_separator(char c)
{
    char *separators = "(){}[]\"\'.,!=><:;+-*/$%&*^~\\|";
    return (strchr(separators, c) != NULL);
}

static bool
Python_special(char *tok)
{
    int sz = strlen(tok);

    if (tok[0] == '@')
        return true;

    if (sz > 4) {
        if (strncmp(tok, "__", 2) == 0 && strncmp(tok+sz-2, "__", 2) == 0)
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
Python_syntax()
{
    return (highlight_t) {
        .extensions = py_extensions,
        .keywords = py_keywords,
        .comment = {
            "#",
            { 0, 0 },
        },
        .special = Python_special,
        .separator = Python_separator,
    };
}
