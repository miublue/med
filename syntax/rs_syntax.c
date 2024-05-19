#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "editor.h"

const char *rs_extensions[] = {
    ".rs", NULL,
};

const char *rs_keywords[] = {
    "as", "use", "extern", "break", "const",
    "continue", "crate", "else", "if", "final",
    "enum", "extern", "false", "fn", "for",
    "impl", "in", "for", "let", "loop", "match",
    "mod", "move", "mut", "pub", "impl", "ref",
    "return", "Self", "self", "static", "struct",
    "super", "trait", "true", "type", "unsafe",
    "use", "where", "while", "abstract", "alignof",
    "become", "box", "do", "final", "macro", "offsetof",
    "override", "priv", "proc", "pure", "sizeof",
    "typeof", "unsized", "virtual", "yield", "try",
    "dyn", "async", "await", "union", "macro_rules",

    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize",
    "f32", "f64", "f128", "bool", "char", "str",
    "String", "Vec",

    NULL,
};

// TODO: uhh how tf do i ' bruh

static bool
Rust_separator(char c)
{
    char *separators = "(){}[]\"\'.,=><:;+-*/$%&*^~\\|#@!";
    return (strchr(separators, c) != NULL);
}

static bool
Rust_special(char *tok)
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
Rust_syntax()
{
    return (highlight_t) {
        .extensions = rs_extensions,
        .keywords = rs_keywords,
        .comment = {
            "//",
            { "/*", "*/" },
        },
        .special = Rust_special,
        .separator = Rust_separator,
    };
}
