#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NTYPES 8
#define N 4096
#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct node {
  struct node* child;
  struct node* next;
  int flag;
  char c;
} node;

void insert(node* trie, const char* s) {
  node** pp = &trie->child;
  node* p = NULL;
  for (int i = 0; s[i]; i++) {
    for (p = *pp; p != NULL; p = p->next) {
      if (p->c == s[i]) {
        pp = &p->child;
        break;
      }
    }
    if (p == NULL) {
      p = (node*)malloc(sizeof(node));
      p->child = NULL;
      p->next = *pp;
      p->flag = 0;
      p->c = s[i];
      *pp = p;
      pp = &p->child;
    }
  }
  p = (node*)pp;
  p->flag = 1;
}

int is_keyword(const char c, const int restart) {
  static int first_call = 1;
  static node* trie;

  if (first_call) {
    trie = (node*)malloc(sizeof(node));
    trie->child = NULL;
    trie->next = NULL;
    trie->flag = 0;
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
    first_call = 0;
  }

  static node* now = NULL;

  if (restart) {
    now = trie;
  }
  else if (now == NULL)
    return -1;

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? -1 : now->flag;
}

int is_identifier(const char c, const int restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (isalpha(c) || c == '_') {
        state = 1;
      }
      else {
        state = -1;
      }
      break;
    case 1:
      if (isalnum(c) || c == '_') {
        state = 1;
      }
      else {
        state = -1;
      }
  }

  return state;
}

int is_operator(const char c, const int restart) {
  static int first_call = 1;
  static node* trie;

  if (first_call) {
    trie = (node*)malloc(sizeof(node));
    trie->child = NULL;
    trie->next = NULL;
    trie->flag = 0;
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
    first_call = 0;
  }

  static node* now = NULL;

  if (restart) {
    now = trie;
  }
  else if (now == NULL)
    return -1;

  for (now = now->child; now != NULL; now = now->next) {
    if (c == now->c) {
      break;
    }
  }

  return now == NULL ? -1 : now->flag;
}

int is_delimiter(const char c, const int restart) {
  if (restart == 0)
    return -1;
  const static char delimiters[10] = {
    ';', ',', ':', '?',
    '(', ')', '[', ']',
    '{', '}'
  };
  for (int i = 0; i < 10; i++) {
    if (c == delimiters[i]) {
      return 1;
    }
  }
  return -1;
}

int is_charcon(const char c, const int restart) {
  static int state = 0;

  if (restart) {
    state = 0;
  }

  switch (state) {
    case 0:
      if (c == '\'') {
        state = 2;
      }
      else if ((c | 32) == 'u' || c == 'L') {
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
  }

  return state == 1 || state == -1 ? state : 0;
}

int is_string(const char c, const int restart) {
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
  }

  return state == 1 || state == -1 ? state : 0;
}

