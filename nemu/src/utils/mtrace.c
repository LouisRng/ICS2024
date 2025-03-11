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
#include <memory/paddr.h>

// 从cpu.h获取CPU定义
#include <cpu/cpu.h>

#ifdef CONFIG_MTRACE

// 记录内存访问信息到日志文件
void mtrace_write(bool is_write, paddr_t addr, int len, word_t data) {
  // 获取外部定义的log_fp
  extern FILE* log_fp;
  
  // 确定操作类型（读/写）
  const char* op = is_write ? "写入" : "读取";
  
  // 格式化并写入日志，使用CPU全局变量
  extern CPU_state cpu;  // 确保正确引用CPU变量
  fprintf(log_fp, "内存追踪: %s 地址=" FMT_PADDR " 长度=%d 数据=" FMT_WORD " PC=" FMT_WORD "\n",
          op, addr, len, data, cpu.pc);
}

#endif /* CONFIG_MTRACE */
