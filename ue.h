#ifndef _UE_H
#define _UE_H

#define BUFSZ 512
struct range { int start, end; };
enum { ACT_INSERT = 1, ACT_DELETE, ACT_BACKSPACE };
enum { MODE_NORMAL, MODE_INSERT, MODE_SELECT, MODE_SEARCH, MODE_OPEN };
struct hist_action {
  int typ, sz, max, cur, off, line;
  char *text;
};

struct hist {
  int sz, max, cur, last;
  struct hist_action *acts;
};

struct buffer {
  int sz, max, cur, sel, off, line, num_lines, max_lines;
  char *text, *name;
  struct hist hist;
  struct range *lines;
};

struct key {
  const char *key;
  void (*action)(struct buffer *buf);
};

static void quit(void);
static void createbuf(char *name);
static void modenormal(struct buffer *buf);
static void modeinsert(struct buffer *buf);
static void modeselect(struct buffer *buf);
static void modesearch(struct buffer *buf);
static void modeopen(struct buffer *buf);
static void findnext(struct buffer *buf);
static void closebuf(struct buffer *buf);
static void writebuf(struct buffer *buf);
static void nextbuf(struct buffer *buf);
static void prevbuf(struct buffer *buf);
static void delete(struct buffer *buf);
static void backspace(struct buffer *buf);
static void indent(struct buffer *buf);
static void unindent(struct buffer *buf);
static void moveup(struct buffer *buf);
static void movedown(struct buffer *buf);
static void moveleft(struct buffer *buf);
static void moveright(struct buffer *buf);
static void movebol(struct buffer *buf);
static void moveeol(struct buffer *buf);
static void pageup(struct buffer *buf);
static void pagedown(struct buffer *buf);
static void undo(struct buffer *buf);
static void redo(struct buffer *buf);
static void yank(struct buffer *buf);
static void paste(struct buffer *buf);

#endif
