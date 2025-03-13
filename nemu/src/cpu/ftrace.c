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

#include <cpu/ftrace.h>
#include <elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// 函数符号表项结构
typedef struct {
  vaddr_t addr;       // 函数起始地址
  uint32_t size;      // 函数大小
  char *name;         // 函数名称
} Func_Symbol;

static Func_Symbol *func_symbols = NULL;
static int func_symbol_count = 0;
static int call_depth = 0;
static bool ftrace_enabled = false;

// 解析ELF文件提取函数符号信息
static void parse_elf(const char *elf_file) {
  if (!elf_file) {
    printf("No ELF file provided for ftrace\n");
    return;
  }

  int fd = open(elf_file, O_RDONLY);
  if (fd < 0) {
    printf("Failed to open ELF file: %s\n", elf_file);
    return;
  }

  // 读取ELF头
  Elf32_Ehdr elf_header;
  if (read(fd, &elf_header, sizeof(elf_header)) != sizeof(elf_header)) {
    printf("Failed to read ELF header\n");
    close(fd);
    return;
  }

  // 验证是否为有效的ELF文件
  if (elf_header.e_ident[EI_MAG0] != ELFMAG0 || 
      elf_header.e_ident[EI_MAG1] != ELFMAG1 ||
      elf_header.e_ident[EI_MAG2] != ELFMAG2 ||
      elf_header.e_ident[EI_MAG3] != ELFMAG3) {
    printf("Not a valid ELF file\n");
    close(fd);
    return;
  }

  // 读取节头表
  Elf32_Shdr *section_headers = malloc(elf_header.e_shentsize * elf_header.e_shnum);
  lseek(fd, elf_header.e_shoff, SEEK_SET);
  if (read(fd, section_headers, elf_header.e_shentsize * elf_header.e_shnum) != 
      elf_header.e_shentsize * elf_header.e_shnum) {
    printf("Failed to read section headers\n");
    free(section_headers);
    close(fd);
    return;
  }

  // 寻找符号表和字符串表
  Elf32_Shdr *symtab_header = NULL;
  Elf32_Shdr *strtab_header = NULL;

  for (int i = 0; i < elf_header.e_shnum; i++) {
    if (section_headers[i].sh_type == SHT_SYMTAB) {
      symtab_header = &section_headers[i];
    } else if (section_headers[i].sh_type == SHT_STRTAB && 
               i != elf_header.e_shstrndx) {
      // 我们需要的是与符号关联的字符串表，而不是节名称的字符串表
      strtab_header = &section_headers[i];
    }
  }

  if (!symtab_header || !strtab_header) {
    printf("Symbol table or string table not found\n");
    free(section_headers);
    close(fd);
    return;
  }

  // 读取字符串表
  char *strtab = malloc(strtab_header->sh_size);
  lseek(fd, strtab_header->sh_offset, SEEK_SET);
  if (read(fd, strtab, strtab_header->sh_size) != strtab_header->sh_size) {
    printf("Failed to read string table\n");
    free(strtab);
    free(section_headers);
    close(fd);
    return;
  }

  // 读取符号表
  Elf32_Sym *symtab = malloc(symtab_header->sh_size);
  lseek(fd, symtab_header->sh_offset, SEEK_SET);
  if (read(fd, symtab, symtab_header->sh_size) != symtab_header->sh_size) {
    printf("Failed to read symbol table\n");
    free(symtab);
    free(strtab);
    free(section_headers);
    close(fd);
    return;
  }

  // 统计函数符号数量
  int symbol_count = symtab_header->sh_size / sizeof(Elf32_Sym);
  for (int i = 0; i < symbol_count; i++) {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC && symtab[i].st_size > 0) {
      func_symbol_count++;
    }
  }

  // 为函数符号分配内存并填充数据
  func_symbols = malloc(func_symbol_count * sizeof(Func_Symbol));
  if (!func_symbols) {
    printf("Failed to allocate memory for function symbols\n");
    free(symtab);
    free(strtab);
    free(section_headers);
    close(fd);
    return;
  }

  int idx = 0;
  for (int i = 0; i < symbol_count; i++) {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC && symtab[i].st_size > 0) {
      func_symbols[idx].addr = symtab[i].st_value;
      func_symbols[idx].size = symtab[i].st_size;
      func_symbols[idx].name = strdup(&strtab[symtab[i].st_name]);
      idx++;
    }
  }

  // 清理资源
  free(strtab);
  free(symtab);
  free(section_headers);
  close(fd);

  printf("ftrace: loaded %d function symbols\n", func_symbol_count);
  ftrace_enabled = true;
}

// 根据地址获取函数名
const char* get_func_name(vaddr_t addr) {
  for (int i = 0; i < func_symbol_count; i++) {
    if (addr >= func_symbols[i].addr && 
        addr < func_symbols[i].addr + func_symbols[i].size) {
      return func_symbols[i].name;
    }
  }
  return "???";
}

// 初始化ftrace
void init_ftrace(const char *elf_file) {
  parse_elf(elf_file);
}

// 记录函数调用
void ftrace_call(vaddr_t pc, vaddr_t target) {
  if (!ftrace_enabled) return;
  
  const char *func_name = get_func_name(target);
  
  // 按调用深度打印缩进
  printf("0x%08x:", pc);
  for (int i = 0; i < call_depth; i++) printf("  ");
  printf(" call [%s@0x%08x]\n", func_name, target);
  
  call_depth++;
}

// 记录函数返回
void ftrace_ret(vaddr_t pc, vaddr_t ret_addr) {
  if (!ftrace_enabled || call_depth <= 0) return;
  
  call_depth--;
  
  const char *func_name = get_func_name(pc);
  
  // 按调用深度打印缩进
  printf("0x%08x:", pc);
  for (int i = 0; i < call_depth; i++) printf("  ");
  printf(" ret  [%s]\n", func_name);
}
