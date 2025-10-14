#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NTYPES 8
#define BUF_SIZE 0x100
#define OTHER(x) (~x & 1u)
#define NELEMS(a) (sizeof(a) / sizeof(a[0]))
#define LOWER(c) (c | 32)
#define IS_OCT_DIGIT(c) ((c | 0x07) == '7')

typedef enum {
  FALSE,
  TRUE
} boolean;

enum ParseResult {
  PARSE_SUCCESS,
  PARSE_END,
  PARSE_INCOMPLETE
};

struct TrieNode {
  struct TrieNode* child;
  struct TrieNode* next;
  enum ParseResult flag;
  char c;
};

void insert(struct TrieNode* trie, const char* s) {

  struct TrieNode** pp = &trie->child;
  struct TrieNode* p = NULL;

  for (int i = 0; s[i]; i++) {
    for (p = *pp; p != NULL; p = p->next) {
      if (p->c == s[i]) {
        pp = &p->child;
        break;
      }
    }

    if (p == NULL) {
      p = (struct TrieNode*)malloc(sizeof(struct TrieNode));
      p->child = NULL;
      p->next = *pp;
      p->flag = PARSE_INCOMPLETE;
      p->c = s[i];
      *pp = p;
      pp = &p->child;
    }
  }

  p->flag = PARSE_SUCCESS;
}

enum ParseResult keyword_parser(char c, boolean rst) {

  static boolean first_call = TRUE;
  static struct TrieNode* trie;

  if (first_call) {
    trie = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    trie->child = NULL;
    trie->next = NULL;
    trie->flag = PARSE_INCOMPLETE;
    trie->c = '\0';

    const char* keywords[32] = {
      "auto", "double", "int", "struct", "break", "else", "static", "long",
      "switch", "case", "enum", "register", "typedef", "char", "extern", "return",
      "union", "const", "float", "short", "unsigned", "continue", "for", "signed",
      "void", "default", "goto", "sizeof", "volatile", "do", "if", "while"
    };

    for (int i = 0; i < NELEMS(keywords); i++) {
      insert(trie, keywords[i]);
    }

    first_call = FALSE;
  }

  static struct TrieNode* now = NULL;

  if (rst) {
    now = trie;
  }
  else if (now == NULL) {
    return PARSE_END;
  }

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? PARSE_END : now->flag;
}

enum ParseResult identifier_parser(char c, boolean rst) {

  static enum ParseResult state;

  if (rst) {
    state = PARSE_INCOMPLETE;
  }

  switch (state) {

    case PARSE_INCOMPLETE:
      if (isalpha(c) || c == '_') {
        state = PARSE_SUCCESS;
      }
      else {
        state = PARSE_END;
      }
      break;

    case PARSE_SUCCESS:
      if (isalnum(c) || c == '_') {
        state = PARSE_SUCCESS;
      }
      else {
        state = PARSE_END;
      }
      break;
  }

  return state;
}

enum ParseResult operator_parser(char c, boolean rst) {

  static boolean first_call = TRUE;
  static struct TrieNode* trie;

  if (first_call) {
    trie = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    trie->child = NULL;
    trie->next = NULL;
    trie->flag = PARSE_INCOMPLETE;
    trie->c = '\0';

    const char* operators[35] = {
      "+", "-", "*", "/", "%", "++", "--",
      "==", "!=", ">", "<", ">=", "<=",
      "&&", "||", "!",
      "&", "|", "^", "~", "<<", ">>",
      "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=",
      ".", "->"
    };

    for (int i = 0; i < NELEMS(operators); i++) {
      insert(trie, operators[i]);
    }

    first_call = FALSE;
  }

  static struct TrieNode* now = NULL;

  if (rst) {
    now = trie;
  }
  else if (now == NULL) {
    return PARSE_END;
  }

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? PARSE_END : now->flag;
}

enum ParseResult delimiter_parser(char c, boolean rst) {

  if (rst == FALSE) {
    return PARSE_END;
  }

