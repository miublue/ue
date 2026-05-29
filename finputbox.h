#ifndef _FINPUTBOX_H
#define _FINPUTBOX_H

#ifndef FINPUTBOX_BUFFER_SIZE
#define FINPUTBOX_BUFFER_SIZE 255
#endif

enum { FINPUTBOX_OK, FINPUTBOX_DONE, FINPUTBOX_STOP };

struct finputbox {
  int sz, cur;
  char buf[FINPUTBOX_BUFFER_SIZE];
};

void finputbox_reset(struct finputbox *fib);
int finputbox_update(struct finputbox *fib);
int finputbox_update_char(struct finputbox *fib, int c);
void finputbox_draw(struct finputbox *fib, int x);

#ifdef FINPUTBOX_IMPL

#include <ncurses.h>
#include <string.h>
#include <ctype.h>

void finputbox_reset(struct finputbox *fib) {
  fib->buf[fib->sz = fib->cur = 0] = 0;
}

static void _addchar(struct finputbox *fib, char c) {
  memmove(fib->buf+fib->cur+1, fib->buf+fib->cur, fib->sz-fib->cur);
  fib->buf[fib->cur++] = c, fib->buf[++fib->sz] = 0;
}

static void _delchar(struct finputbox *fib) {
  if (!fib->sz || fib->cur >= fib->sz) return;
  memmove(fib->buf+fib->cur, fib->buf+fib->cur+1, fib->sz-fib->cur);
  fib->buf[--fib->sz] = 0;
}

#define FCTRL(K) ((K) & 0x1f)
int finputbox_update(struct finputbox *fib) {
  return finputbox_update_char(fib, getch());
}

int finputbox_update_char(struct finputbox *fib, int c) {
  switch (c) {
  case FCTRL('q'): case FCTRL('c'): return FINPUTBOX_STOP;
  case '\n': return FINPUTBOX_DONE;
  case KEY_LEFT: if (--fib->cur < 0) fib->cur = 0; break;
  case KEY_RIGHT: if (++fib->cur > fib->sz) fib->cur = fib->sz; break;
  case KEY_UP: case KEY_HOME: fib->cur = 0; break;
  case KEY_DOWN: case KEY_END: fib->cur = fib->sz; break;
  case KEY_BACKSPACE: case 127:
    if (--fib->cur < 0) { fib->cur = 0; break; }
    /* FALLTHROUGH */
  case KEY_DC: _delchar(fib); break;
  default: if (isprint(c)) _addchar(fib, c); break;
  }
  return FINPUTBOX_OK;
}
#undef FCTRL

void finputbox_draw(struct finputbox *fib, int x) {
  int max_x, max_y;
  getmaxyx(stdscr, max_y, max_x);
  mvprintw(max_y-1, x, "%.*s", fib->sz >= x+max_x? max_x-x-1 : fib->sz, fib->buf);
  attron(A_REVERSE);
  mvprintw(max_y-1, x+fib->cur, "%c", isprint(fib->buf[fib->cur])? fib->buf[fib->cur] : ' ');
  attroff(A_REVERSE);
}

#endif

#endif
