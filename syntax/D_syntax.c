#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *d_extensions[] = {
    ".d", ".di", NULL,
};

const char *d_keywords[] = {
    "abstract", "alias", "align", "asm", "assert",
    "auto", "body", "bool", "break", "byte", "case",
    "cast", "catch", "cdouble", "cent", "cfloat",
    "char", "class", "const", "continue", "creal",
    "dchar", "debug", "default", "delegate", "delete",
    "deprecated", "do", "double", "else", "enum", "export",
    "extern", "false", "final", "finally", "float", "for",
    "foreach", "foreach_reverse", "function", "goto",
    "idouble", "if", "ifloat", "immutable", "import", "in",
    "inout", "int", "interface", "invariant", "ireal", "is",
    "lazy", "long", "macro", "mixin", "module", "new",
    "nothrow", "null", "out", "override", "package", "pragma",
    "private", "protected", "public", "pure", "real", "ref",
    "return", "scope", "shared", "short", "static", "struct",
    "super", "switch", "synchronized", "template", "this",
    "throw", "true", "try", "while", "with", "typeid",
    "typeof", "ubyte", "ucent", "uint", "ulong", "union",
    "unittest", "ushort", "version", "void", "wchar",
    "string", "dstring", "wstring",
    "__FILE__", "__FILE_FULL_PATH__", "__MODULE__",
    "__LINE__", "__FUNCTION__", "__PRETTY_FUNCTION__",
    "__gshared", "__traits", "__vector", "__parameters",
    NULL,
};

static bool
D_separator(char c)
{
    char *separators = "(){}[]\"\'.,!=><:;+-*/$%&*^~\\|";
    return (strchr(separators, c) != NULL);
}

static bool
D_special(char *tok)
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
D_syntax()
{
    return (highlight_t) {
        .extensions = d_extensions,
        .keywords = d_keywords,
        .comment = {
            "//",
            { "/*", "*/" },
        },
        .special = D_special,
        .separator = D_separator,
    };
}