  static const char delimiters[10] = {
    ';', ',', ':', '?',
    '(', ')', '[', ']',
    '{', '}'
  };

  for (int i = 0; i < NELEMS(delimiters); i++) {
    if (c == delimiters[i]) {
      return PARSE_SUCCESS;
    }
  }

  return PARSE_END;
}

enum ParseResult charcon_parser(char c, boolean rst) {

  static enum {
    START,
    WIDE,
    EMPTY,
    WAIT,
    ESCAPE,
    HEX,
    U8,
    U7,
    U6,
    U5,
    U4,
    U3,
    U2,
    U1,
    ACCEPT,
    ERROR
  } state;

  if (rst) {
    state = START;
  }

  switch (state) {

    case START:
      if (c == '\'') {
        state = EMPTY;
      }
      else if (LOWER(c) == 'u' || c == 'L') {
        state = WIDE;
      }
      else {
        state = ERROR;
      }
      break;

    case WIDE:
      if (c == '\'') {
        state = EMPTY;
      }
      else {
        state = ERROR;
      }
      break;

    case EMPTY:
      if (c == '\\') {
        state = ESCAPE;
      }
      else if (c != '\'' || c != '\n') {
        state = WAIT;
      }
      else {
        state = ERROR;
      }
      break;

    case WAIT:
      if (c == '\\') {
        state = ESCAPE;
      }
      else if (c == '\'') {
        state = ACCEPT;
      }
      else if (c == '\n') {
        state = ERROR;
      }
      break;

    case ACCEPT:
      state = ERROR;
      break;

    case ESCAPE:
      if (c == '\'' || c == '\"' || c == '?' || c == '\\'
        || c == 'a' || c == 'b' || c == 'f' || c == 'n'
        || c == 'r' || c == 't' || c == 'v' || IS_OCT_DIGIT(c)) {
        state = WAIT;
      }
      else if (c == 'x') {
        state = U1;
      }
      else if (c == 'u') {
        state = U4;
      }
      else if (c == 'U') {
        state = U8;
      }
      else {
        state = ERROR;
      }
      break;

    case U8:
      if (isxdigit(c)) {
        state = U7;
      }
      else {
        state = ERROR;
      }
      break;

    case U7:
      if (isxdigit(c)) {
        state = U6;
      }
      else {
        state = ERROR;
      }
      break;

    case U6:
      if (isxdigit(c)) {
        state = U5;
      }
      else {
        state = ERROR;
      }
      break;

    case U5:
      if (isxdigit(c)) {
        state = U4;
      }
      else {
        state = ERROR;
      }
      break;

    case U4:
      if (isxdigit(c)) {
        state = U3;
      }
      else {
        state = ERROR;
      }
      break;

    case U3:
      if (isxdigit(c)) {
        state = U2;
      }
      else {
        state = ERROR;
      }
      break;

    case U2:
      if (isxdigit(c)) {
        state = U1;
      }
      else {
        state = ERROR;
      }
      break;

    case U1:
      if (isxdigit(c)) {
        state = WAIT;
      }
      else {
        state = ERROR;
      }
      break;
  }

