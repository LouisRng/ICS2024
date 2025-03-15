#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

struct Context {
  // TODO: fix the order of these members to match trap.S: fixed
  // 通用寄存器
  uintptr_t gpr[NR_REGS];
  // 异常号
  uintptr_t mcause;
  // 处理器状态
  uintptr_t mstatus;
  // 触发异常时的PC
  uintptr_t mepc;
  // 地址空间信息
  void *pdir;
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[10] // a0用于系统调用返回值
#define GPR3 gpr[11] // a1
#define GPR4 gpr[12] // a2
#define GPRx gpr[10] // a0用于返回值

#endif
