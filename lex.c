#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NTYPES 8
#define BUF_SIZE 0x1000
#define TOKEN_SIZE 100
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

struct node {
  struct node* child;
  struct node* next;
  enum ParseResult flag;
  char c;
};

void insert(struct node* trie, const char* s) {
  struct node** pp = &trie->child;
  struct node* p = NULL;
  for (int i = 0; s[i]; i++) {
    for (p = *pp; p != NULL; p = p->next) {
      if (p->c == s[i]) {
        pp = &p->child;
        break;
      }
    }
    if (p == NULL) {
      p = (struct node*)malloc(sizeof(struct node));
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

enum ParseResult keyword_parser(char c, boolean restart) {
  static boolean first_call = TRUE;
  static struct node* trie;

  if (first_call) {
    trie = (struct node*)malloc(sizeof(struct node));
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

  static struct node* now = NULL;

  if (restart) {
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

enum ParseResult identifier_parser(char c, boolean restart) {
  static enum ParseResult state;

  if (restart) {
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

enum ParseResult operator_parser(char c, boolean restart) {
  static boolean first_call = TRUE;
  static struct node* trie;

  if (first_call) {
    trie = (struct node*)malloc(sizeof(struct node));
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

  static struct node* now = NULL;

  if (restart) {
    now = trie;
  }
  else if (now == NULL)
    return PARSE_END;

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? PARSE_END : now->flag;
}

enum ParseResult delimiter_parser(char c, boolean restart) {
  if (restart == FALSE) {
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

enum ParseResult charcon_parser(char c, boolean restart) {
  static int state;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '\'') {
        state = 2;
      }
      else if (LOWER(c) == 'u' || c == 'L') {
        state = 4;
      }
      else {
        state = -1;
      }
      break;
    case 1:
      state = -1;
      break;
    case 2:
      if (c == '\\') {
        state = 3;
      }
      else if (c == '\'') {
        state = 1;
      }
      else if (c == '\n') {
        state = -1;
      }
      break;
    case 3:
      state = 2;
      break;
    case 4:
      if (c == '\'') {
        state = 2;
      }
      else {
        state = -1;
      }
      break;
  }

  if (state == -1) {
    return PARSE_END;
  }
  else if (state == 1) {
    return PARSE_SUCCESS;
  }
  else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult string_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '\"') {
        state = 2;
      }
      else if (c == 'U' || c == 'L') {
        state = 4;
      }
      else if (c == 'u') {
        state = 5;
      }
      else {
        state = -1;
      }
      break;
    case 1:
      state = -1;
      break;
    case 2:
      if (c == '\\') {
        state = 3;
      }
      else if (c == '\"') {
        state = 1;
      }
      else if (c == '\n') {
        state = -1;
      }
      break;
    case 3:
      state = 2;
      break;
    case 4:
      if (c == '\"') {
        state = 2;
      }
      else {
        state = -1;
      }
      break;
    case 5:
      if (c == '\"') {
        state = 2;
      }
      else if (c == '8') {
        state = 4;
      }
      else {
        state = -1;
      }
      break;
  }

  if (state == -1) {
    return PARSE_END;
  }
  else if (state == 1) {
    return PARSE_SUCCESS;
  }
  else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult integer_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '0') {
        state = 1; // 0
      }
      else if (isdigit(c)) {
        state = 2; // dec
      }
      else {
        state = -1;
      }
      break;
    case 1:
      if (IS_OCT_DIGIT(c)) {
        state = 3; // oct
      }
      else if (LOWER(c) == 'x') {
        state = 4; // 0x
      }
      else if (LOWER(c) == 'l') {
        state = 6; // l
      }
      else if (LOWER(c) == 'u') {
        state = 7; // u
      }
      else {
        state = -1;
      }
      break;
    case 2: // dec
      if (LOWER(c) == 'l') {
        state = 6; // l
      }
      else if (LOWER(c) == 'u') {
        state = 7; // u
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 3: // oct
      if (LOWER(c) == 'l') {
        state = 6; // l
      }
      else if (LOWER(c) == 'u') {
        state = 7; // u
      }
      else if (!IS_OCT_DIGIT(c)) {
        state = -1;
      }
      break;
    case 4: // 0x
      if (isxdigit(c)) {
        state = 5; // hex
      }
      else {
        state = -1;
      }
      break;
    case 5: // hex
      if (LOWER(c) == 'l') {
        state = 6; // l
      }
      else if (LOWER(c) == 'u') {
        state = 7; // u
      }
      else if (!isxdigit(c)) {
        state = -1;
      }
      break;
    case 6: // l
      if (LOWER(c) == 'l') {
        state = 8; // ll
      }
      else if (LOWER(c) == 'u') {
        state = 10; // lu
      }
      else {
        state = -1;
      }
      break;
    case 7: // u
      if (LOWER(c) == 'l') {
        state = 9; // ul
      }
      else {
        state = -1;
      }
      break;
    case 8: // ll
      if (LOWER(c) == 'u') {
        state = 10; // llu
      }
      else {
        state = -1;
      }
      break;
    case 9: // ul
      if (LOWER(c) == 'l') {
        state = 10; // ull
      }
      else {
        state = -1;
      }
      break;
    case 10:
      state = -1;
      break;
  }

  if (state == -1) {
    return PARSE_END;
  }
  else if (state == 4) {
    return PARSE_INCOMPLETE;
  }
  else {
    return PARSE_SUCCESS;
  }
}

enum ParseResult floating_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '0') {
        state = 1; // 0
      }
      else if (isdigit(c)) {
        state = 2; // x
      }
      else if (c == '.') {
        state = 3; // .
      }
      else {
        state = -1;
      }
      break;
    case 1: // 0
      if (isdigit(c)) {
        state = 2; // x
      }
      else if (c == '.') {
        state = 4; // dec
      }
      else if (LOWER(c) == 'x') {
        state = 5; // 0x
      }
      else if (LOWER(c) == 'e') {
        state = 9; // e
      }
      else {
        state = -1;
      }
      break;
    case 2: // x
      if (c == '.') {
        state = 4; // dec
      }
      else if (LOWER(c) == 'e') {
        state = 9; // e
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 3: // .
      if (isdigit(c)) {
        state = 4; // dec
      }
      else {
        state = -1;
      }
      break;
    case 4: // dec
      if (LOWER(c) == 'e') {
        state = 9; // e
      }
      else if (LOWER(c) == 'f' || LOWER(c) == 'l') {
        state = 12; // f/l
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 5: // 0x
      if (isxdigit(c)) {
        state = 6; // hx
      }
      else if (state == '.') {
        state = 7; // h.
      }
      else {
        state = -1;
      }
      break;
    case 6: // hx
      if (c == '.') {
        state = 8; // hex
      }
      else {
        state = -1;
      }
      break;
    case 7: // h.
      if (isxdigit(c)) {
        state = 8; // hex
      }
      else {
        state = -1;
      }
      break;
    case 8: // hex
      if (LOWER(c) == 'p') {
        state = 9; // e
      }
      else if (!isxdigit(c)) {
        state = -1;
      }
      break;
    case 9: // e
      if (c == '+' || c == '-') {
        state = 10; // +
      }
      else if (isdigit(c)) {
        state = 11; // ex
      }
      else {
        state = -1;
      }
      break;
    case 10: // +
      if (isdigit(c)) {
        state = 11; // ex
      }
      else {
        state = -1;
      }
      break;
    case 11: // ex
      if (LOWER(c) == 'f' || LOWER(c) == 'l') {
        state = 12; // f/l
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 12:
      state = -1;
      break;
  }

  if (state == -1) {
    return PARSE_END;
  }
  else if (state == 4 || state == 11 || state == 12) {
    return PARSE_SUCCESS;
  }
  else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult number_parser(char c, boolean restart) {
  enum ParseResult res_int = integer_parser(c, restart);
  enum ParseResult res_float = floating_parser(c, restart);

  if (res_int == PARSE_SUCCESS || res_float == PARSE_SUCCESS) {
    return PARSE_SUCCESS;
  }
  else if (res_int == PARSE_INCOMPLETE || res_float == PARSE_INCOMPLETE) {
    return PARSE_INCOMPLETE;
  }
  else {
    return PARSE_END;
  }
}

enum ParseResult error_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '@' || c == '$' || c == '`') {
        state = 1; // @
      }
      else if (c == '\'') {
        state = 2;
      }
      else if (c == '\"') {
        state = 3;
      }
      else if (isdigit(c)) {
        state = 4;
      }
      else if (c == 'L' || c == 'U') {
        state = 6;
      }
      else if (c == 'u') {
        state = 7;
      }
      else {
        state = -1;
      }
      break;
    case 1: // illegal char
      state = -1;
      break;
    case 2: // char
      if (c == '\\') {
        state = 8;
      }
      else if (c == '\n' || c == '\'') {
        state = -1;
      }
      break;
    case 3: // string
      if (c == '\\') {
        state = 9;
      }
      else if (c == '\n' || c == '\"') {
        state = -1;
      }
      break;
    case 4: // number
      if (isalpha(c) || c == '_') {
        state = 5;
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 5: // illegal identifier
      if (!(isalnum(c) || c == '_')) {
        state = -1;
      }
      break;
    case 6: // L/U
      if (c == '\'') {
        state = 2;
      }
      else if (c == '\"') {
        state = 3;
      }
      else {
        state = -1;
      }
      break;
    case 7: // u
      if (c == '\'') {
        state = 2;
      }
      else if (c == '\"') {
        state = 3;
      }
      else if (c == '8') {
        state = 10;
      }
      else {
        state = -1;
      }
      break;
    case 8:
      state = 2;
      break;
    case 9:
      state = 3;
      break;
    case 10:
      if (c == '\"') {
        state = 3;
      }
      else {
        state = -1;
      }
      break;
  }

  if (state == -1) {
    return PARSE_END;
  }
  else if (state == 4) {
    return PARSE_INCOMPLETE;
  }
  else {
    return PARSE_SUCCESS;
  }
}

enum ParseResult comment_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '/') {
        state = 1;
      }
      else {
        state = -1;
      }
      break;
    case 1:
      if (c == '/') {
        state = 2;
      }
      else if (c == '*') {
        state = 3;
      }
      else {
        state = -1;
      }
      break;
    case 2:
      if (c == '\n') {
        state = -1;
      }
      break;
    case 3:
      if (c == '*') {
        state = 4;
      }
      break;
    case 4:
      if (c == '/') {
        state = 5;
      }
      else if (c != '*') {
        state = 3;
      }
      break;
    case 5:
      state = -1;
      break;
  }

  if (state == -1) {
    return PARSE_END;
  }
  else if (state == 1) {
    return PARSE_INCOMPLETE;
  }
  else {
    return PARSE_SUCCESS;
  }
}

typedef enum ParseResult (*TokenParser)(char, boolean);

enum ParseResult token_parser(char c, boolean restart, int i) {
  static const TokenParser parser[NTYPES + 1] = {
    keyword_parser, identifier_parser, operator_parser, delimiter_parser,
    charcon_parser, string_parser, number_parser, error_parser, comment_parser
  };
  return parser[i](c, restart);
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

void parse(char c, boolean restart, enum ParseResult* rv, int len) {
  for (int i = 0; i < len; i++) {
    rv[i] = token_parser(c, restart, i);
  }
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

  char token[TOKEN_SIZE];
  int idx = 0;

  int n_line = 1;
  int n[NTYPES] = { 0 };
  int len[NTYPES + 1] = { 0 };

  char c;
  boolean restart;
  enum ParseResult rv[NTYPES + 1];

  int cnt;

  char buf[2][BUF_SIZE + 1];
  char* lexeme_begin = buf[0];
  char* forward = &buf[1][BUF_SIZE];
  
  buf[0][BUF_SIZE] = EOF;
  buf[1][BUF_SIZE] = EOF;

  for (;;) {
    switch (*forward) {
      case EOF:
        for (cnt = 0; cnt < 2; cnt++) {
          if (forward == &buf[cnt][BUF_SIZE]) {
            int i = OTHER(cnt);
            int rc = fread(buf[i], sizeof(char), BUF_SIZE, fp);
            buf[i][rc] = EOF;
            if (lexeme_begin == forward)
              lexeme_begin = buf[i];
            forward = buf[i];
            break;
          }
        }
        if (cnt == 2) {
          if (lexeme_begin != forward) {
            int i = max_idx(len, NELEMS(len));
            if (i != -1 && i != NTYPES) {
              token[idx] = '\0';
              printf("%d <%s,%s>\n", n_line, token_name(i), token);
              n[i]++;
            }
          }
          printf("%d\n", n_line);
          for (int i = 0; i < NTYPES - 1; i++) {
            printf("%d%c", n[i], i == NTYPES - 2 ? '\n' : ' ');
          }
          printf("%d\n", n[NTYPES - 1]);
          exit(EXIT_SUCCESS);
        }
        break;

      default:
        c = *forward;
        restart = (lexeme_begin == forward) ? TRUE : FALSE;
        parse(c, restart, rv, NELEMS(rv));

        token[idx++] = c;
        forward++;

        if (all(rv, NELEMS(rv), PARSE_END)) {
          int i = max_idx(len, NELEMS(len));
          if (i != -1) {
            if (i != NTYPES) {
              token[len[i]] = '\0';
              printf("%d <%s,%s>\n", n_line, token_name(i), token);
              n[i]++;
            }
            if (c == '\n') {
              n_line--;
            }
            forward = lexeme_begin + len[i];
          }
          set(len, NELEMS(len), 0);
          lexeme_begin = forward;
          idx = 0;
        }
        else {
          for (int i = 0; i < NELEMS(rv); i++) {
            if (rv[i] == PARSE_SUCCESS) {
              len[i] = forward - lexeme_begin;
            }
          }
        }

        if (c == '\n') {
          n_line++;
        }
        break;
    }
  }
  return 0;
}

