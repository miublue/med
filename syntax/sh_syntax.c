#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *sh_extensions[] = {
    ".sh", ".bash", ".bashrc", ".profile", NULL,
};

const char *sh_keywords[] = {
    "alias", "export", "source", "set", "unset", "unalias",
    "if", "then", "elif", "else", "fi", "time",
    "for", "in", "until", "while", "do", "done",
    "case", "esac", "coproc", "select", "function",
    "{", "}", "[", "]", "[[", "]]", "(", ")", "!",
    "true", "false", "yes", "no",
    NULL,
};

static bool
sh_separator(char c)
{
    char *separators = "\"\',><:;+*/%&*^~\\=";
    return (strchr(separators, c) != NULL);
}

static bool
sh_special(char *tok)
{
    if (tok[0] == '$' || tok[0] == '@' || tok[0] == '-')
        return true;

    return false;
}

highlight_t
Sh_syntax()
{
    return (highlight_t) {
        .extensions = sh_extensions,
        .keywords = sh_keywords,
        .comment = {
            "#",
            { 0, 0 },
        },
        .special = sh_special,
        .separator = sh_separator,
    };
}
