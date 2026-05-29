#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#define FINPUTBOX_IMPL
#include "finputbox.h"
#include "ue.h"
#include "config.h"

/* TODO: commands, goto, backwards search, replace */
static struct {
  int mode, cur, sz, max, max_x, max_y;
  struct finputbox inp;
  struct buffer *buf;
} ue;

static void _insert(struct buffer *buf, char *text, int sz);
static void _delete(struct buffer *buf, int sz);

static int _insaction(struct buffer *buf, char *text, int sz) {
  struct hist_action *act = &buf->hist.acts[buf->hist.cur-1];
  const int ins = act->typ == ACT_INSERT && act->cur+act->sz == buf->cur;
  const int del = act->typ == ACT_DELETE && act->cur == buf->cur;
  const int bak = act->typ == ACT_BACKSPACE && act->cur-1 == buf->cur;
  if (ins || del || bak) {
    if (bak) act->cur = buf->cur, act->line = buf->line, act->off = buf->off;
    if (act->sz+sz >= act->max) act->text = realloc(act->text, act->max+=sz+BUFSZ);
    memmove(act->text+act->sz, text, sz);
    act->sz += sz;
    return 1;
  }
  return 0;
}

static void _doaction(struct buffer *buf, int typ, char *text, int sz) {
  if (buf->hist.sz >= buf->hist.max)
    buf->hist.acts=realloc(buf->hist.acts,(buf->hist.max*=1.5)*sizeof(struct hist_action));
  struct hist_action act;
  int i;
  if (buf->hist.cur && buf->hist.sz && buf->hist.acts[buf->hist.cur-1].typ == typ)
    if (_insaction(buf, text, sz)) return;
  act.text = malloc(act.max=sz+BUFSZ); memmove(act.text, text, act.sz=sz);
  act.cur = buf->cur, act.off = buf->off, act.line = buf->line, act.typ = typ;
  if (buf->hist.sz)
    for (i = buf->hist.cur; i < buf->hist.sz; ++i) free(buf->hist.acts[i].text);
  buf->hist.acts[buf->hist.cur++] = act, buf->hist.sz = buf->hist.cur;
}

static void _undo_insert(struct buffer *buf, struct hist_action *act) {
  buf->cur = act->cur, buf->off = act->off, buf->line = act->line;
  _delete(buf, act->sz);
}

static void _undo_delete(struct buffer *buf, struct hist_action *act) {
  buf->cur = act->cur, buf->off = act->off, buf->line = act->line;
  _insert(buf, act->text, act->sz);
}

static void _undo_backspace(struct buffer *buf, struct hist_action *act) {
  buf->cur = act->cur, buf->off = act->off, buf->line = act->line;
  int i; for (i = act->sz; i > 0; --i) _insert(buf, act->text+i-1, 1);
}

static void undo(struct buffer *buf) {
  if (!buf->hist.cur || !buf->hist.sz) return;
  struct hist_action act = buf->hist.acts[--buf->hist.cur];
  switch (act.typ) {
  case ACT_INSERT:    _undo_insert(buf, &act); break;
  case ACT_DELETE:    _undo_delete(buf, &act); break;
  case ACT_BACKSPACE: _undo_backspace(buf, &act); break;
  }
}

static void redo(struct buffer *buf) {
  if (buf->hist.cur == buf->hist.sz || !buf->hist.sz) return;
  struct hist_action act = buf->hist.acts[buf->hist.cur++];
  switch (act.typ) {
  case ACT_INSERT:    _undo_delete(buf, &act); break;
  case ACT_DELETE:    _undo_insert(buf, &act); break;
  case ACT_BACKSPACE: _undo_insert(buf, &act); break;
  }
}

static void _calc_newline(struct buffer *buf, struct range ln) {
  if (buf->num_lines >= buf->max_lines)
    buf->lines = realloc(buf->lines, (buf->max_lines*=1.5)*sizeof(struct range));
  buf->lines[buf->num_lines++] = ln;
}

static void _calc_numlines(struct buffer *buf) {
  if (!buf->max_lines) buf->lines = malloc((buf->max_lines = BUFSZ)*sizeof(struct range));
  if (!buf->sz) { _calc_newline(buf, (struct range) {.start=0, .end=0}); return; }
  int c, s = buf->num_lines = 0;
  for (c = 0; c < buf->sz; ++c) {
    if (buf->text[c]=='\n') {
      _calc_newline(buf, (struct range) {.start=s, .end=c});
      s = c+1;
    }
  }
  if (c != s) _calc_newline(buf, (struct range) {.start=s, .end=c});
}