int is_number(const char c, const int restart) {
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
      else if ((c | 32) == 'e') {
        state = 5; // e
      }
      else if ((c | 0x07) == '7') {
        state = 8; // oct
      }
      else if ((c | 32) == 'x') {
        state = 9; // hex
      }
      else if ((c | 32) == 'l') {
        state = 10; // l
      }
      else if ((c | 32) == 'u') {
        state = 12; // u
      }
      else if ((c | 32) == 'f') {
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
      else if ((c | 32) == 'e') {
        state = 5; // e
      }
      else if ((c | 32) == 'l') {
        state = 10; // l
      }
      else if ((c | 32) == 'u') {
        state = 12; // u
      }
      else if ((c | 32) == 'f') {
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
      if ((c | 32) == 'e') {
        state = 5; // e
      }
      else if ((c | 32) == 'l') {
        state = 10; // l
      }
      else if ((c | 32) == 'u') {
        state = 12; // u
      }
      else if ((c | 32) == 'f') {
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
      if ((c | 32) == 'l') {
        state = 10; // l
      }
      else if ((c | 32) == 'u') {
        state = 12; // u
      }
      else if ((c | 32) == 'f') {
        state = 14; // f
      }
      else if (!isdigit(c)) {
        state = -1;
      }
      break;
    case 8: // oct
      if ((c | 0x07) != '7') {
        state = -1;
      }
      break;
    case 9: // hex
      if (!isxdigit(c)) {
        state = -1;
      }
      break;
    case 10: // l
      if ((c | 32) == 'l') {
        state = 11; // ll
      }
      else if ((c | 32) == 'u') {
        state = 14; // f(lu)
      }
      else {
        state = -1;
      }
      break;
    case 11: // ll
      if ((c | 32) == 'u') {
        state = 14; // f(llu)
      }
      else {
        state = -1;
      }
      break;
    case 12: // u
      if ((c | 32) == 'l') {
        state = 13; // ul
      }
      else {
        state = -1;
      }
      break;
    case 13: // ul
      if ((c | 32) == 'l') {
        state = 14; // ull
      }
      else {
        state = -1;
      }
      break;
    case 14: // f
      state = -1;
  }

  return state == -1 ? -1 : (state == 3 || state == 5 || state == 6 ? 0 : 1);
}

int is_error(const char c, const int restart) {
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
  }
  return state == -1 ? -1 : state == 4 ? 0 : 1;
}

int is_comment(const char c, const int restart) {  
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
  }

  return state == -1 ? -1 : state == 1 ? 0 : 1;
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
int all(int* arr, int len, int val) {
  for (int i = 0; i < len; i++) {
    if (arr[i] != val) {
      return 0;
    }
  }
  return 1;
}

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

typedef int (*func)(const char, const int);

int main(int argc, char* argv[])
{
  if (argc < 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    exit(1);
  }
  FILE* fp = fopen(argv[1], "r");
  if (fp == NULL) {
    printf("cannot open file `%s`.\n", argv[1]);
    exit(1);
  }

  const char* token_name[NTYPES] = {
    "KEYWORD", "IDENTIFIER", "OPERATOR", "DELIMITER",
    "CHARCON", "STRING", "NUMBER", "ERROR"
  };

  const func is_token[NTYPES + 1] = {
    is_keyword, is_identifier, is_operator, is_delimiter,
    is_charcon, is_string, is_number, is_error, is_comment
  };

  char buf[2][N + 1];
  char token[100];
  int idx = 0;

  buf[0][N] = EOF;
  buf[1][N] = EOF;

  char* lexeme_begin = buf[0];
  char* forward = &buf[1][N];

  int n_line = 1;
  int n[NTYPES] = { 0 };
  int len[NTYPES + 1] = { 0 };

  for (;;) {
    switch (*forward) {
      case EOF:
        if (forward == &buf[0][N]) {
          int rv = fread(buf[1], sizeof(char), N, fp);
          buf[1][rv] = EOF;
          if (lexeme_begin == forward)
            lexeme_begin = buf[1];
          forward = buf[1];
        }
        else if (forward == &buf[1][N]) {
          int rv = fread(buf[0], sizeof(char), N, fp);
          buf[0][rv] = EOF;
          if (lexeme_begin == forward)
            lexeme_begin = buf[0];
          forward = buf[0];
        }
        else {
          if (lexeme_begin != forward) {
            token[idx] = '\0';
            int i = max_idx(len, NELEMS(len));
            if (i != -1 && i != NTYPES) {
              printf("%d <%s,%s>\n", n_line, token_name[i], token);
              n[i]++;
            }
          }
          printf("%d\n%d %d %d %d %d %d %d\n%d\n",
            n_line, n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7]);
          exit(0);
        }
        break;

      default:
        char c = *forward;
        token[idx++] = c;
        int rv[NTYPES + 1];
        for (int i = 0; i < NELEMS(rv); i++) {
          rv[i] = is_token[i](c, lexeme_begin == forward);
        }
        if (all(rv, NELEMS(rv), -1)) {
          token[idx - 1] = '\0';
          int i = max_idx(len, NELEMS(len));
          if (i != -1) {
            if (i != NTYPES) {
              printf("%d <%s,%s>\n", n_line, token_name[i], token);
              n[i]++;
            }
          }
          else {
            forward++;
            if (c == '\n')
              n_line++;
          }
          idx = 0;
          lexeme_begin = forward;
          set(len, NELEMS(len), 0);
        }
        else {
          forward++;
          if (c == '\n')
            n_line++;
          for (int i = 0; i < NELEMS(rv); i++) {
            if (rv[i] == 1) {
              len[i] = forward - lexeme_begin;
            }
          }
        }
    }
  }

  return 0;
}

