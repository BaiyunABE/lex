#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define NTYPES 8
#define BUF_SIZE 0x1000
#define TOKEN_SIZE 100
#define OTHER(x) (~x & 1u)
#define NELEMS(a) (sizeof(a) / sizeof(a[0]))
#define UPPER(c) (c | 32)
#define IS_OCT_DIGIT(c) ((c | 0x07) == '7')

typedef enum {
  FALSE,
  TRUE
} boolean;

enum ParseResult {
  PARSE_SUCCESS,
  PARSE_ERROR,
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
    return PARSE_ERROR;
  }

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? PARSE_ERROR : now->flag;
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
        state = PARSE_ERROR;
      }
      break;
    case PARSE_SUCCESS:
      if (isalnum(c) || c == '_') {
        state = PARSE_SUCCESS;
      }
      else {
        state = PARSE_ERROR;
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
    return PARSE_ERROR;

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? PARSE_ERROR : now->flag;
}

enum ParseResult delimiter_parser(char c, boolean restart) {
  if (restart == FALSE) {
    return PARSE_ERROR;
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
  return PARSE_ERROR;
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
      else if (UPPER(c) == 'u' || c == 'L') {
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
    return PARSE_ERROR;
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
    return PARSE_ERROR;
  }
  else if (state == 1) {
    return PARSE_SUCCESS;
  }
  else {
    return PARSE_INCOMPLETE;
  }
}

enum ParseResult number_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0: // s
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
      if (c == '.') {
        state = 3; // .
      }
      else if (UPPER(c) == 'e') {
        state = 5; // e
      }
      else if (IS_OCT_DIGIT(c)) {
        state = 8; // oct
      }
      else if (UPPER(c) == 'x') {
        state = 9; // hex
      }
      else if (UPPER(c) == 'l') {
        state = 10; // l
      }
      else if (UPPER(c) == 'u') {
        state = 12; // u
      }
      else if (UPPER(c) == 'f') {
        state = 14; // f
      }
      else {
        state = -1;
      }
      break;
    case 2: // x
      if (c == '.') {
        state = 3; // .
      }
      else if (UPPER(c) == 'e') {
        state = 5; // e
      }
      else if (UPPER(c) == 'l') {
        state = 10; // l
      }
      else if (UPPER(c) == 'u') {
        state = 12; // u
      }
      else if (UPPER(c) == 'f') {
        state = 14; // f
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 3: // .
      if (isdigit(c)) {
        state = 4; // .x
      }
      else {
        state = -1;
      }
      break;
    case 4: // .x
      if (UPPER(c) == 'e') {
        state = 5; // e
      }
      else if (UPPER(c) == 'l') {
        state = 10; // l
      }
      else if (UPPER(c) == 'u') {
        state = 12; // u
      }
      else if (UPPER(c) == 'f') {
        state = 14; // f
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 5: // e
      if (c == '+' || c == '-') {
        state = 6; // +
      }
      else if (isdigit(c)) {
        state = 7; // ex
      }
      else {
        state = -1;
      }
      break;
    case 6: // +
      if (isdigit(c )) {
        state = 7; // ex
      }
      else {
        state = -1;
      }
      break;
    case 7: // ex
      if (UPPER(c) == 'l') {
        state = 10; // l
      }
      else if (UPPER(c) == 'u') {
        state = 12; // u
      }
      else if (UPPER(c) == 'f') {
        state = 14; // f
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 8: // oct
      if (!IS_OCT_DIGIT(c)) {
        state = -1;
      }
      break;
    case 9: // hex
      if (!isxdigit(c)) {
        state = -1;
      }
      break;
    case 10: // l
      if (UPPER(c) == 'l') {
        state = 11; // ll
      }
      else if (UPPER(c) == 'u') {
        state = 14; // f(lu)
      }
      else {
        state = -1;
      }
      break;
    case 11: // ll
      if (UPPER(c) == 'u') {
        state = 14; // f(llu)
      }
      else {
        state = -1;
      }
      break;
    case 12: // u
      if (UPPER(c) == 'l') {
        state = 13; // ul
      }
      else {
        state = -1;
      }
      break;
    case 13: // ul
      if (UPPER(c) == 'l') {
        state = 14; // ull
      }
      else {
        state = -1;
      }
      break;
    case 14: // f
      state = -1;
      break;
  }

  if (state == -1) {
    return PARSE_ERROR;
  }
  else if (state == 3 || state == 5 || state == 6) {
    return PARSE_INCOMPLETE;
  }
  else {
    return PARSE_SUCCESS;
  }
}

enum ParseResult error_parser(char c, boolean restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '@') {
        state = 1;
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
      else {
        state = -1;
      }
      break;
    case 1:
      state = -1;
      break;
    case 2:
      if (c == '\n') {
        state = -1;
      }
      else if (c == '\'') {
        state = -1;
      }
      break;
    case 3:
      if (c == '\n') {
        state = -1;
      }
      else if (c == '\"') {
        state = -1;
      }
      break;
    case 4:
      if (isalpha(c) || c == '_') {
        state = 5;
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 5:
      if (!(isalnum(c) || c == '_')) {
        state = -1;
      }
      break;
  }

  if (state == -1) {
    return PARSE_ERROR;
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
    return PARSE_ERROR;
  }
  else if (state == 1) {
    return PARSE_INCOMPLETE;
  }
  else {
    return PARSE_SUCCESS;
  }
}

enum TokenType {
  KEYWORD,
  IDENTIFIER,
  OPERATOR,
  DELIMITER,
  CHARCON,
  STRING,
  NUMBER,
  ERROR,
  COMMENT
};

typedef enum ParseResult (*TokenParser)(char, boolean);


struct thread_data {
  enum TokenType type;
  char c;
  boolean restart;
};

void* token_parser(void* arg) {
  struct thread_data* data = (struct thread_data*)arg;
  static const TokenParser parser[NTYPES + 1] = {
    keyword_parser, identifier_parser, operator_parser, delimiter_parser,
    charcon_parser, string_parser, number_parser, error_parser, comment_parser
  };
  return (void*)parser[data->type](data->c, data->restart);
}

const char* token_name(enum TokenType type) {
  static const char* name[NTYPES] = {
    "KEYWORD", "IDENTIFIER", "OPERATOR", "DELIMITER",
    "CHARCON", "STRING", "NUMBER", "ERROR"
  };
  return name[type];
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
int all(enum ParseResult* arr, int len, enum ParseResult val) {
  for (int i = 0; i < len; i++) {
    if (arr[i] != val) {
      return 0;
    }
  }
  return 1;
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

  char buf[2][BUF_SIZE + 1];
  char token[TOKEN_SIZE];
  int idx = 0;

  buf[0][BUF_SIZE] = EOF;
  buf[1][BUF_SIZE] = EOF;

  char* lexeme_begin = buf[0];
  char* forward = &buf[1][BUF_SIZE];

  int n_line = 1;
  int n[NTYPES] = { 0 };
  int len[NTYPES + 1] = { 0 };

  for (;;) {
    switch (*forward) {
      case EOF:
        int cnt = 0;
        for (; cnt < 2; cnt++) {
          if (forward == &buf[cnt][BUF_SIZE]) {
            int i = OTHER(cnt);
            int rv = fread(buf[i], sizeof(char), BUF_SIZE, fp);
            buf[i][rv] = EOF;
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
        char c = *forward;
        token[idx++] = c;
        enum ParseResult rv[NTYPES + 1];
        pthread_t tids[NTYPES + 1];       
        struct thread_data data[NTYPES + 1];
        for (int i = 0; i < NELEMS(rv); i++) {
          data[i].type = i;
          data[i].c = c;
          data[i].restart = (lexeme_begin == forward);
          pthread_create(&tids[i], NULL, token_parser, (void*)&data[i]);
        }
        for (int i = 0; i < NELEMS(rv); i++) {
          pthread_join(tids[i], (void**)&rv[i]);
        }
        forward++;
        if (all(rv, NELEMS(rv), PARSE_ERROR)) {
          int i = max_idx(len, NELEMS(len));
          set(len, NELEMS(len), 0);
          lexeme_begin = forward;
          if (i != -1) {
            if (i != NTYPES) {
              token[idx - 1] = '\0';
              printf("%d <%s,%s>\n", n_line, token_name(i), token);
              n[i]++;
            }
            if (c == '\n') {
              n_line--;
            }
            forward--;
          }
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

