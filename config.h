#ifndef _CONFIG_H
#define _CONFIG_H

#include <ctype.h>
#include "ue.h"

#define TABSIZE 2

static int _isword(int c) {
  return isalnum(c) || c == '_';
}

#define C(S,D) (((D)>0?buf->cur+1<buf->sz : buf->cur>0) && S(buf->text[buf->cur+(D)]))
static void _pword(struct buffer *buf) {
  if(_isword(buf->text[buf->cur-1]))
    while(C(_isword,-1)) moveleft(buf);
  else while(C(!_isword,-1)) moveleft(buf);
}

static void _nword(struct buffer *buf) {
  if(_isword(buf->text[buf->cur+1]))
    while(C(_isword,1)) moveright(buf);
  else while(C(!_isword,1)) moveright(buf);
}

static void _insbol(struct buffer *buf) {
  movebol(buf);
  modeinsert(buf);
}

static void _inseol(struct buffer *buf) {
  moveeol(buf);
  modeinsert(buf);
}

static void _insafter(struct buffer *buf) {
  if (buf->cur < buf->lines[buf->line].end) moveright(buf);
  modeinsert(buf);
}

static void _ydelete(struct buffer *buf) {
  yank(buf);
  delete(buf);
}

static void _yreplace(struct buffer *buf) {
  _ydelete(buf);
  modeinsert(buf);
}

static void _delline(struct buffer *buf) {
  modeselect(buf);
  moveeol(buf);
  _ydelete(buf);
}

static void _selline(struct buffer *buf) {
  movebol(buf);
  modeselect(buf);
  moveeol(buf);
}

static void _left(struct buffer *buf) {
  if (buf->cur > buf->lines[buf->line].start) moveleft(buf);
}

static void _right(struct buffer *buf) {
  if (buf->cur < buf->lines[buf->line].end) moveright(buf);
}

#define KEY_DEFAULTS \
  { "KEY_LEFT",      _left      }, \
  { "KEY_RIGHT",     _right     }, \
  { "KEY_UP",        moveup     }, \
  { "KEY_DOWN",      movedown   }, \
  { "KEY_HOME",      movebol    }, \
  { "KEY_END",       moveeol    }, \
  { "KEY_PPAGE",     pageup     }, \
  { "KEY_NPAGE",     pagedown   }

#define KEY_EXTRAS \
  { "x",             _ydelete   }, \
  { "d",             _ydelete   }, \
  { "s",             _yreplace  }, \
  { "c",             _yreplace  }, \
  { "y",             yank       }, \
  { "p",             paste      }, \
  { "b",             _pword     }, \
  { "e",             _nword     }, \
  { "h",             _left      }, \
  { "l",             _right     }, \
  { "k",             moveup     }, \
  { "j",             movedown   }

static struct key keys_normal[] = {
  { "q",             closebuf   },
  { "w",             writebuf   },
  { "o",             modeopen   },
  { "i",             modeinsert },
  { "a",             _insafter  },
  { "I",             _insbol    },
  { "A",             _inseol    },
  { "v",             modeselect },
  { "/",             modesearch },
  { "n",             findnext   },
  { "g",             modegoto   },
  { "V",             _selline   },
  { "D",             _delline   },
  { "^I",            indent     },
  { "KEY_BTAB",      unindent   },
  { "u",             undo       },
  { "r",             redo       },
  { "t",             nextbuf    },
  { "T",             prevbuf    },
  KEY_DEFAULTS, KEY_EXTRAS,
  0,
};

static struct key keys_insert[] = {
  { "^[",            modenormal },
  { "^I",            indent     },
  { "KEY_BTAB",      unindent   },
  { "^V",            paste      },
  { "KEY_DC",        delete     },
  { "KEY_BACKSPACE", backspace  },
  { "^H",            backspace  },
  KEY_DEFAULTS,
  0
};

static struct key keys_select[] = {
  { "^[",            modenormal },
  { "v",             modenormal },
  { "i",             modeinsert },
  { "a",             modeinsert },
  { "KEY_DC",        delete     },
  { "KEY_BACKSPACE", backspace  },
  { "^H",            backspace  },
  KEY_DEFAULTS, KEY_EXTRAS,
  0
};

#endif
