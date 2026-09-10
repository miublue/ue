#ifndef _UE_H
#define _UE_H

#define BUFSZ 512
struct range { int start, end; };
enum { ACT_INSERT = 1, ACT_DELETE, ACT_BACKSPACE };
enum { MODE_NORMAL, MODE_INSERT, MODE_SELECT, MODE_SEARCH, MODE_GOTO, MODE_OPEN };
enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_BOL, DIR_EOL, DIR_PAGEUP, DIR_PAGEDOWN };
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
  void (*action)(struct buffer *buf, int arg);
  int arg;
};

/* these are the functions that may be mapped to keys in config.h */
void changemode(struct buffer *buf, int mode);
void findnext(struct buffer *buf, int dir);
void changebuffer(struct buffer *buf, int dir);
void writebuffer(struct buffer *buf, int _);
void closebuffer(struct buffer *buf, int _);
void delete(struct buffer *buf, int dir);
void indent(struct buffer *buf, int amount);
void movecursor(struct buffer *buf, int dir);
void undo(struct buffer *buf, int _);
void redo(struct buffer *buf, int _);
void yank(struct buffer *buf, int _);
void paste(struct buffer *buf, int _);

#endif
