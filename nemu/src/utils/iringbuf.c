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

#include <common.h>

#ifdef CONFIG_IRINGBUF

/* 环形缓冲区及相关变量 */
IRingbufEntry iringbuf[IRINGBUF_SIZE] = {0};
int iringbuf_index = 0;
vaddr_t current_pc = 0;

/* 记录指令到环形缓冲区 */
void iringbuf_record(vaddr_t pc, uint8_t *inst, int inst_len, char *disasm) {
  // 更新当前执行的PC
  current_pc = pc;
  
  // 保存指令信息
  iringbuf[iringbuf_index].pc = pc;
  iringbuf[iringbuf_index].inst_len = inst_len;
  
  // 复制指令二进制表示
  memset(iringbuf[iringbuf_index].inst, 0, sizeof(iringbuf[iringbuf_index].inst));
  memcpy(iringbuf[iringbuf_index].inst, inst, inst_len);
  
  // 复制反汇编文本
  strncpy(iringbuf[iringbuf_index].disasm, disasm, sizeof(iringbuf[iringbuf_index].disasm) - 1);
  iringbuf[iringbuf_index].disasm[sizeof(iringbuf[iringbuf_index].disasm) - 1] = '\0';
  
  // 更新索引，实现环形覆盖
  iringbuf_index = (iringbuf_index + 1) % IRINGBUF_SIZE;
}

/* 显示环形缓冲区内容 */
void iringbuf_display() {
  printf("\n====== Instruction Ring Buffer ======\n");
  
  // 从当前索引开始，按时间顺序打印指令
  for (int i = 0; i < IRINGBUF_SIZE; i++) {
    // 计算要显示的条目索引
    int idx = (iringbuf_index + i) % IRINGBUF_SIZE;
    
    // 跳过未初始化的条目
    if (iringbuf[idx].pc == 0) continue;
    
    // 生成指令二进制表示字符串
    char inst_str[32] = {0};
    for (int j = 0; j < iringbuf[idx].inst_len; j++) {
#ifdef CONFIG_ISA_x86
      sprintf(inst_str + j*3, "%02x ", iringbuf[idx].inst[j]);
#else
      sprintf(inst_str + (iringbuf[idx].inst_len - j - 1)*3, "%02x ", 
              iringbuf[idx].inst[iringbuf[idx].inst_len - j - 1]);
#endif
    }
    
    // 打印指令，标记当前执行位置
    if (iringbuf[idx].pc == current_pc) {
      printf("--> " FMT_WORD ": %-30s %s\n", 
             iringbuf[idx].pc, iringbuf[idx].disasm, inst_str);
    } else {
      printf("    " FMT_WORD ": %-30s %s\n", 
             iringbuf[idx].pc, iringbuf[idx].disasm, inst_str);
    }
  }
  
  printf("======================================\n");
}

#endif /* CONFIG_IRINGBUF */