  if (state == ERROR) {
    return PARSE_END;
  } else if (state == ACCEPT) {
    return PARSE_SUCCESS;
  } else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult string_parser(char c, boolean rst) {

  static enum {
    START,
    UPPER_PREF,
    LOWER_PREF,
    WAIT,
    ESCAPE,
    HEX,
    U8,
    U7,
    U6,
    U5,
    U4,
    U3,
    U2,
    U1,
    ACCEPT,
    ERROR
  } state;

  if (rst) {
    state = START;
  }

  switch (state) {

    case START:
      if (c == '\"') {
        state = WAIT;
      }
      else if (c == 'U' || c == 'L') {
        state = UPPER_PREF;
      }
      else if (c == 'u') {
        state = LOWER_PREF;
      }
      else {
        state = ERROR;
      }
      break;

    case UPPER_PREF:
      if (c == '\"') {
        state = WAIT;
      }
      else {
        state = ERROR;
      }
      break;

    case LOWER_PREF:
      if (c == '8') {
        state = UPPER_PREF;
      }
      else if (c == '\"') {
        state = WAIT;
      }
      else {
        state = ERROR;
      }
      break;

    case WAIT:
      if (c == '\\') {
        state = ESCAPE;
      }
      else if (c == '\"') {
        state = ACCEPT;
      }
      else if (c == '\n') {
        state = ERROR;
      }
      break;

    case ACCEPT:
      state = ERROR;
      break;

    case ESCAPE:
      if (c == '\'' || c == '\"' || c == '?' || c == '\\'
        || c == 'a' || c == 'b' || c == 'f' || c == 'n'
        || c == 'r' || c == 't' || c == 'v' || IS_OCT_DIGIT(c)) {
        state = WAIT;
      }
      else if (c == 'x') {
        state = U1;
      }
      else if (c == 'u') {
        state = U4;
      }
      else if (c == 'U') {
        state = U8;
      }
      else {
        state = ERROR;
      }
      break;

    case U8:
      if (isxdigit(c)) {
        state = U7;
      }
      else {
        state = ERROR;
      }
      break;

    case U7:
      if (isxdigit(c)) {
        state = U6;
      }
      else {
        state = ERROR;
      }
      break;

    case U6:
      if (isxdigit(c)) {
        state = U5;
      }
      else {
        state = ERROR;
      }
      break;

    case U5:
      if (isxdigit(c)) {
        state = U4;
      }
      else {
        state = ERROR;
      }
      break;

    case U4:
      if (isxdigit(c)) {
        state = U3;
      }
      else {
        state = ERROR;
      }
      break;

    case U3:
      if (isxdigit(c)) {
        state = U2;
      }
      else {
        state = ERROR;
      }
      break;

    case U2:
      if (isxdigit(c)) {
        state = U1;
      }
      else {
        state = ERROR;
      }
      break;

    case U1:
      if (isxdigit(c)) {
        state = WAIT;
      }
      else {
        state = ERROR;
      }
      break;
  }

  if (state == ERROR) {
    return PARSE_END;
  } else if (state == ACCEPT) {
    return PARSE_SUCCESS;
  } else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult integer_parser(char c, boolean rst) {

  static enum {
    START,
    ZERO,
    DEC,
    OCT,
    ZEROX,
    HEX,
    XL,
    XU,
    XLL,
    XUL,
    XSUF,
    ERROR
  } state;

  if (rst) {
    state = START;
  }

  switch (state) {

    case START:
      if (c == '0') {
        state = ZERO;
      }
      else if (isdigit(c)) {
        state = DEC;
      }
      else {
        state = ERROR;
      }
      break;

    case ZERO:
      if (IS_OCT_DIGIT(c)) {
        state = OCT;
      }
      else if (LOWER(c) == 'x') {
        state = ZEROX;
      }
      else if (LOWER(c) == 'l') {
        state = XL;
      }
      else if (LOWER(c) == 'u') {
        state = XU;
      }
      else {
        state = ERROR;
      }
      break;

    case DEC:
      if (LOWER(c) == 'l') {
        state = XL;
      }
      else if (LOWER(c) == 'u') {
        state = XU;
      }
      else if (!isdigit(c)) {
        state = ERROR;
      }
      break;

    case OCT:
      if (LOWER(c) == 'l') {
        state = XL;
      }
      else if (LOWER(c) == 'u') {
        state = XU;
      }
      else if (!IS_OCT_DIGIT(c)) {
        state = ERROR;
      }
      break;

    case ZEROX:
      if (isxdigit(c)) {
        state = HEX;
      }
      else {
        state = ERROR;
      }
      break;

    case HEX:
      if (LOWER(c) == 'l') {
        state = XL;
      }
      else if (LOWER(c) == 'u') {
        state = XU;
      }
      else if (!isxdigit(c)) {
        state = ERROR;
      }
      break;

    case XL:
      if (LOWER(c) == 'l') {
        state = XLL;
      }
      else if (LOWER(c) == 'u') {
        state = XSUF; // lu
      }
      else {
        state = ERROR;
      }
      break;

    case XU:
      if (LOWER(c) == 'l') {
        state = XUL;
      }
      else {
        state = ERROR;
      }
      break;

    case XLL:
      if (LOWER(c) == 'u') {
        state = XSUF; // llu
      }
      else {
        state = ERROR;
      }
      break;

    case XUL:
      if (LOWER(c) == 'l') {
        state = XSUF; // ull
      }
      else {
        state = ERROR;
      }
      break;

    case XSUF:
      state = ERROR;
      break;
  }

  if (state == ERROR) {
    return PARSE_END;
  } else if (state == ZEROX) {
    return PARSE_INCOMPLETE;
  } else {
    return PARSE_SUCCESS;
  }
}

enum ParseResult floating_parser(char c, boolean rst) {

  static enum {
    START,
    ZERO,
    DEC,
    DDOT,
    FDEC,
    ZEROX,
    HEX,
    HDOT,
    FHEX,
    EXP,
    SIGN,
    EDEC,
    FSUF,
    ERROR
  } state;

  if (rst) {
    state = START;
  }

  switch (state) {

    case START:
      if (c == '0') {
        state = ZERO;
      }
      else if (isdigit(c)) {
        state = DEC;
      }
      else if (c == '.') {
        state = DDOT;
      }
      else {
        state = ERROR;
      }
      break;

    case ZERO:
      if (isdigit(c)) {
        state = DEC;
      }
      else if (c == '.') {
        state = FDEC;
      }
      else if (LOWER(c) == 'x') {
        state = ZEROX;
      }
      else if (LOWER(c) == 'e') {
        state = EXP;
      }
      else {
        state = ERROR;
      }
      break;

    case DEC:
      if (c == '.') {
        state = FDEC;
      }
      else if (LOWER(c) == 'e') {
        state = EXP;
      }
      else if (!isdigit(c)) {
        state = ERROR;
      }
      break;

    case DDOT:
      if (isdigit(c)) {
        state = FDEC;
      }
      else {
        state = ERROR;
      }
      break;

    case FDEC:
      if (LOWER(c) == 'e') {
        state = EXP;
      }
      else if (LOWER(c) == 'f' || LOWER(c) == 'l') {
        state = FSUF;
      }
      else if (!isdigit(c)) {
        state = ERROR;
      }
      break;

    case ZEROX:
      if (isxdigit(c)) {
        state = HEX;
      }
      else if (c == '.') {
        state = HDOT;
      }
      else {
        state = ERROR;
      }
      break;

    case HEX:
      if (c == '.') {
        state = FHEX;
      }
      else {
        state = ERROR;
      }
      break;

    case HDOT:
      if (isxdigit(c)) {
        state = FHEX;
      }
      else {
        state = ERROR;
      }
      break;

    case FHEX:
      if (LOWER(c) == 'p') {
        state = EXP;
      }
      else if (!isxdigit(c)) {
        state = ERROR;
      }
      break;

    case EXP:
      if (c == '+' || c == '-') {
        state = SIGN;
      }
      else if (isdigit(c)) {
        state = EDEC;
      }
      else {
        state = ERROR;
      }
      break;

    case SIGN:
      if (isdigit(c)) {
        state = EDEC;
      }
      else {
        state = ERROR;
      }
      break;

    case EDEC:
      if (LOWER(c) == 'f' || LOWER(c) == 'l') {
        state = FSUF;
      }
      else if (!isdigit(c)) {
        state = ERROR;
      }
      break;

    case FSUF:
      state = ERROR;
      break;
  }

  if (state == ERROR) {
    return PARSE_END;
  } else if (state == FDEC || state == EDEC || state == FSUF) {
    return PARSE_SUCCESS;
  } else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult number_parser(char c, boolean rst) {

  enum ParseResult res_int = integer_parser(c, rst);
  enum ParseResult res_float = floating_parser(c, rst);

  if (res_int == PARSE_SUCCESS || res_float == PARSE_SUCCESS) {
    return PARSE_SUCCESS;
  } else if (res_int == PARSE_INCOMPLETE || res_float == PARSE_INCOMPLETE) {
    return PARSE_INCOMPLETE;
  } else {
    return PARSE_END;
  }
}

enum ParseResult error_parser(char c, boolean rst) {

  static enum {
    START,
    BAD_CHAR,
    BAD_CHARCON,
    BAD_STRING,
    NUM_PREF,
    BAD_IDENTIFIER,
    UPPER_PREF,
    LOWER_PREF,
    CHAR_ESCAPE,
    STR_ESCAPE,
    STR_PREF,
    ERROR
  } state;

  if (rst) {
    state = START;
  }

  switch (state) {

    case START:
      if (c == '\'') {
        state = BAD_CHARCON;
      }
      else if (c == '\"') {
        state = BAD_STRING;
      }
      else if (isdigit(c)) {
        state = NUM_PREF;
      }
      else if (c == 'L' || c == 'U') {
        state = UPPER_PREF;
      }
      else if (c == 'u') {
        state = LOWER_PREF;
      }
      else if (c != ' ' && c != '\n' && c != '\t' && c != EOF) {
        state = BAD_CHAR;
      }
      else {
        state = ERROR;
      }
      break;

    case BAD_CHAR:
      state = ERROR;
      break;

    case BAD_CHARCON:
      if (c == '\\') {
        state = CHAR_ESCAPE;
      }
      else if (c == '\n') {
        state = ERROR;
      }
      else if (c == '\'') {
        state = BAD_CHAR;
      }
      break;

    case BAD_STRING:
      if (c == '\\') {
        state = STR_ESCAPE;
      }
      else if (c == '\n') {
        state = ERROR;
      }
      else if (c == '\"') {
        state = BAD_CHAR;
      }
      break;

    case NUM_PREF:
      if (isalpha(c) || c == '_') {
        state = BAD_IDENTIFIER;
      }
      else if (!isdigit(c)) {
        state = ERROR;
      }
      break;

    case BAD_IDENTIFIER:
      if (!(isalnum(c) || c == '_')) {
        state = ERROR;
      }
      break;

    case UPPER_PREF:
      if (c == '\'') {
        state = BAD_CHARCON;
      }
      else if (c == '\"') {
        state = BAD_STRING;
      }
      else {
        state = ERROR;
      }
      break;

    case LOWER_PREF:
      if (c == '\'') {
        state = BAD_CHARCON;
      }
      else if (c == '\"') {
        state = BAD_STRING;
      }
      else if (c == '8') {
        state = STR_PREF;
      }
      else {
        state = ERROR;
      }
      break;

    case CHAR_ESCAPE:
      state = BAD_CHARCON;
      break;

    case STR_ESCAPE:
      state = BAD_STRING;
      break;

    case STR_PREF:
      if (c == '\"') {
        state = BAD_STRING;
      }
      else {
        state = ERROR;
      }
      break;
  }

  if (state == ERROR) {
    return PARSE_END;
  } else if (state == BAD_CHAR || state == BAD_CHARCON || state == BAD_STRING || state == BAD_IDENTIFIER) {
    return PARSE_SUCCESS;
  } else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult comment_parser(char c, boolean rst) {

  static enum {
    START,
    SLASH,
    SINGLE,
    MULTI,
    STAR,
    END,
    ERROR
  } state;

  if (rst) {
    state = START;
  }

  switch (state) {

    case START:
      if (c == '/') {
        state = SLASH;
      }
      else {
        state = ERROR;
      }
      break;

    case SLASH:
      if (c == '/') {
        state = SINGLE;
      }
      else if (c == '*') {
        state = MULTI;
      }
      else {
        state = ERROR;
      }
      break;

    case SINGLE:
      if (c == '\n') {
        state = ERROR;
      }
      break;

    case MULTI:
      if (c == '*') {
        state = STAR;
      }
      break;

    case STAR:
      if (c == '/') {
        state = END;
      }
      else if (c != '*') {
        state = MULTI;
      }
      break;
    
    case END:
      state = ERROR;
      break;
  }

  if (state == ERROR) {
    return PARSE_END;
  } else if (state == SLASH) {
    return PARSE_INCOMPLETE;
  } else {
    return PARSE_SUCCESS;
  }
}

typedef enum ParseResult (*TokenParser)(char, boolean);

enum ParseResult token_parser(char c, boolean rst, int i) {

  static const TokenParser parser[NTYPES + 1] = {
    keyword_parser, identifier_parser, operator_parser, delimiter_parser,
    charcon_parser, string_parser, number_parser, error_parser, comment_parser
  };

  return parser[i](c, rst);
}

const char* token_name(int i) {

  static const char* name[NTYPES] = {
    "KEYWORD", "IDENTIFIER", "OPERATOR", "DELIMITER",
    "CHARCON", "STRING", "NUMBER", "ERROR"
  };

  return name[i];
}

/*
 * let `arr` be all `val`
 * */
void set(int* arr, int len, int val) {
  for (int i = 0; i < len; i++) {
    arr[i] = val;
  }
}

/*
 * if `arr` is all `val`, return `1`
 * else return `0`
 * */
boolean all(enum ParseResult* arr, int len, enum ParseResult val) {

  for (int i = 0; i < len; i++) {
    if (arr[i] != val) {
      return FALSE;
    }
  }

  return TRUE;
}

/*
 * if arr is all `0`, return -1
 * else return index of max number
 * */
int max_idx(int* arr, int len) {

  int sum = 0;
  int idx = -1;

  for (int i = 0; i < len; i++) {
    if (arr[i] > sum) {
      sum = arr[i];
      idx = i;
    }
  }

  return idx;
}

void parse(char c, boolean rst, enum ParseResult* rv, int len) {
  for (int i = 0; i < len; i++) {
    if (rst == TRUE || rv[i] != PARSE_END) {
      rv[i] = token_parser(c, rst, i);
    }
  }
}

struct DoubleBuffer {
  FILE* fp;
  char* lexeme_begin;
  char* fwd;
  char buf[2][BUF_SIZE + 1];
  int lb_bf_n;
  int fwd_bf_n;
  int len;
};

void db_init(struct DoubleBuffer* db, FILE* fp) {
  db->fp = fp;
  db->lexeme_begin = &db->buf[1][BUF_SIZE];
  db->fwd = &db->buf[1][BUF_SIZE];
  db->buf[0][BUF_SIZE] = '\0';
  db->buf[1][BUF_SIZE] = EOF;
  db->lb_bf_n = 1;
  db->fwd_bf_n = 1;
  db->len = 0;
}

char db_getc(struct DoubleBuffer* db) {

  if (*db->fwd == EOF) {
    for (int i = 0; i < 2; i++) {
      if (db->fwd == &db->buf[i][BUF_SIZE]) {
        db->fwd_bf_n = OTHER(i);
        char* new_buf = db->buf[db->fwd_bf_n];

        if (new_buf[BUF_SIZE] != EOF) {
          int rc = fread(new_buf, sizeof(char), BUF_SIZE, db->fp);
          new_buf[rc] = EOF;
          new_buf[BUF_SIZE] = EOF;
        }

        if (db->lexeme_begin == db->fwd) {
          db->lexeme_begin = new_buf;
          db->lb_bf_n = db->fwd_bf_n;
          db->buf[i][BUF_SIZE] = '\0';
        }

        db->fwd = new_buf;
        break;
      }
    }
  }

  db->len++;

  return *db->fwd++;
}

void __db_ptok_nonfull(char* s, int len) {
  char tmp = s[len];
  s[len] = '\0';
  printf("%s", s);
  s[len] = tmp;
}

void db_ptok(struct DoubleBuffer* db, int len) {
  // assume that len <= BUF_SIZE
  int remain = &db->buf[db->lb_bf_n][BUF_SIZE] - db->lexeme_begin;

  if (remain >= len) {
    __db_ptok_nonfull(db->lexeme_begin, len);
  } else {
    __db_ptok_nonfull(db->lexeme_begin, remain);
    __db_ptok_nonfull(db->buf[OTHER(db->lb_bf_n)], len - remain);
  }
}

void db_move(struct DoubleBuffer* db, int len) {
  // assume that len <= BUF_SIZE
  int remain = &db->buf[db->lb_bf_n][BUF_SIZE] - db->lexeme_begin;

  if (remain >= len) {
    db->lexeme_begin += len;
  } else {
    db->buf[db->lb_bf_n][BUF_SIZE] = '\0';
    db->lb_bf_n = OTHER(db->lb_bf_n);
    db->lexeme_begin = &db->buf[db->lb_bf_n][len - remain];
  }
  db->fwd = db->lexeme_begin;
  db->fwd_bf_n = db->lb_bf_n;
  db->len = 0;
}

int db_get_len(struct DoubleBuffer* db) {
  return db->len;
}

void update_len(enum ParseResult* rv, int* len, int n, struct DoubleBuffer* db, int n_line) {

  int l = db_get_len(db);
  
  if (l == BUF_SIZE) {
    printf("%d <ERROR,", n_line);
    db_ptok(db, l);
    printf(">\n");
    db_move(db, l);
    set(len, NELEMS(len), 0);
    return;
  }

  for (int i = 0; i < n; i++) {
    if (rv[i] == PARSE_SUCCESS) {
      len[i] = l;
    }
  }
}

void print_answer(int n_line, int* n, int len) {

  printf("%d\n", n_line);

  for (int i = 0; i < len - 1; i++) {
    printf("%d ", n[i]);
  }

  printf("\n%d\n", n[len - 1]);
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  FILE* fp = fopen(argv[1], "r");
  
  if (fp == NULL) {
    printf("%s: cannot open %s\n", argv[0], argv[1]);
    exit(EXIT_FAILURE);
  }

  struct DoubleBuffer db;
  db_init(&db, fp);

  int n_line = 1;
  int n[NTYPES] = { 0 };
  int len[NTYPES + 1] = { 0 };
  boolean in_cmt = FALSE;

  char c;
  boolean rst = TRUE;
  enum ParseResult rv[NTYPES + 1];

  for (;;) {
    c = db_getc(&db);
    parse(c, rst, rv, NELEMS(rv));
    rst = FALSE;

    if (all(rv, NELEMS(rv), PARSE_END)) {
      int i = max_idx(len, NELEMS(len));

      if (i != -1) {
        if (i != NTYPES) {
          printf("%d <%s,", n_line, token_name(i));
          db_ptok(&db, len[i]);
          printf(">\n");
          n[i]++;
        }

        if (c == '\n') {
          n_line--; // skip '\n'
        }

        db_move(&db, len[i]);
      }
      else {
        if (c == EOF) {
          print_answer(n_line, n, NELEMS(n));
          exit(EXIT_SUCCESS);
        }

        db_move(&db, 1);
      }

      set(len, NELEMS(len), 0);
      rst = TRUE;
      in_cmt = FALSE;
    }
    else if (in_cmt) {
      db_move(&db, 1);
    }
    else {
      update_len(rv, len, NELEMS(rv), &db, n_line);

      if (all(rv, NELEMS(rv) - 1, PARSE_END)) {
        db_move(&db, len[NELEMS(rv) - 1]);
        set(len, NELEMS(len), 0);
        in_cmt = TRUE;
      }
    }

    if (c == '\n') {
      n_line++;
    }
  }

  return EXIT_SUCCESS;
}

