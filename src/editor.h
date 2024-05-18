#ifndef EDITOR_H
#define EDITOR_H

#include <stdlib.h>
#include <stdbool.h>
#include "mlist.h"
#include "mstring.h"

#define L_LINES  (lines_t) LIST_ALLOC(string_t)

typedef struct lines_t {
    string_t *data;
    size_t size, alloc;
} lines_t;

enum {
    MODE_INSERT,

    MODE_VISUAL,
    MODE_VISUAL_EDIT_START,
    MODE_VISUAL_EDIT_END,

    MODE_DIALOG_SAVE,
    MODE_DIALOG_OPEN,
    MODE_DIALOG_GOTO,
    MODE_DIALOG_FIND,
};

typedef struct visual_t {
    int start, end;
} visual_t;

struct highlight_comment_t {
    const char *scomment; // single-line comment
    const char *mcomment[2]; // multi-line comment
};

typedef struct highlight_t {
    const char **extensions;
    const char **keywords;
    struct highlight_comment_t comment;
    // in case it couldn't find a keyword nor a comment
    // it'll call this function to see if it needs any
    // language-specific highlighting (for example, C macros)
    bool (*special)(char*);
    // language-specific separators
    bool (*separator)(char);
} highlight_t;

LIST_DEFINE(highlight_t, list_highlight_t);

typedef struct editor_t {
    string_t file;
    string_t dialog;
    size_t dialog_cursor;
    lines_t lines;
    size_t line;
    size_t cursor;
    int scroll;
    int win_w, win_h;
    char mode;
    bool show_lines;
    bool quit;
    visual_t visual;
    string_t visual_buffer;

    int csyntax; // current syntax
    list_highlight_t syntax;
} editor_t;

void render_text(editor_t *ed);
void update_editor(editor_t *ed);
editor_t init_editor();
void free_editor(editor_t *ed);

string_t open_file(const char *path);
void write_file(editor_t *ed);
lines_t split_lines(string_t text);

#endif
