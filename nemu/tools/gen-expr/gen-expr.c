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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static inline uint32_t choose(uint32_t n) {
  return rand() % n;
}

static inline void gen(const char *s) {
  strcat(buf, s);
}

// 生成一个随机数字（无符号整数）
static void gen_num() {
  char num_buf[12]; // 足够容纳uint32_t的最大值
  uint32_t num = rand() % 100; // 限制数字范围，避免过大
  sprintf(num_buf, "%uu", num); // 添加u后缀确保无符号
  gen(num_buf);
}

// 生成一个随机操作符
static void gen_rand_op() {
  // 操作符列表：+, -, *, /
  // 注意：/ 需要特殊处理以避免除零错误
  char ops[] = {'+', '-', '*', '/'};
  char op_buf[2];
  op_buf[0] = ops[choose(4)]; // 随机选择一个操作符
  op_buf[1] = '\0';
  
  // 随机插入空格
  if (choose(2)) {
    gen(" ");
  }
  
  gen(op_buf);
  
  // 随机插入空格
  if (choose(2)) {
    gen(" ");
  }
}

// 控制表达式的最大深度，避免栈溢出和过于复杂的表达式
static int expr_depth = 0;
static const int MAX_DEPTH = 10;

// 主要的表达式生成函数
static void _gen_rand_expr(int allow_div) {
  // 避免表达式过深导致栈溢出
  if (expr_depth > MAX_DEPTH) {
    gen_num();
    return;
  }

  // 增加表达式深度
  expr_depth++;
  
  // 随机选择表达式类型：数字、括号表达式或二元运算
  switch (choose(3)) {
    case 0: {
      // 生成数字
      gen_num();
      break;
    }
    case 1: {
      // 生成括号表达式
      gen("(");
      _gen_rand_expr(1);  // 括号内可以有除法
      gen(")");
      break;
    }
    default: {
      // 生成二元运算表达式
      int left_allow_div = allow_div && choose(2); // 左侧是否允许除法
      _gen_rand_expr(left_allow_div);
      
      char op;
      if (allow_div) {
        // 允许使用所有操作符
        op = "+-*/"[choose(4)];
      } else {
        // 不允许使用除法
        op = "+-*"[choose(3)];
      }
      
      // 随机插入空格
      if (choose(2)) {
        gen(" ");
      }
      
      char op_str[2] = {op, '\0'};
      gen(op_str);
      
      // 随机插入空格
      if (choose(2)) {
        gen(" ");
      }
      
      // 如果是除法，右侧不能为0（避免除零错误）
      if (op == '/') {
        // 确保不会除以0
        gen("(");
        _gen_rand_expr(0); // 递归生成表达式，但不允许除法（避免潜在的0结果）
        gen("+1u)"); // 加1确保非零
      } else {
        _gen_rand_expr(allow_div); // 递归生成表达式
      }
      break;
    }
  }
  
  // 减少表达式深度
  expr_depth--;
}

static void gen_rand_expr() {
  // 重置缓冲区和深度计数
  buf[0] = '\0';
  expr_depth = 0;
  
  // 生成表达式
  _gen_rand_expr(1); 
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
