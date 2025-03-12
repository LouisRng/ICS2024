#ifndef __CPU_IRINGBUF_H
#define __CPU_IRINGBUF_H

#include <common.h>

/* 环形缓冲区的大小 */
#define IRINGBUF_SIZE 16

/* 指令记录结构体 */
typedef struct {
  vaddr_t pc;       // 指令的pc值
  uint32_t inst;    // 指令的二进制表示
  char disasm[64];  // 反汇编结果
} iringbuf_entry_t;

/* 初始化缓冲区 */
void init_iringbuf();

/* 记录指令到环形缓冲区 */
void iringbuf_record(vaddr_t pc, uint32_t inst);

/* 打印环形缓冲区内容 */
void iringbuf_display(vaddr_t current_pc);

#endif
