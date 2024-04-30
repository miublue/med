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
} editor_t;

void render_text(editor_t *ed);
void update_editor(editor_t *ed);
editor_t init_editor();
void free_editor(editor_t *ed);

string_t open_file(const char *path);
void write_file(editor_t *ed);
lines_t split_lines(string_t text);

#endif