static void createbuf(char *name) {
  struct buffer buf = {0};
  const int name_sz = strlen(name);
  ue.mode = MODE_NORMAL, buf.name = malloc(name_sz+1);
  strcpy(buf.name, name);
  buf.text = malloc(buf.max = BUFSZ);
  buf.hist.sz = buf.hist.cur = buf.hist.last = 0;
  buf.hist.acts = malloc((buf.hist.max=BUFSZ)*sizeof(struct hist_action));
  FILE *fp = fopen(buf.name, "r");
  if (!fp) goto e;
  fseek(fp, 0, SEEK_END);
  buf.sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  buf.text = realloc(buf.text, buf.max += buf.sz);
  fread(buf.text, buf.sz, 1, fp);
  fclose(fp);
e:
  if (!buf.sz) _insert(&buf, "\n", 1);
  _calc_numlines(&buf);
  finputbox_reset(&ue.inp);
  buf.cur = buf.sel = buf.off = buf.line = 0;
  if (ue.sz >= ue.max) ue.buf = realloc(ue.buf, (ue.max*=1.5)*sizeof(struct buffer));
  ue.buf[ue.sz++] = buf, ue.cur = ue.sz-1;
}

static void closebuf(struct buffer *buf) {
  if (buf->hist.last != buf->hist.cur) {
    mvprintw(ue.max_y-1, 0, "buffer '%s' has unsaved changes, exit anyway (y/n)?", buf->name);
    if (!strchr("Yy", getch())) return;
  }
  int i, b;
  for (i = 0; i < buf->hist.sz; ++i) free(buf->hist.acts[i].text);
  free(buf->hist.acts);
  free(buf->name);
  free(buf->text);
  for (b = 0; b < ue.sz; ++b) if (&ue.buf[b] == buf) break;
  for (i = b; i < ue.sz-1; ++i) ue.buf[i] = ue.buf[i+1];
  if (ue.cur >= --ue.sz) ue.cur = ue.sz-1;
  if (!ue.sz) quit();
}

static void writebuf(struct buffer *buf) {
  FILE *fp = fopen(buf->name, "w+");
  if (!fp) quit();
  fwrite(buf->text, buf->sz, 1, fp);
  fclose(fp);
  buf->hist.last = buf->hist.cur;
}

static void nextbuf(struct buffer *buf) { if (++ue.cur >= ue.sz) ue.cur = 0; }
static void prevbuf(struct buffer *buf) { if (--ue.cur < 0) ue.cur = ue.sz-1; }
static void modenormal(struct buffer *buf) { ue.mode = MODE_NORMAL; }
static void modeinsert(struct buffer *buf) { ue.mode = MODE_INSERT; }
static void modeselect(struct buffer *buf) { ue.mode = MODE_SELECT, buf->sel = buf->cur; }
static void modesearch(struct buffer *buf) { ue.mode = MODE_SEARCH; finputbox_reset(&ue.inp); }
static void modeopen(struct buffer *buf) { ue.mode = MODE_OPEN; finputbox_reset(&ue.inp); }

static void findnext(struct buffer *buf) {
  if (!ue.inp.sz) return;
  char *a = strstr(buf->text+buf->cur+1, ue.inp.buf), *b = strstr(buf->text, ue.inp.buf);
  char *match = a? a : b;
  if (!match) return;
  if (b) buf->cur = buf->off = buf->line = 0;
  while (buf->text+buf->cur != match) moveright(buf);
  buf->sel = buf->cur;
}

static void quit(void) {
  while (ue.sz) closebuf(0);
  free(ue.buf);
  curs_set(1);
  endwin();
  exit(0);
}

static struct range _getsel(struct buffer *buf) {
  return (struct range) {
    .start = buf->cur < buf->sel? buf->cur : buf->sel,
    .end = (buf->cur > buf->sel? buf->cur : buf->sel) + 1,
  };
}

static void _gotoselstart(struct buffer *buf) {
  while (buf->cur > buf->sel) moveleft(buf);
}

static void _insert(struct buffer *buf, char *text, int sz) {
  if (buf->sz+sz >= buf->max) buf->text = realloc(buf->text, (buf->max+=sz+BUFSZ));
  memmove(buf->text+buf->cur+sz, buf->text+buf->cur, buf->sz-buf->cur);
  memmove(buf->text+buf->cur, text, sz);
  buf->sz += sz;
  _calc_numlines(buf);
  int i; for (i = 0; i < sz; ++i) moveright(buf);
}

