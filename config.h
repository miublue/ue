#ifndef _CONFIG_H
#define _CONFIG_H

#include <ctype.h>
#include "ue.h"

#define TABSIZE 2

static int _isword(int c) {
  return isalnum(c) || c == '_';
}

#define C(S,D) (((D)>0?buf->cur+1<buf->sz : buf->cur>0) && S(buf->text[buf->cur+(D)]))
static void prevword(struct buffer *buf, int _) {
  if(_isword(buf->text[buf->cur-1]))
    while(C(_isword,-1)) moveleft(buf, 0);
  else while(C(!_isword,-1)) moveleft(buf, 0);
}

static void nextword(struct buffer *buf, int _) {
  if(_isword(buf->text[buf->cur+1]))
    while(C(_isword,1)) moveright(buf, 0);
  else while(C(!_isword,1)) moveright(buf, 0);
}

static void insertbol(struct buffer *buf, int _) {
  movebol(buf, 0);
  changemode(buf, MODE_INSERT);
}

static void inserteol(struct buffer *buf, int _) {
  moveeol(buf, 0);
  changemode(buf, MODE_INSERT);
}

static void insertafter(struct buffer *buf, int _) {
  if (buf->cur < buf->lines[buf->line].end) moveright(buf, 0);
  changemode(buf, MODE_INSERT);
}

static void yankdelete(struct buffer *buf, int _) {
  yank(buf, 0);
  delete(buf, 0);
}

static void yankreplace(struct buffer *buf, int _) {
  yankdelete(buf, 0);
  changemode(buf, MODE_INSERT);
}

static void selectline(struct buffer *buf, int _) {
  movebol(buf, 0);
  changemode(buf, MODE_SELECT);
  moveeol(buf, 0);
}

static void deleteline(struct buffer *buf, int _) {
  selectline(buf, 0);
  yankdelete(buf, 0);
}

static void vimleft(struct buffer *buf, int _) {
  if (buf->cur > buf->lines[buf->line].start) moveleft(buf, 0);
}

static void vimright(struct buffer *buf, int _) {
  if (buf->cur < buf->lines[buf->line].end) moveright(buf, 0);
}

#define KEY_DEFAULTS \
  { "KEY_LEFT",      vimleft,     0 }, \
  { "KEY_RIGHT",     vimright,    0 }, \
  { "KEY_UP",        moveup,      0 }, \
  { "KEY_DOWN",      movedown,    0 }, \
  { "KEY_HOME",      movebol,     0 }, \
  { "KEY_END",       moveeol,     0 }, \
  { "KEY_PPAGE",     pageup,      0 }, \
  { "KEY_NPAGE",     pagedown,    0 }

#define KEY_EXTRAS \
  { "x",             yankdelete,  0 }, \
  { "d",             yankdelete,  0 }, \
  { "s",             yankreplace, 0 }, \
  { "c",             yankreplace, 0 }, \
  { "y",             yank,        0 }, \
  { "p",             paste,       0 }, \
  { "b",             prevword,    0 }, \
  { "e",             nextword,    0 }, \
  { "h",             vimleft,     0 }, \
  { "l",             vimright,    0 }, \
  { "k",             moveup,      0 }, \
  { "j",             movedown,    0 }

static struct key keys_normal[] = {
  { "q",             closebuffer,   0 },
  { "w",             writebuffer,   0 },
  { "o",             changemode,    MODE_OPEN },
  { "i",             changemode,    MODE_INSERT },
  { "a",             insertafter,   0 },
  { "I",             insertbol,     0 },
  { "A",             inserteol,     0 },
  { "v",             changemode,    MODE_SELECT },
  { "/",             changemode,    MODE_SEARCH },
  { "n",             findnext,      1 },
  { "N",             findnext,     -1 },
  { "g",             changemode,    MODE_GOTO },
  { "V",             selectline,    0 },
  { "D",             deleteline,    0 },
  { "^I",            indent,        1 },
  { "KEY_BTAB",      indent,       -1 },
  { "u",             undo,          0 },
  { "r",             redo,          0 },
  { "t",             changebuffer,  1 },
  { "T",             changebuffer, -1 },
  KEY_DEFAULTS, KEY_EXTRAS,
  { 0 },
};

static struct key keys_insert[] = {
  { "^[",            changemode, MODE_NORMAL },
  { "^I",            indent,     1 },
  { "KEY_BTAB",      indent,    -1 },
  { "^V",            paste,      0 },
  { "KEY_DC",        delete,     0 },
  { "KEY_BACKSPACE", delete,    -1 },
  { "^H",            delete,    -1 },
  KEY_DEFAULTS,
  { 0 },
};

static struct key keys_select[] = {
  { "^[",            changemode, MODE_NORMAL },
  { "v",             changemode, MODE_NORMAL },
  { "i",             changemode, MODE_INSERT },
  { "a",             changemode, MODE_INSERT },
  { "KEY_DC",        delete,     0 },
  { "KEY_BACKSPACE", delete,    -1 },
  { "^H",            delete,    -1 },
  KEY_DEFAULTS, KEY_EXTRAS,
  { 0 },
};

#endif
