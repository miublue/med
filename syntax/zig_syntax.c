#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *zig_extensions[] = {
    ".zig", NULL,
};

const char *zig_keywords[] = {
    "addrspace", "align", "allowzero", "and",
    "anyframe", "anytype", "asm", "async", "await", "break",
    "callconv", "catch", "comptime", "const", "continue",
    "defer", "else", "enum", "errdefer", "error",
    "export", "extern", "fn", "for", "if", "inline",
    "linksection", "noalias", "noinline", "nosuspend",
    "opaque", "or", "orelse", "packed", "pub", "resume",
    "return", "struct", "suspend", "switch", "test",
    "threadLocal", "try", "union", "unreachable",
    "usingnamespace", "var", "volatile", "while",

    "void", "bool", "isize", "usize", "c_char", "c_short",
    "c_ushort", "c_int", "c_uint", "c_long", "c_ulong",
    "c_longlong", "c_ulonglong", "c_longdouble", "anyopaque",
    "noreturn", "type", "anyerror", "comptime_int", "comptime_float",
    "true", "false", "null", "undefined",
    NULL,
};

static bool
Zig_separator(char c)
{
    char *separators = "(){}[]\"\'?.,!=><:;+-*/$%&*^~\\|";
    return (strchr(separators, c) != NULL);
}

static bool
Zig_special(char *tok)
{
    int sz = strlen(tok);

    // builtin functions
    if (tok[0] == '@')
        return true;

    // base number types
    if (sz > 1 && (tok[0] == 'i' || tok[0] == 'u' || tok[0] == 'f') && isdigit(tok[1]))
        return true;

    // constants
    for (int i = 0; i < sz; ++i) {
        if (islower(tok[i]))
            return false;
    }
    return true;
}

highlight_t
Zig_syntax()
{
    return (highlight_t) {
        .extensions = zig_extensions,
        .keywords = zig_keywords,
        .comment = {
            "//",
            { "/*", "*/" },
        },
        .special = Zig_special,
        .separator = Zig_separator,
    };
}