static void _delete(struct buffer *buf, int sz) {
  if (buf->sz == 0 || buf->cur+sz >= buf->sz) return;
  memmove(buf->text+buf->cur, buf->text+buf->cur+sz, buf->sz-buf->cur);
  buf->sz -= sz;
  _calc_numlines(buf);
}

static void insert(struct buffer *buf, char *text, int sz) {
  _doaction(buf, ACT_INSERT, text, sz);
  _insert(buf, text, sz);
}

static void delete(struct buffer *buf) {
  struct range sel = _getsel(buf);
  if (buf->sel != buf->cur) _gotoselstart(buf);
  _doaction(buf, ACT_DELETE, buf->text+buf->cur, sel.end-sel.start);
  _delete(buf, sel.end-sel.start);
  buf->sel = buf->cur;
  if (ue.mode == MODE_SELECT) modenormal(buf);
}

static void backspace(struct buffer *buf) {
  if (buf->sel != buf->cur) { delete(buf); return; }
  if (buf->cur == 0) return;
  moveleft(buf);
  _doaction(buf, ACT_BACKSPACE, buf->text+buf->cur, 1);
  _delete(buf, 1);
}

static void _fix_scroll(struct buffer *buf) {
  if (buf->line-buf->off < 0 && buf->off > 0) --buf->off;
  else if (buf->line-buf->off+1 >= ue.max_y) ++buf->off;
}

static void moveup(struct buffer *buf) {
  if (buf->line <= 0) { buf->line=0; return; }
  --buf->line;
  buf->cur -= buf->lines[buf->line].end-buf->lines[buf->line].start+1;
  if (buf->cur > buf->lines[buf->line].end) buf->cur = buf->lines[buf->line].end;
  _fix_scroll(buf);
}

static void movedown(struct buffer *buf) {
  if (buf->line+1 >= buf->num_lines) { buf->line=buf->num_lines-1; return; }
  buf->cur += buf->lines[buf->line].end-buf->lines[buf->line].start+1;
  ++buf->line;
  if (buf->cur > buf->lines[buf->line].end) buf->cur = buf->lines[buf->line].end;
  _fix_scroll(buf);
}

static void moveleft(struct buffer *buf) {
  if (buf->cur <= 0) { buf->cur=0; return; }
  if (--buf->cur < buf->lines[buf->line].start && buf->line > 0) --buf->line;
  _fix_scroll(buf);
}

static void moveright(struct buffer *buf) {
  if (buf->cur+1 >= buf->sz) { buf->cur=buf->sz-1; return; }
  if (++buf->cur > buf->lines[buf->line].end && buf->line < buf->num_lines) ++buf->line;
  _fix_scroll(buf);
}

static void movebol(struct buffer *buf) {
  buf->cur = buf->lines[buf->line].start;
}

static void moveeol(struct buffer *buf) {
  buf->cur = buf->lines[buf->line].end;
}

static void pageup(struct buffer *buf) {
  int i; for (i = 0; i < ue.max_y-2; ++i) moveup(buf);
}

static void pagedown(struct buffer *buf) {
  int i; for (i = 0; i < ue.max_y-2; ++i) movedown(buf);
}

static void indent(struct buffer *buf) {
  int c = buf->cur;
  buf->cur = buf->sel = buf->lines[buf->line].start;
  char tab[TABSIZE]; memset(tab, ' ', TABSIZE);
  insert(buf, tab, TABSIZE);
  buf->cur = c + TABSIZE;
}

static void unindent(struct buffer *buf) {
  int i, c = buf->cur;
  buf->cur = buf->sel = buf->lines[buf->line].start;
  for (i = 0; i < TABSIZE && strchr("\t ", buf->text[buf->cur]); ++i) delete(buf);
  buf->cur = (c-i) < buf->cur? buf->cur : c-i;
}

static void yank(struct buffer *buf) {
  struct range sel = _getsel(buf);
  FILE *fp = fopen("/tmp/uesel", "w+");
  if (!fp) return;
  fwrite(buf->text+sel.start, sel.end-sel.start, 1, fp);
  fclose(fp);
#ifdef USE_X11
  system("cat /tmp/uesel | xsel -b 2> /dev/null");
#endif
  modenormal(buf);
}

