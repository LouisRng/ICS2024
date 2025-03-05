/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NEQ, TK_AND, TK_OR, TK_HEX, TK_NUM, TK_REG, TK_DEREF
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  
  {"\\-", '-'},
  {"\\*", '*'},
  {"/", '/'},           // 在正则表达式中不是特殊字符
  {"\\(", '('},
  {"\\)", ')'},
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_NUM},
  {"\\$[a-zA-Z0-9]+", TK_REG}, 
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type; // 记录 token 的类型
  char str[32]; // 对于整数和寄存器，我们还需要把相应的子串记录下来 (溢出需要进行处理)
} Token;

static Token tokens[32] __attribute__((used)) = {}; // 顺序存放已经被识别的 token 信息
static int nr_token __attribute__((used))  = 0; // 指示已经被识别出的 token 数目

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUM: case TK_HEX: case TK_REG: 
            // 对于十六进制数，十进制数和寄存器组，需要存储字符串
            if (nr_token >= 64) {
              printf("Too many tokens (> 64)\n");
              return false;
            } 
            tokens[nr_token].type = rules[i].token_type;
            if (substr_len < sizeof(tokens[nr_token].str)) {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0';
            } else {
              printf("Error: Token too long at position %d\n", position - substr_len);
              return false;
            }
            nr_token++;
            break;  
          case '*':
            // 检查是否是引用字符 (通过判断前面一个字符)
            if (nr_token == 0 || (tokens[nr_token - 1].type != TK_NUM && \
                                  tokens[nr_token - 1].type != TK_HEX && \
                                  tokens[nr_token - 1].type != TK_REG && \
                                  tokens[nr_token - 1].type != ')')) {
              tokens[nr_token].type = TK_DEREF;  
            } else {
              tokens[nr_token].type = '*';
            }
            nr_token++;
            break;  
          /* case TK_AND: */
          /* case TK_OR: */
          default: 
            // 其它的 token 直接记录类型
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int count = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type != '(') {
      count++;
    }
    else if (tokens[i].type != ')') {
      count--;
    }

    // 如果括号已经匹配完，但是还没有到表达式结尾，则不是最外层括号
    if (count == 0 && i < q) {
      return false;
    }

    // 如果右括号过多，则语法错误
    if (count < 0) {
      return false;
    }
  }

  // 最终 count 应该等于 0 
  return count == 0;
  
}

static int get_priority(int type) {
  switch (type) {
    case TK_DEREF: return 1; // 解引用优先级最高
    case '*': case '/': return 2;
    case '+': case '-': return 3;
    case TK_EQ: case TK_NEQ: return 4;
    case TK_AND: return 5;
    case TK_OR: return 6;
    default: return 0;
  }  
}

static int find_main_operator(int p, int q) {
  int brack_count = 0;
  int op_pos = -1;
  int op_priority = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      brack_count++;
    }
    else if (tokens[i].type == ')') {
      brack_count--;
    } 
    else if (brack_count == 0) {
      // 只考虑不在括号内的运算符
      int current_priority = get_priority(tokens[i].type);
      if (current_priority > 0) {
        /* 优先级更低的运算符作为主运算符
         * 对于同等优先级，选择最右边的运算符
         * 除了解引用操作符，它是右结合的，需要选择最左边的
         */
        if (current_priority > op_priority || \
          (current_priority == op_priority && current_priority != 1)) {
          op_pos = i;
          op_priority = current_priority;
        }
      }
    }
  }

  return op_pos;
}

static word_t eval(int p, int q, bool *success) {
  if (p > q) {
    /* Bad expression */
    *success = false;
    printf("Error: Bad expression (%d > %d)\n", p, q);
    return 0;
  }
  else if (p == q) {
    /* Single token. 
     * For now this token should be a number.
     * Return the value of the number.
     */
    word_t val = 0;
    switch (tokens[p].type) {
      case TK_NUM:
        // 十进制数
        val = strtoul(tokens[p].str, NULL, 10);
        break;
      case TK_HEX:
        // 十六进制数
        val = strtoul(tokens[p].str, NULL, 16);
        break;
      case TK_REG:
        // 寄存器值，跳过'$'前缀
        val = isa_reg_str2val(tokens[p].str + 1, success);
        if (!*success) {
          printf("Error: Invalid register name: %s\n", tokens[p].str);
          return 0;
        }
        break;
      default:
        *success = false;
        printf("Error: Invalid token type at position %d\n", p);
        return 0;
    }
    return val;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  } 
  else {
    int op = find_main_operator(p, q);
    if (op == -1) {
      *success = false;
      printf("Error: Can't not find main operator between position %d and %d\n", p, q);
      return 0;
    }

    /* // 处理解引用操作符（单目运算符） */
    /* if (tokens[op].type == TK_DEREF) { */
    /*   word_t addr = eval(op + 1, q, success); */
    /*   if (!*success) return 0; */
    /*    */
    /*   // 解引用，从内存中读取值 */
    /*   return vaddr_read(addr, 4); */
    /* } */

    // 处理二元运算符
    word_t val1 = eval(p, op - 1, success);
    if (!*success) return 0;

    word_t val2 = eval(op + 1, q, success);
    if (!*success) return 0;

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': 
        if (val2 == 0) {
          *success = false;
          printf("Error: Division by zero\n");
          return 0;
        }
        return val1 / val2; 
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;

      default: 
        *success = false;
        printf("Error: unsupported opertor at position %d\n", op);
        return 0;
    }
  }
  *success = false;
  printf("Error: Unexpected control flow in eval function\n");
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  /* TODO(); */
  *success = true;
  return eval(0, nr_token - 1, success);
}
