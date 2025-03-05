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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_END;
  return -1;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  assert(arg != NULL);
  if (strcmp(arg, "r") == 0) {
    isa_reg_display();
  }
  return 0;
}

static int cmd_x(char *args) {
  /* 参数检查 */
  if (args == NULL) {
    printf("Error: Missing arguments for 'x' command\n");
    printf("Usage: x N EXPR - Examine N 4-byte words starting at address EXPR\n");
    return 0;
  }
  
  /* 获取扫描内存的数量 */
  char *arg_n = strtok(args, " ");
  if (arg_n == NULL) {
    printf("Error: Missing number of units to display\n");
    return 0;
  }
  
  /* 获取剩余的参数作为表达式 */
  char *expr_str = strtok(NULL, "");
  if (expr_str == NULL) {
    printf("Error: Missing address expression\n");
    return 0;
  }
  
  /* 转换扫描数量 */
  word_t num = (word_t)strtol(arg_n, NULL, 10);

  if (num <= 0) {
    printf("Error: Number of units must be positive\n");
    return 0;
  }
  
  /* 使用表达式求值获取地址 */
  bool success = true;
  paddr_t addr = expr(expr_str, &success);
  
  if (!success) {
    printf("Error: Invalid address expression\n");
    return 0;
  }

  /* 扫描并打印内存内容 */
  for (int i = 0; i < num; i++) {
    paddr_t value = paddr_read(addr + i * 4, 4);  // 每次偏移 4 字节
    printf("0x%08x:  0x%08x\n", addr + i * 4, value); 
  } 
  return 0; 
}

static int cmd_si(char *args); 

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execute n steps of commands, n is given after si", cmd_si },
  { "info", "Print information of register or watchpoint", cmd_info }, 
  { "x", "displays the contents of memory at a specified address", cmd_x},
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  
  if (arg == NULL) {
    /* no argument given, execute once */
    cpu_exec(1); 
    return 0;
  } 

  char *endptr;
  int steps = 0;
  steps = (int)strtol(arg, &endptr, 10);

  /* handle invalid input */
  if (*endptr != '\0'){
    printf("Error: Invalid input %s, must be a positive input integer.\n", arg);
    return 0;
  }

  if (steps <= 0) {
    printf("Error: Step count '%d' is invalid. Must be a positive integer.\n", steps);
    return 0;
  } 

  cpu_exec(steps);
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
