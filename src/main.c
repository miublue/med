#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "mlist.h"
#include "mstring.h"
#include "editor.h"

// #define CTRL(c) ((c) & 037)
#define CTRL(c) ((c) & 0x1f)
#define PAIR_NORMAL 0
#define PAIR_SELECT 1
#define PAIR_LINES 2
#define PAIR_STATUS_MESSAGE 3
#define PAIR_STATUS_MODE 4
#define PAIR_HL_KEYWORD 5
#define PAIR_HL_NUMBER 6
#define PAIR_HL_STRING 7
#define PAIR_HL_CONSTANT 8
#define PAIR_HL_COMMENT 9

#define TAB_WIDTH 4

// TODO: horizontal scrolling
// TODO: proper clipboard
// TODO: proper selection mode
// TODO: separate search buffer from dialog + replace
// TODO: file explorer

#define STATUS_SIZE 1024
#define STATUS_DELAY 1
char status_message[STATUS_SIZE] = {0};
int status_delay = 0;
lines_t clip;
lines_t save_buffer;
lines_t find_buffer;
int buffer_pos = 0;

void
init_curses()
{
    initscr();
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();

    init_pair(PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(PAIR_SELECT, COLOR_BLACK, COLOR_WHITE);
    init_pair(PAIR_LINES, COLOR_RED, COLOR_BLACK);
    init_pair(PAIR_STATUS_MESSAGE, COLOR_WHITE, COLOR_RED);
    init_pair(PAIR_STATUS_MODE, COLOR_BLACK, COLOR_BLUE);

    init_pair(PAIR_HL_KEYWORD, COLOR_BLUE, COLOR_BLACK);
    init_pair(PAIR_HL_NUMBER, COLOR_RED, COLOR_BLACK);
    init_pair(PAIR_HL_STRING, COLOR_RED, COLOR_BLACK);
    init_pair(PAIR_HL_CONSTANT, COLOR_RED, COLOR_BLACK);
    init_pair(PAIR_HL_COMMENT, COLOR_GREEN, COLOR_BLACK);
}

void
end_curses()
{
    endwin();
    curs_set(1);
}

void
render_dialog(editor_t *ed, int x)
{
    mvprintw(ed->win_h-1, x, STR_FMT, STR_ARG(ed->dialog));

    attron(COLOR_PAIR(PAIR_SELECT));

    int ch = ' ';
    if (ed->dialog_cursor < ed->dialog.size)
        ch = ed->dialog.data[ed->dialog_cursor];

    mvprintw(ed->win_h-1, x + ed->dialog_cursor, "%c", ch);
    attroff(COLOR_PAIR(PAIR_SELECT));
}

int
update_dialog(editor_t *ed)
{
    int ch = getch();
    switch (ch) {
        case CTRL('q'):
        case CTRL('c'): {
            ed->mode = MODE_INSERT;
        } break;
        case CTRL('x'): {
            ed->dialog.size = 0;
            ed->dialog_cursor = 0;
        } break;
        case CTRL('p'): {
            if (clip.size) {
                for (size_t i = 0; i < clip.size; ++i) {
                    LIST_ADD(ed->dialog, ed->dialog_cursor, clip.data[0].data[i]);
                    ed->dialog_cursor++;
                }
            }
        } break;
        case KEY_LEFT: {
            if (ed->dialog_cursor > 0)
                ed->dialog_cursor--;
        } break;
        case KEY_RIGHT: {
            if (ed->dialog_cursor < ed->dialog.size)
                ed->dialog_cursor++;
        } break;
        case KEY_UP: {
            if (ed->mode == MODE_DIALOG_OPEN || ed->mode == MODE_DIALOG_SAVE) {
                if (!save_buffer.size) break;
                ed->dialog.size = 0;
                for (size_t i = 0; i < save_buffer.data[buffer_pos].size; ++i) {
                    LIST_ADD(ed->dialog, ed->dialog.size, save_buffer.data[buffer_pos].data[i]);
                }
                ed->dialog_cursor = ed->dialog.size;
                if (++buffer_pos >= save_buffer.size) {
                    buffer_pos = 0;
                }
            }
            else if (ed->mode == MODE_DIALOG_FIND) {
                if (!find_buffer.size) break;
                ed->dialog.size = 0;
                for (size_t i = 0; i < find_buffer.data[buffer_pos].size; ++i) {
                    LIST_ADD(ed->dialog, ed->dialog.size, find_buffer.data[buffer_pos].data[i]);
                }
                ed->dialog_cursor = ed->dialog.size;
                if (++buffer_pos >= find_buffer.size) {
                    buffer_pos = 0;
                }
            }
        } break;
        case KEY_DOWN: {
            if (ed->mode == MODE_DIALOG_OPEN || ed->mode == MODE_DIALOG_SAVE) {
                if (!save_buffer.size) break;
                ed->dialog.size = 0;
                for (size_t i = 0; i < save_buffer.data[buffer_pos].size; ++i) {
                    LIST_ADD(ed->dialog, ed->dialog.size, save_buffer.data[buffer_pos].data[i]);
                }
                ed->dialog_cursor = ed->dialog.size;
                if (--buffer_pos < 0) {
                    buffer_pos = save_buffer.size-1;
                }
            }
            else if (ed->mode == MODE_DIALOG_FIND) {
                if (!find_buffer.size) break;
                ed->dialog.size = 0;
                for (size_t i = 0; i < find_buffer.data[buffer_pos].size; ++i) {
                    LIST_ADD(ed->dialog, ed->dialog.size, find_buffer.data[buffer_pos].data[i]);
                }
                ed->dialog_cursor = ed->dialog.size;
                if (--buffer_pos < 0) {
                    buffer_pos = find_buffer.size-1;
                }
            }
        } break;
        case KEY_HOME: {
            ed->dialog_cursor = 0;
        } break;
        case KEY_END: {
            ed->dialog_cursor = ed->dialog.size;
        } break;
        case 8: // shift + backspace
        case KEY_BACKSPACE: {
            if (ed->dialog_cursor > 0) {
                ed->dialog_cursor--;
                LIST_POP(ed->dialog, ed->dialog_cursor);
            }
        } break;
        case KEY_DC: {
            if (ed->dialog_cursor < ed->dialog.size) {
                LIST_POP(ed->dialog, ed->dialog_cursor);
            }
        } break;
        case '\n': {
            // dumb dumb
            string_t str = L_STRING;
            for (size_t i = 0; i < ed->dialog.size; ++i) {
                LIST_ADD(str, str.size, ed->dialog.data[i]);
            }
            if (ed->mode == MODE_DIALOG_OPEN || ed->mode == MODE_DIALOG_SAVE) {
                LIST_ADD(save_buffer, save_buffer.size, str);
            }
            else if (ed->mode == MODE_DIALOG_FIND) {
                LIST_ADD(find_buffer, find_buffer.size, str);
            }
            else {
                LIST_FREE(str);
            }
            ed->dialog_cursor = ed->dialog.size;
            ed->mode = MODE_INSERT;
            return 1;
        } break;
        case '\t': {
            // TODO: maybe autocomplete based on save_buffer/find_buffer
            LIST_ADD(ed->dialog, ed->dialog_cursor, ' ');
            ed->dialog_cursor++;
        } break;
        default: {
            LIST_ADD(ed->dialog, ed->dialog_cursor, ch);
            ed->dialog_cursor++;
        } break;
    }

    return 0;
}

char*
mode_to_str(int mode)
{
    switch (mode) {
    case MODE_INSERT:
        return "INSERT";
    case MODE_VISUAL:
        return "VISUAL";
    case MODE_VISUAL_EDIT_START:
        return "VISUAL EDIT";
    case MODE_VISUAL_EDIT_END:
        return "VISUAL EDIT";
    case MODE_DIALOG_SAVE:
        return "SAVE";
    case MODE_DIALOG_OPEN:
        return "OPEN";
    case MODE_DIALOG_GOTO:
        return "GOTO";
    case MODE_DIALOG_FIND:
        return "FIND";
    default:
        return "NORMAL";
    }
}

void
render_status(editor_t *ed)
{
    move(ed->win_h-1, 0);
    clrtoeol();
    char *mode = mode_to_str(ed->mode);

    char file_stat[500];
    sprintf(file_stat, STR_FMT" %d:%d",
            STR_ARG(ed->file), ed->line+1, ed->cursor+1);

    attron(COLOR_PAIR(PAIR_LINES));
    mvprintw(ed->win_h-1, ed->win_w-strlen(file_stat)-1, "%s", file_stat);
    attroff(COLOR_PAIR(PAIR_LINES));

    if (status_message && status_delay) {
        status_delay--;
        attron(COLOR_PAIR(PAIR_STATUS_MESSAGE));
        mvprintw(ed->win_h-1, strlen(mode)+2, " %s ", status_message);
        attroff(COLOR_PAIR(PAIR_STATUS_MESSAGE));
    }

    attron(COLOR_PAIR(PAIR_STATUS_MODE));
    mvprintw(ed->win_h-1, 0, " %s ", mode);
    // mvprintw(ed->win_h-1, ed->win_w-strlen(mode)-2, " %s ", mode);
    attroff(COLOR_PAIR(PAIR_STATUS_MODE));

    // mvprintw(ed->win_h-1, ed->win_w-20, "%s [%d]", keyname(ch), ch);

    if (ed->mode >= MODE_DIALOG_SAVE) {
        render_dialog(ed, strlen(mode)+3);
    }
}

#define CURR_LINE ed->lines.data[ed->line]

#define CL ed->lines.data[line]
#define LEN(x) (sizeof(x) / (sizeof((x)[0])))

static char *keywords[] = {
    "alignas", "alignof", "auto", "bool",
    "break", "case", "char", "const", "asm",
    "constexpr", "continue", "default",
    "do", "double", "else", "enum", "extern",
    "false", "float", "for", "goto", "if",
    "inline", "int", "long", "nullptr", "register",
    "restrict", "return", "short", "signed",
    "sizeof", "static", "static_assert", "struct",
    "switch", "thread_local", "true", "typedef",
    "typeof", "typeof_unqual", "union", "unsigned",
    "void", "volatile", "while", "_Alignas",
    "_Alignof", "_Atomic", "_BitInt", "_Bool",
    "_Complex", "_Decimal128", "_Decimal32",
    "_Decimal64", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local",
};

// static char *keywords[] = {
//     "struct", "union", "enum", "typedef",
//     "static", "inline", "const", "return",
//     "for", "while", "do", "if", "else",
//     "continue", "switch", "case",
//     "default", "break",
// 
//     "auto", "void", "int", "long", "bool",
//     "float", "double", "size_t", "char",
// };

static int
is_word(char c)
{
    return isalnum(c) || c == '_';
}

static int
is_word_start(char c)
{
    return isalpha(c) || c == '_';
}

static int
is_number(char c)
{
    return isdigit(c) || c == '.';
}

static int
is_keyword(char *tok)
{
    int sz = strlen(tok);

    for (int i = 0; i < LEN(keywords); ++i) {
        if (strlen(keywords[i]) != sz)
            continue;
        if (strncmp(tok, keywords[i], sz) == 0)
            return true;
    }

    // custom type
    if (sz > 2 && tok[sz-2] == '_' && (tok[sz-1] == 't' || tok[sz-1] == 'T')) {
        return true;
    }

    return false;
}

static int
is_constant(char *tok)
{
    int sz = strlen(tok);
    for (int i = 0; i < sz; ++i) {
        if (islower(tok[i]))
            return false;
    }
    return true;
}

static inline char*
token(editor_t *ed, size_t line, size_t pos, size_t size)
{
    char tok[size+1];
    memcpy(tok, CL.data+pos, size);
    tok[size] = '\0';
    return tok;
}

// TODO: for some reason comments don't highlight properly if line is too long (like this)
// TODO: syntax highlighting for other languages
// TODO: crop line if too long (+ add horizontal scrolling)
static void
render_line(editor_t *ed, size_t line, size_t off)
{
    size_t l = line - ed->scroll;
    size_t i = 0;
    size_t s = 0;

    mvprintw(l, off, STR_FMT, STR_ARG(CL));

    while (i < CL.size) {
        if (i+1 < CL.size && CL.data[i] == '/' && CL.data[i+1] == '/') {
            s = i;
            i = CL.size;
            char *tok = token(ed, line, s, i-s);
            attron(COLOR_PAIR(PAIR_HL_COMMENT));
            mvprintw(l, off+s, "%s", tok);
            attroff(COLOR_PAIR(PAIR_HL_COMMENT));
        }
        else if (CL.data[i] == '\"') {
            s = i++;
            while (i < CL.size && CL.data[i] != '\"') {
                ++i;
                bool escape = CL.data[i-1] == '\\' && CL.data[i-2] == '\\';
                if (CL.data[i] == '\"' && CL.data[i-1] == '\\') {
                    if (escape) continue;
                    ++i;
                }
            }
            if (i < CL.size) ++i;
            char *tok = token(ed, line, s, i-s);
            attron(COLOR_PAIR(PAIR_HL_STRING));
            mvprintw(l, off+s, "%s", tok);
            attroff(COLOR_PAIR(PAIR_HL_STRING));
        }
        else if (CL.data[i] == '\'') {
            s = i++;
            while (i < CL.size && CL.data[i] != '\'') {
                ++i;
                bool escape = CL.data[i-1] == '\\' && CL.data[i-2] == '\\';
                if (CL.data[i] == '\'' && CL.data[i-1] == '\\') {
                    if (escape) continue;
                    ++i;
                }
            }
            if (i < CL.size) ++i;
            char *tok = token(ed, line, s, i-s);
            attron(COLOR_PAIR(PAIR_HL_STRING));
            mvprintw(l, off+s, "%s", tok);
            attroff(COLOR_PAIR(PAIR_HL_STRING));
        }
        else if (isdigit(CL.data[i])) {
            s = i;
            while (i < CL.size && is_number(CL.data[i])) ++i;
            char *tok = token(ed, line, s, i-s);
            attron(COLOR_PAIR(PAIR_HL_NUMBER));
            mvprintw(l, off+s, "%s", tok);
            attroff(COLOR_PAIR(PAIR_HL_NUMBER));
        }
        else if (is_word_start(CL.data[i])) {
            s = i;
            while (i < CL.size && is_word(CL.data[i])) ++i;
            char *tok = token(ed, line, s, i-s);
            if (is_keyword(tok)) {
                attron(COLOR_PAIR(PAIR_HL_KEYWORD));
                mvprintw(l, off+s, "%s", tok);
                attroff(COLOR_PAIR(PAIR_HL_KEYWORD));
            }
            else if (is_constant(tok)) {
                attron(COLOR_PAIR(PAIR_HL_CONSTANT));
                mvprintw(l, off+s, "%s", tok);
                attroff(COLOR_PAIR(PAIR_HL_CONSTANT));
            }
        }
        else if (CL.data[i] == '#') {
            s = i++;
            while (i < CL.size && is_word(CL.data[i])) ++i;
            char *tok = token(ed, line, s, i-s);
            attron(COLOR_PAIR(PAIR_HL_KEYWORD));
            mvprintw(l, off+s, "%s", tok);
            attroff(COLOR_PAIR(PAIR_HL_KEYWORD));
        }
        else {
            ++i;
        }
    }
}

#undef CL

void
render_text(editor_t *ed)
{
    char line_num[16] = {0};
    size_t line_size = 0;

    sprintf(line_num, "%d ", ed->lines.size);
    line_size = strlen(line_num);

    int start = (ed->visual.start < ed->visual.end)?
                    ed->visual.start : ed->visual.end;
    int end = (ed->visual.start < ed->visual.end)?
                    ed->visual.end : ed->visual.start;

    for (size_t i = 0; i < ed->lines.size; ++i) {
        if (i - ed->scroll < 0 || i - ed->scroll+1 > ed->win_h) continue;

        sprintf(line_num, "%d ", i + 1);
        move(i - ed->scroll, 0);
        if (ed->show_lines) {
            attron(COLOR_PAIR(PAIR_LINES));
            printw("%s", line_num);
            attroff(COLOR_PAIR(PAIR_LINES));
        }

        if (ed->mode == MODE_VISUAL && i >= start && i <= end) {
            attron(COLOR_PAIR(PAIR_SELECT));
            mvprintw(i - ed->scroll, line_size, STR_FMT, STR_ARG(ed->lines.data[i]));
            attroff(COLOR_PAIR(PAIR_SELECT));
        }
        else if (ed->mode == MODE_VISUAL_EDIT_START && i >= start && i <= end) {
            mvprintw(i - ed->scroll, line_size, STR_FMT, STR_ARG(ed->visual_buffer));
            render_line(ed, i, line_size + ed->visual_buffer.size);

            attron(COLOR_PAIR(PAIR_SELECT));
            move(i - ed->scroll, ed->cursor + line_size);
            int current_char = (ed->lines.data[i].size)? ed->lines.data[i].data[0] : ' ';
            if (ed->cursor < ed->visual_buffer.size)
                current_char = ed->visual_buffer.data[ed->cursor];
            addch(current_char);
            attroff(COLOR_PAIR(PAIR_SELECT));
        }
        else if (ed->mode == MODE_VISUAL_EDIT_END && i >= start && i <= end) {
            render_line(ed, i, line_size);
            printw(STR_FMT, STR_ARG(ed->visual_buffer));

            attron(COLOR_PAIR(PAIR_SELECT));
            move(i - ed->scroll, ed->cursor + line_size + ed->lines.data[i].size);
            int current_char = ' ';
            if (ed->cursor < ed->visual_buffer.size)
                current_char = ed->visual_buffer.data[ed->cursor];
            addch(current_char);
            attroff(COLOR_PAIR(PAIR_SELECT));
        }
        else {
            render_line(ed, i, line_size);
        }
    }

    // cursor
    if (ed->mode == MODE_INSERT) {
        move(ed->line - ed->scroll, ed->cursor + line_size);
        attron(COLOR_PAIR(PAIR_SELECT));
        int current_char = ' ';
        if (ed->cursor < CURR_LINE.size)
            current_char = CURR_LINE.data[ed->cursor];
        addch(current_char);
        attroff(COLOR_PAIR(PAIR_SELECT));
    }
}

static inline void
merge_lines(editor_t *ed)
{
#define PREV_LINE ed->lines.data[ed->line-1]
    ed->cursor = PREV_LINE.size;
    for (size_t i = 0; i < CURR_LINE.size; ++i) {
        LIST_ADD(PREV_LINE, PREV_LINE.size, CURR_LINE.data[i]);
    }
    free(CURR_LINE.data);
    LIST_POP(ed->lines, ed->line);
    ed->line--;
#undef PREV_LINE
}

void
scroll_set(editor_t *ed)
{
    ed->scroll = (ed->line - (ed->win_h / 2));
    if (ed->scroll < 0) {
        ed->scroll = 0;
    }
}

void
scroll_up(editor_t *ed)
{
    if (ed->line - ed->scroll < 4 && ed->scroll > 0) {
        ed->scroll--;
    }
}

void
scroll_down(editor_t *ed)
{
    if (ed->line - ed->scroll + 4 > ed->win_h-1) {
        ed->scroll++;
    }
}

void
move_up(editor_t *ed)
{
    if (ed->line <= 0) return;
    ed->line--;
    scroll_up(ed);

    if (ed->mode == MODE_VISUAL)
        ed->visual.end = ed->line;

    if (ed->cursor >= CURR_LINE.size) {
        ed->cursor = CURR_LINE.size;
    }
}

void
move_down(editor_t *ed)
{
    if (ed->line+1 >= ed->lines.size) return;
    ed->line++;
    scroll_down(ed);

    if (ed->mode == MODE_VISUAL)
        ed->visual.end = ed->line;

    if (ed->cursor >= CURR_LINE.size) {
        ed->cursor = CURR_LINE.size;
    }
}

void
move_left(editor_t *ed)
{
    if (ed->cursor > 0) {
        ed->cursor--;
        return;
    }
    if (ed->cursor == 0 && ed->line > 0) {
        move_up(ed);
        ed->cursor = ed->lines.data[ed->line].size;
    }
}

void
move_right(editor_t *ed)
{
    if (ed->cursor < CURR_LINE.size) {
        ed->cursor++;
        return;
    }
    if (ed->cursor >= CURR_LINE.size && ed->line+1 < ed->lines.size) {
        move_down(ed);
        ed->cursor = 0;
    }
}

void
page_up(editor_t *ed)
{
    for (size_t i = 0; i < ed->win_h - 4; ++i) {
        move_up(ed);
    }
}

void
page_down(editor_t *ed)
{
    for (size_t i = 0; i < ed->win_h - 4; ++i) {
        move_down(ed);
    }
}

bool
search_line(editor_t *ed, char *str, int l)
{
    char line[ed->lines.data[l].size+1];
    memcpy(line, ed->lines.data[l].data, ed->lines.data[l].size);
    line[ed->lines.data[l].size] = '\0';

    if (str[0] == '^') {
        *str++;
        int f = (strncmp(str, line, strlen(str)) == 0);
        if (f) {
            ed->line = l;
            scroll_set(ed);
            // ed->cursor = 0;
            ed->cursor = strlen(str);
            // ed->cursor = ed->lines.data[l].size;
            return TRUE;
        }
        return FALSE;
    }

    char *f = strstr(line, str);
    if (f) {
        ed->line = l;
        scroll_set(ed);
        // ed->cursor = ed->lines.data[l].size;

        // TODO: this feels stupid. fix this. maybe.
        size_t sz = strlen(str);
        for (size_t i = 0; i + sz <= ed->lines.data[l].size; ++i) {
            if (strncmp(str, line+i, sz) == 0) {
                // ed->cursor = i;
                ed->cursor = i+sz;
                break;
            }
        }

        return TRUE;
    }
    return FALSE;
}

void
search_text(editor_t *ed)
{
    if (ed->dialog.size == 0 && find_buffer.size > 0) {
        buffer_pos = find_buffer.size-1;
        ed->dialog.size = 0;
        for (size_t i = 0; i < find_buffer.data[buffer_pos].size; ++i) {
            LIST_ADD(ed->dialog, ed->dialog.size, find_buffer.data[buffer_pos].data[i]);
        }
        ed->dialog_cursor = ed->dialog.size;
    }

    char str[ed->dialog.size+1];
    memcpy(str, ed->dialog.data, ed->dialog.size);
    str[ed->dialog.size] = '\0';
    
    for (size_t i = ed->line+1; i < ed->lines.size; ++i) {        
        if (search_line(ed, str, i))
            return;
    }
    // wrap search
    for (size_t i = 0; i < ed->line; ++i) {
        if (search_line(ed, str, i))
            return;
    }
    
    sprintf(status_message, "ERROR: could not find text '%s'", str);
    status_delay = STATUS_DELAY;
}

void
new_clip()
{
    if (clip.data) {
        LIST_FREE(clip);
    }
    clip = L_LINES;
}

void
copy_line(editor_t *ed)
{
    LIST_ADD(clip, clip.size, CURR_LINE);
}

void
delete_line(editor_t *ed)
{
    copy_line(ed);
    if (ed->lines.size > 1 && ed->line+1 < ed->lines.size) {
        // free(CURR_LINE.data);
        LIST_POP(ed->lines, ed->line);
    }
    else {
        CURR_LINE.size = 0;
    }

    ed->cursor = 0;
}

void
paste_line(editor_t *ed)
{
    for (size_t i = 0; i < clip.size; ++i) {
        size_t clip_size = clip.data[i].size;
        string_t s = {
            .data = malloc(clip_size * sizeof(char)),
            .size = clip_size,
            .alloc = clip_size,
        };
        memcpy(s.data, clip.data[i].data, clip.data[i].size);
        LIST_ADD(ed->lines, ed->line, s);
        move_down(ed);
    }
}

void
insert_tab(editor_t *ed)
{
    for (size_t i = 0; i < TAB_WIDTH; ++i) {
        LIST_ADD(CURR_LINE, ed->cursor, ' ');
        ed->cursor++;
    }
}

void
remove_tab(editor_t *ed)
{
    if (CURR_LINE.size < TAB_WIDTH) return;
    if (CURR_LINE.data[0] == '\t') {
        LIST_POP(CURR_LINE, 0);
        return;
    }

    for (size_t i = 0; i < TAB_WIDTH; ++i) {
        if (CURR_LINE.data[0] != ' ') break;
        LIST_POP(CURR_LINE, 0);
        if (ed->cursor > 0)
            ed->cursor--;
    }
}

void
move_home(editor_t *ed)
{
    if (ed->cursor == 0) {
        for (size_t i = 0; i < CURR_LINE.size; ++i) {
            if (CURR_LINE.data[i] != ' ') break;
            ed->cursor++;
        }
    }
    else {
        ed->cursor = 0;
    }
}

void
comment_line(editor_t *ed)
{
    size_t c = ed->cursor;
    ed->cursor = 0;
    move_home(ed);
    if (CURR_LINE.data[ed->cursor] == '/') {
        if (CURR_LINE.data[ed->cursor+1] == '/') {
            LIST_POP(CURR_LINE, ed->cursor);
            LIST_POP(CURR_LINE, ed->cursor);
            if (CURR_LINE.data[ed->cursor] == ' ') {
                LIST_POP(CURR_LINE, ed->cursor);
            }
        }
    }
    else {
        LIST_ADD(CURR_LINE, ed->cursor, ' ');
        LIST_ADD(CURR_LINE, ed->cursor, '/');
        LIST_ADD(CURR_LINE, ed->cursor, '/');
    }
    ed->cursor = c;
    if (ed->cursor >= CURR_LINE.size)
        ed->cursor = CURR_LINE.size;
}

void
newline(editor_t *ed)
{
    string_t s = L_STRING;
    if (ed->cursor < CURR_LINE.size) {
        memmove(s.data, CURR_LINE.data+ed->cursor, CURR_LINE.size-ed->cursor);
        s.size = CURR_LINE.size - ed->cursor;
        CURR_LINE.size = ed->cursor;
    }
    ed->line++;
    LIST_ADD(ed->lines, ed->line, s);

    ed->cursor = 0;
    int sz = ed->lines.data[ed->line-1].size;
    if (sz > 0) {
        while (ed->cursor < sz && ed->lines.data[ed->line-1].data[ed->cursor] == ' ') {
            LIST_ADD(CURR_LINE, 0, ' ');
            ed->cursor++;
        }
    }

    // if (ed->line + ed->scroll + 2 > ed->win_h) {
        // ed->scroll++;
    // }
    scroll_down(ed);
}

void
update_insert(editor_t *ed)
{
    int ch = getch();

    switch (ch) {
        case CTRL('q'): {
            ed->quit = TRUE;
        } break;
        case CTRL('s'): {
            if (ed->file.size != 0) {
                ed->dialog.size = 0;
                for (size_t i = 0; i < ed->file.size; ++i) {
                    LIST_ADD(ed->dialog, ed->dialog.size, ed->file.data[i]);
                }
            }
            ed->dialog_cursor = ed->dialog.size;
            buffer_pos = save_buffer.size-1;
            ed->mode = MODE_DIALOG_SAVE;
        } break;
        case CTRL('o'): {
            ed->dialog.size = 0;
            ed->dialog_cursor = 0;
            buffer_pos = save_buffer.size-1;
            ed->mode = MODE_DIALOG_OPEN;
        } break;
        case CTRL('x'): {
            new_clip();
            delete_line(ed);
        } break;
        case CTRL('c'): {
            new_clip();
            copy_line(ed);
        } break;
        case CTRL('p'): {
            paste_line(ed);
        } break;
        case CTRL('d'): {
            new_clip();
            copy_line(ed);
            paste_line(ed);
        } break;
        case CTRL('v'): {
            ed->visual = (visual_t) {
                .start = ed->line,
                .end = ed->line,
            };
            ed->mode = MODE_VISUAL;
        } break;
        case CTRL('g'): {
            ed->dialog.size = 0;
            ed->dialog_cursor = 0;
            ed->mode = MODE_DIALOG_GOTO;
        } break;
        case CTRL('f'): {
            ed->dialog.size = 0;
            ed->dialog_cursor = 0;
            buffer_pos = find_buffer.size-1;
            ed->mode = MODE_DIALOG_FIND;
        } break;
        case CTRL('n'): {
            search_text(ed);
        } break;
        case CTRL('z'): {
            scroll_set(ed);
        } break;
        case CTRL('k'): {
            ed->line++;
            ed->cursor = 0;
            merge_lines(ed);
        } break;
        case KEY_HOME: {
            move_home(ed);
        } break;
        case CTRL('l'): {
            comment_line(ed);
        } break;
        case KEY_END: {
            ed->cursor = CURR_LINE.size;
        } break;
        case KEY_PPAGE: {
            page_up(ed);
        } break;
        case KEY_NPAGE: {
            page_down(ed);
        } break;
        // TODO: refactor backspace/delete
        case 8: // shift + backspace
        case KEY_BACKSPACE: {
            if (ed->cursor == 0 && ed->line > 0) {
                if (CURR_LINE.size == 0) {
                    free(CURR_LINE.data);
                    LIST_POP(ed->lines, ed->line);
                    ed->line--;
                    ed->cursor = CURR_LINE.size;
                }
                else {
                    merge_lines(ed);
                }
                if (ed->line + ed->scroll + 2 > ed->win_h) {
                   ed->scroll--;
                }
            }
            else if (ed->cursor > 0) {
                ed->cursor--;
                LIST_POP(CURR_LINE, ed->cursor);
            }
        } break;
        case KEY_DC: {
            if (ed->cursor < CURR_LINE.size) {
                LIST_POP(CURR_LINE, ed->cursor);
            }
            else if (ed->line < ed->lines.size-1) {
                ed->line++;
                ed->cursor = 0;
                merge_lines(ed);
            }
        } break;
        case KEY_LEFT: {
            move_left(ed);
        } break;
        case KEY_RIGHT: {
            move_right(ed);
        } break;
        case KEY_UP: {
            move_up(ed);
        } break;
        case KEY_DOWN: {
            move_down(ed);
        } break;
        case '\t': {
            insert_tab(ed);
        } break;
        case KEY_BTAB: {
            remove_tab(ed);
        } break;
        case '\n': {
            newline(ed);
        } break;
        default: {
            LIST_ADD(CURR_LINE, ed->cursor, ch);
            ed->cursor++;
        } break;
    }
}

// TODO: refactor this shit
void
update_visual_edit_start(editor_t *ed)
{
    int ch = getch();

    int start = (ed->visual.start < ed->visual.end)?
                    ed->visual.start : ed->visual.end;
    int end = (ed->visual.start < ed->visual.end)?
                    ed->visual.end : ed->visual.start;

    switch (ch) {
        case CTRL('q'): {
            ed->mode = MODE_INSERT;
            ed->cursor = 0;
            // scroll_set(ed);
        } break;
        case CTRL('c'): {
            new_clip();
            LIST_ADD(clip, 0, ed->visual_buffer);
        } break;
        case CTRL('x'): {
            new_clip();
            LIST_ADD(clip, 0, ed->visual_buffer);
            ed->visual_buffer.size = 0;
            ed->cursor = 0;
            // ed->mode = MODE_INSERT;
        } break;
        case CTRL('p'): {
            if (clip.size != 0) {
                for (size_t i = 0; i < clip.data[0].size; ++i) {
                    LIST_ADD(ed->visual_buffer, ed->cursor, clip.data[0].data[i]);
                    ed->cursor++;
                }
            }
        } break;
        case CTRL('l'): {
            size_t l = ed->line, s = ed->scroll;
            ed->line = start;
            for (size_t i = start; i <= end; ++i) {
                comment_line(ed);
                move_down(ed);
            }
            ed->line = l;
            ed->scroll = s;
        } break;
        case KEY_HOME: {
            ed->cursor = 0;
        } break;
        case KEY_END: {
            ed->cursor = ed->visual_buffer.size;
        } break;
        case KEY_UP:
        case KEY_DOWN: break;
        case KEY_LEFT: {
            if (ed->cursor > 0)
                ed->cursor--;
        } break;
        case KEY_RIGHT: {
            if (ed->cursor < ed->visual_buffer.size)
                ed->cursor++;
        } break;
        case KEY_DC: {
            if (ed->cursor < ed->visual_buffer.size) {
                LIST_POP(ed->visual_buffer, ed->cursor);
            }
            else {
                for (size_t i = start; i <= end; ++i) {
                    ed->line = i;
                    if (CURR_LINE.size > 0) {
                        LIST_POP(CURR_LINE, 0);
                    }
                }
            }
        } break;
        case 8: // shift + backspace
        case KEY_BACKSPACE: {
            if (ed->cursor > 0) {
                ed->cursor--;
                LIST_POP(ed->visual_buffer, ed->cursor);
            }
        } break;
        case '\t': {
            for (size_t i = 0; i < TAB_WIDTH; ++i) {
                LIST_ADD(ed->visual_buffer, 0, ' ');
                ed->cursor++;
            }
        } break;
        case KEY_BTAB: {
            for (size_t i = 0; i < TAB_WIDTH; ++i) {
                if (ed->visual_buffer.size == 0 || ed->visual_buffer.data[0] != ' ')
                    break;
                LIST_POP(ed->visual_buffer, 0);
                ed->cursor--;
            }
        } break;
        case CTRL('v'):
        case '\n': {
            for (size_t i = start; i <= end; ++i) {
                ed->line = i;
                for (size_t s = 0; s < ed->visual_buffer.size; ++s) {
                    LIST_ADD(CURR_LINE, s, ed->visual_buffer.data[s]);
                }
            }
            scroll_set(ed);
            // ed->cursor = 0;
            ed->mode = MODE_INSERT;
        } break;
        default: {
            LIST_ADD(ed->visual_buffer, ed->cursor, ch);
            ed->cursor++;
        } break;
    }
}

void
update_visual_edit_end(editor_t *ed)
{
    int ch = getch();

    int start = (ed->visual.start < ed->visual.end)?
                    ed->visual.start : ed->visual.end;
    int end = (ed->visual.start < ed->visual.end)?
                    ed->visual.end : ed->visual.start;

    switch (ch) {
        case CTRL('q'): {
            ed->mode = MODE_INSERT;
            ed->cursor = 0;
            // scroll_set(ed);
        } break;
        case CTRL('c'): {
            new_clip();
            LIST_ADD(clip, 0, ed->visual_buffer);
        } break;
        case CTRL('x'): {
            new_clip();
            LIST_ADD(clip, 0, ed->visual_buffer);
            ed->visual_buffer.size = 0;
            ed->cursor = 0;
            // ed->mode = MODE_INSERT;
        } break;
        case CTRL('p'): {
            if (clip.size != 0) {
                for (size_t i = 0; i < clip.data[0].size; ++i) {
                    LIST_ADD(ed->visual_buffer, ed->cursor, clip.data[0].data[i]);
                    ed->cursor++;
                }
            }
        } break;
        case CTRL('l'): {
            size_t l = ed->line, s = ed->scroll;
            ed->line = start;
            for (size_t i = start; i <= end; ++i) {
                comment_line(ed);
                move_down(ed);
            }
            ed->line = l;
            ed->scroll = s;
        } break;
        case KEY_HOME: {
            ed->cursor = 0;
        } break;
        case KEY_END: {
            ed->cursor = ed->visual_buffer.size;
        } break;
        case KEY_UP:
        case KEY_DOWN: break;
        case KEY_LEFT: {
            if (ed->cursor > 0)
                ed->cursor--;
        } break;
        case KEY_RIGHT: {
            if (ed->cursor < ed->visual_buffer.size)
                ed->cursor++;
        } break;
        case KEY_DC: {
            if (ed->cursor < ed->visual_buffer.size) {
                LIST_POP(ed->visual_buffer, ed->cursor);
            }
        } break;
        case 8: // shift + backspace
        case KEY_BACKSPACE: {
            if (ed->cursor > 0) {
                ed->cursor--;
                LIST_POP(ed->visual_buffer, ed->cursor);
            }
            else {
                for (size_t i = start; i <= end; ++i) {
                    ed->line = i;
                    if (CURR_LINE.size > 0) {
                        LIST_POP(CURR_LINE, CURR_LINE.size);
                    }
                }
            }
        } break;
        case '\t': {
            for (size_t i = 0; i < TAB_WIDTH; ++i) {
                LIST_ADD(ed->visual_buffer, 0, ' ');
                ed->cursor++;
            }
        } break;
        case KEY_BTAB: {
            for (size_t i = 0; i < TAB_WIDTH; ++i) {
                if (ed->visual_buffer.size == 0 || ed->visual_buffer.data[0] != ' ')
                    break;
                LIST_POP(ed->visual_buffer, 0);
                ed->cursor--;
            }
        } break;
        case CTRL('v'):
        case '\n': {
            for (size_t i = start; i <= end; ++i) {
                ed->line = i;
                for (size_t s = 0; s < ed->visual_buffer.size; ++s) {
                    LIST_ADD(CURR_LINE, CURR_LINE.size, ed->visual_buffer.data[s]);
                }
            }
            scroll_set(ed);
            ed->cursor = CURR_LINE.size;
            ed->mode = MODE_INSERT;
        } break;
        default: {
            LIST_ADD(ed->visual_buffer, ed->cursor, ch);
            ed->cursor++;
        } break;
    }
}

void
update_visual(editor_t *ed)
{
    int ch = getch();

    int start = (ed->visual.start < ed->visual.end)?
                    ed->visual.start : ed->visual.end;
    int end = (ed->visual.start < ed->visual.end)?
                    ed->visual.end : ed->visual.start;

    switch (ch) {
        case CTRL('v'):
        case CTRL('q'): {
            ed->mode = MODE_INSERT;
        } break;
        case 'i':
        case 'I': {
            ed->cursor = 0;
            ed->visual_buffer.size = 0;
            ed->mode = MODE_VISUAL_EDIT_START;
        } break;
        case 'a':
        case 'A': {
            ed->cursor = 0;
            ed->visual_buffer.size = 0;
            ed->mode = MODE_VISUAL_EDIT_END;
        } break;
        case KEY_PPAGE: {
            page_up(ed);
        } break;
        case KEY_NPAGE: {
            page_down(ed);
        } break;
        case KEY_UP: {
            move_up(ed);
        } break;
        case KEY_DOWN: {
            move_down(ed);
        } break;
        case KEY_DC:
        case CTRL('x'):
        case 8: // shift + backspace
        case KEY_BACKSPACE: {
            new_clip();
            size_t l = ed->line, s = ed->scroll;
            ed->line = start;
            for (size_t i = start; i <= end; ++i) {
                delete_line(ed);
            }
            ed->line = l;
            ed->scroll = s;
            ed->visual = (visual_t) {0, 0};
            ed->mode = MODE_INSERT;
        } break;
        case CTRL('c'): {
            new_clip();
            size_t l = ed->line, s = ed->scroll;
            ed->line = start;
            for (size_t i = start; i <= end; ++i) {
                copy_line(ed);
                move_down(ed);
            }
            ed->line = l;
            ed->scroll = s;
            ed->visual = (visual_t) {0, 0};
            ed->mode = MODE_INSERT;
        } break;
        case CTRL('d'): {
            new_clip();
            ed->line = start;
            scroll_set(ed);
            for (size_t i = start; i <= end; ++i) {
                copy_line(ed);
                move_down(ed);
            }
            paste_line(ed);
            move_down(ed);
            ed->visual = (visual_t) {0, 0};
            ed->mode = MODE_INSERT;
        } break;
        case CTRL('l'): {
            size_t l = ed->line, s = ed->scroll;
            ed->line = start;
            for (size_t i = start; i <= end; ++i) {
                comment_line(ed);
                move_down(ed);
            }
            ed->line = l;
            ed->scroll = s;
        } break;
        case '\t': {
            for (size_t i = start; i <= end; ++i) {
                ed->line = i;
                ed->cursor = 0;
                insert_tab(ed);
            }
            ed->line = end;
        } break;
        case KEY_BTAB: {
            for (size_t i = start; i <= end; ++i) {
                ed->line = i;
                remove_tab(ed);
            }
            ed->cursor = 0;
            ed->line = end;
        } break;
        case '\n': {
            move_down(ed);
        } break;
        default: break;
    }
}

#undef CURR_LINE

bool
can_read_file(const char *path)
{
    return (access(path, R_OK) == 0);
}

string_t
read_file(const char *path)
{
    string_t text = L_STRING;

    if (!can_read_file(path))
        goto read_text_fail;

    FILE *file = fopen(path, "r");

    if (!file)
        goto read_text_fail;

    fseek(file, 0, SEEK_END);
    text.size = ftell(file);
    rewind(file);

    if (text.size == 0) {
        // goto read_text_fail;
        LIST_ADD(text, 0, '\n');
    }
    else {
        text.alloc = text.size;
        text.data = realloc(text.data, (text.size) * sizeof(char));
        fread(text.data, sizeof(char), text.size, file);
    }

    fclose(file);
    return text;

read_text_fail:
    if (file) fclose(file);
    sprintf(status_message, "ERROR: could not read file '%s'", path);
    status_delay = STATUS_DELAY;
    return text;
}

void
write_file(editor_t *ed)
{
    char path[ed->file.size+1];
    memmove(path, ed->file.data, ed->file.size);
    path[ed->file.size] = '\0';

    FILE *file = fopen(path, "w");
    if (!file)
        goto write_file_fail;

    for (size_t i = 0; i < ed->lines.size; ++i) {
        if (i > 0) fprintf(file, "\n");
        fprintf(file, STR_FMT, STR_ARG(ed->lines.data[i]));
    }

    fclose(file);
    return;

write_file_fail:
    if (file) fclose(file);
    sprintf(status_message, "ERROR: could not write file '%s'", path);
    status_delay = STATUS_DELAY;
}

lines_t
split_lines(string_t text)
{
    lines_t lines = L_LINES;

    string_t s = L_STRING;
    for (size_t i = 0; i < text.size; ++i) {
        if (text.data[i] == '\n') {
            LIST_ADD(lines, lines.size, s);
            s = L_STRING;
            continue;
        }
        LIST_ADD(s, s.size, text.data[i]);
    }

    LIST_ADD(lines, lines.size, s);
    return lines;
}

editor_t
init_editor(const char *path)
{
    editor_t ed = {0};
    ed.quit = FALSE;
    ed.cursor = ed.line = 0;
    ed.file = L_STRING;
    ed.mode = MODE_INSERT;
    ed.scroll = 0;
    ed.show_lines = TRUE;
    ed.visual = (visual_t) {0, 0};
    ed.visual_buffer = L_STRING;
    ed.dialog = L_STRING;
    ed.dialog_cursor = 0;

    if (!path) {
        ed.lines = L_LINES;
        string_t s = L_STRING;
        LIST_ADD(ed.lines, 0, s);
    }
    else {
        string_t s = read_file(path);
        if (s.size == 0) {
            LIST_ADD(ed.lines, 0, s);
        }
        else {
            ed.lines = split_lines(s);
            free(s.data);
        }

        ed.file.size = strlen(path);
        if (ed.file.size >= ed.file.alloc) {
            ed.file.alloc = ed.file.size;
            ed.file.data = realloc(ed.file.data, ed.file.alloc * sizeof(char));
        }
        memmove(ed.file.data, path, ed.file.size);
    }

    return ed;
}

void
free_editor(editor_t *ed)
{
    LIST_FREE(ed->lines);
    LIST_FREE(ed->visual_buffer);
}

int
main(int argc, const char *argv[])
{
    init_curses();
    const char *path = NULL;
    if (argc > 1)
        path = argv[1];

    save_buffer = L_LINES;
    find_buffer = L_LINES;
    new_clip();

    editor_t ed = init_editor(path);

    while (!ed.quit) {
        getmaxyx(stdscr, ed.win_h, ed.win_w);

        // TODO: move every mode to a different function
        clear();
        render_text(&ed);
        render_status(&ed);
        switch (ed.mode) {
            case MODE_INSERT: {
                update_insert(&ed);
            } break;
            case MODE_VISUAL: {
                update_visual(&ed);
            } break;
            case MODE_VISUAL_EDIT_START: {
                update_visual_edit_start(&ed);
            } break;
            case MODE_VISUAL_EDIT_END: {
                update_visual_edit_end(&ed);
            } break;
            case MODE_DIALOG_SAVE: {
                if (update_dialog(&ed)) {
                    ed.file.size = 0;
                    for (size_t i = 0; i < ed.dialog.size; ++i) {
                        LIST_ADD(ed.file, ed.file.size, ed.dialog.data[i]);
                    }
                    write_file(&ed);
                    ed.dialog.size = 0;
                }
            } break;
            case MODE_DIALOG_OPEN: {
                // TODO: don't do anything if it can't open a file
                if (update_dialog(&ed)) {
                    char f[ed.dialog.size+1];
                    memmove(f, ed.dialog.data, ed.dialog.size);
                    f[ed.dialog.size] = '\0';
                    if (can_read_file(f)) {
                        free_editor(&ed);
                        ed = init_editor(f);
                    }
                    else {
                        sprintf(status_message, "ERROR: file '%s' does not exist", f);
                        status_delay = STATUS_DELAY;
                    }
                    ed.dialog.size = 0;
                }
            } break;
            case MODE_DIALOG_GOTO: {
                if (update_dialog(&ed)) {
                    char l[20] = {0};
                    memcpy(l, ed.dialog.data, ed.dialog.size);
                    long line = strtol(l, &l+strlen(l), 10);
                    if (line <= 0) {
                        ed.line = 0;
                    }
                    else if (line > ed.lines.size) {
                        ed.line = ed.lines.size-1;
                    }
                    else {
                        ed.line = line-1;
                    }
                    scroll_set(&ed);
                    ed.cursor = 0;
                }
            } break;
            case MODE_DIALOG_FIND: {
                if (update_dialog(&ed)) {
                    search_text(&ed);
                }
            } break;
        }
    }

    if (clip.data)
        LIST_FREE(clip);
    LIST_FREE(save_buffer);
    LIST_FREE(find_buffer);
    free_editor(&ed);
    end_curses();
    return 0;
}
