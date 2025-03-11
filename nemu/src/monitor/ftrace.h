#ifndef __MONITOR_FTRACE_H__
#define __MONITOR_FTRACE_H__

#include <common.h>

// ELF header structure (32-bit)
typedef struct {
  unsigned char e_ident[16];
  uint16_t      e_type;
  uint16_t      e_machine;
  uint32_t      e_version;
  uint32_t      e_entry;
  uint32_t      e_phoff;
  uint32_t      e_shoff;
  uint32_t      e_flags;
  uint16_t      e_ehsize;
  uint16_t      e_phentsize;
  uint16_t      e_phnum;
  uint16_t      e_shentsize;
  uint16_t      e_shnum;
  uint16_t      e_shstrndx;
} Elf32_Ehdr;

// Section header structure
typedef struct {
  uint32_t   sh_name;
  uint32_t   sh_type;
  uint32_t   sh_flags;
  uint32_t   sh_addr;
  uint32_t   sh_offset;
  uint32_t   sh_size;
  uint32_t   sh_link;
  uint32_t   sh_info;
  uint32_t   sh_addralign;
  uint32_t   sh_entsize;
} Elf32_Shdr;

// Symbol table entry
typedef struct {
  uint32_t   st_name;
  uint32_t   st_value;
  uint32_t   st_size;
  unsigned char st_info;
  unsigned char st_other;
  uint16_t   st_shndx;
} Elf32_Sym;

// Function symbol information
typedef struct {
  char* name;       // 函数名
  paddr_t start;    // 起始地址
  uint32_t size;    // 函数大小
} FuncInfo;

// Call stack entry
typedef struct {
  paddr_t caller_pc;  // 调用指令的地址
  paddr_t callee_addr; // 被调用的函数地址
} CallStackEntry;

// 初始化函数追踪器
void init_ftrace(const char *elf_file);

// 记录函数调用
void ftrace_call(paddr_t pc, paddr_t target);

// 记录函数返回
void ftrace_ret(paddr_t pc, paddr_t ret_addr);

// 根据地址获取函数名
const char* ftrace_func_name(paddr_t addr);

// 清理资源
void ftrace_cleanup(void);

// 常量定义
#define MAX_FUNC_COUNT 1024
#define MAX_CALL_DEPTH 256

// ELF常量
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define STT_FUNC 2
#define ELF32_ST_TYPE(i) ((i)&0xf)

#endif