static void paste(struct buffer *buf) {
  if (buf->sel != buf->cur) delete(buf);
#ifdef USE_X11
  if (system("xsel -b -o > /tmp/uesel 2> /dev/null") != 0) return;
#endif
  int cur = ue.cur;
  createbuf("/tmp/uesel");
  insert(buf, ue.buf[ue.cur].text, ue.buf[ue.cur].sz);
  closebuf(&ue.buf[ue.cur]);
  ue.cur = cur;
}

static int _update_input(struct buffer *buf, int c) {
  int res = FINPUTBOX_STOP;
  if (c == 27) goto r;
  res = finputbox_update_char(&ue.inp, c);
r:if (res != FINPUTBOX_OK) modenormal(buf);
  return res;
}

static void update(struct buffer *buf) {
  int i, c = getch();
  const char *k = keyname(c);
  struct key *keys;
  switch (ue.mode) {
  case MODE_SEARCH:
    if (_update_input(buf, c) == FINPUTBOX_DONE) findnext(buf);
    return;
  case MODE_OPEN:
    if (_update_input(buf, c) == FINPUTBOX_DONE) createbuf(ue.inp.buf);
    return;
  case MODE_INSERT: keys = keys_insert; break;
  case MODE_SELECT: keys = keys_select; break;
  default: keys = keys_normal; break;
  }
  for (i = 0; keys[i].key; ++i)
    if (!strcmp(k, keys[i].key)) { keys[i].action(buf); goto e; }
  if (ue.mode == MODE_INSERT && (isprint(c) || c == '\n')) insert(buf, (char*)&c, 1);
e:
  if (ue.mode != MODE_SELECT) buf->sel = buf->cur;
}

#define SEL(N) ((N) >= sel.start && (N) < sel.end)
#define CAP(N) ((N)+1 >= ue.max_x? ue.max_x : (N))
static void _drawline(struct buffer *buf, int lineno, int off) {
  struct range ln = buf->lines[lineno], sel = _getsel(buf);
  int i, attr = 0;
  if (off > ln.end-ln.start) return;
  for (i = ln.start+off; i <= ln.end; ++i) {
    if (i-ln.start-off > ue.max_x) return;
    attr = (ue.mode == MODE_SELECT && SEL(i))? A_REVERSE : 0;
    attron(attr);
    mvprintw(lineno-buf->off, i-ln.start-off, "%c", isprint(buf->text[i])? buf->text[i] : ' ');
    attroff(attr);
  }
}

static void draw(struct buffer *buf) {
  static const char *_mode[] = { "NOR", "INS", "SEL", "FND", "OPN" };
  erase();
  int i, off = 0;
  curs_set(0);
  if (buf->cur-buf->lines[buf->line].start+4 >= ue.max_x)
    off = buf->cur-buf->lines[buf->line].start+4-ue.max_x;
  for (i = buf->off; i < buf->num_lines; ++i)
    if (i-buf->off+1 >= ue.max_y) break; else _drawline(buf, i, off);
  if (ue.mode < MODE_SEARCH) {
    mvprintw(ue.max_y-1, 0, "%s (%d:%d) %d:%d:%d %s%s",
      _mode[ue.mode], ue.cur+1, ue.sz,
      buf->cur-buf->lines[buf->line].start, buf->line+1, buf->num_lines,
      buf->hist.last!=buf->hist.cur?"*":"", buf->name);
  } else {
    mvprintw(ue.max_y-1, 0, "%s ", _mode[ue.mode]);
    finputbox_draw(&ue.inp, strlen(_mode[ue.mode])+1);
  }
  if (buf->cur == buf->sel) {
    curs_set(1);
    move(buf->line-buf->off, buf->cur-buf->lines[buf->line].start-off);
  }
}
#undef SEL
#undef CAP

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <file> [file1 ...]\n", argv[0]);
    return 1;
  }
  ue.buf = malloc((ue.max=BUFSZ)*sizeof(struct buffer));
  initscr(); noecho(); raw(); keypad(stdscr, TRUE);
  set_escdelay(20);
  define_key("\033[1~", KEY_HOME);
  define_key("\033[4~", KEY_END);
  int i; for (i = 1; i < argc; ++i) createbuf(argv[i]);
  for (;;) {
    getmaxyx(stdscr, ue.max_y, ue.max_x);
    draw(&ue.buf[ue.cur]);
    update(&ue.buf[ue.cur]);
  }
  quit();
  return 0;
}
