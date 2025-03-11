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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <common.h>
#include <stdio.h>
#include <utils.h>

#define Log(format, ...) \
    _Log(ANSI_FMT("[%s:%d %s] " format, ANSI_FG_BLUE) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      MUXDEF(CONFIG_TARGET_AM, printf(ANSI_FMT(format, ANSI_FG_RED) "\n", ## __VA_ARGS__), \
        (fflush(stdout), fprintf(stderr, ANSI_FMT(format, ANSI_FG_RED) "\n", ##  __VA_ARGS__))); \
      IFNDEF(CONFIG_TARGET_AM, extern FILE* log_fp; fflush(log_fp)); \
      extern void assert_fail_msg(); \
      assert_fail_msg(); \
      assert(cond); \
    } \
  } while (0)

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")

/* 指令环形缓冲区相关定义 */
#ifdef CONFIG_IRINGBUF
#define IRINGBUF_SIZE 16  // 环形缓冲区大小

typedef struct {
  vaddr_t pc;          // 指令地址
  uint8_t inst[4];     // 指令机器码
  int inst_len;        // 指令长度
  char disasm[128];    // 反汇编文本
} IRingbufEntry;

extern IRingbufEntry iringbuf[IRINGBUF_SIZE];  // 环形缓冲区
extern int iringbuf_index;                     // 当前写入位置
extern vaddr_t current_pc;                     // 当前执行的PC

void iringbuf_record(vaddr_t pc, uint8_t *inst, int inst_len, char *disasm);
void iringbuf_display();
#endif /* CONFIG_IRINGBUF */

#endif

/* 内存访问踪迹相关定义 */
#ifdef CONFIG_MTRACE
// 判断是否输出mtrace的条件
#define MTRACE_COND (CONFIG_MTRACE_COND)
// mtrace函数声明
void mtrace_write(bool is_write, paddr_t addr, int len, word_t data);
#endif
