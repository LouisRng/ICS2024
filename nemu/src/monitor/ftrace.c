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

#include "ftrace.h"
#include <stdlib.h>
#include <string.h>

// Global variables
static FuncInfo *func_table = NULL;   // Function symbol table
static int func_count = 0;            // Number of functions
static CallStackEntry call_stack[MAX_CALL_DEPTH];  // Call stack
static int call_depth = 0;            // Current call depth

// Initialize function tracer
void init_ftrace(const char *elf_file) {
  if (!elf_file || strlen(elf_file) == 0) {
    Log("No ELF file provided for ftrace, function names will not be available");
    return;
  }

  FILE *fp = fopen(elf_file, "rb");
  if (!fp) {
    Log("Failed to open ELF file '%s'", elf_file);
    return;
  }

  // Read ELF header
  Elf32_Ehdr elf_header;
  if (fread(&elf_header, sizeof(elf_header), 1, fp) != 1) {
    Log("Failed to read ELF header");
    fclose(fp);
    return;
  }

  // Verify ELF magic number
  if (memcmp(elf_header.e_ident, "\x7F" "ELF", 4) != 0) {
    Log("Invalid ELF file format");
    fclose(fp);
    return;
  }

  // Allocate memory for section headers
  Elf32_Shdr *section_headers = malloc(elf_header.e_shnum * elf_header.e_shentsize);
  if (!section_headers) {
    Log("Failed to allocate memory for section headers");
    fclose(fp);
    return;
  }

  // Read section headers
  fseek(fp, elf_header.e_shoff, SEEK_SET);
  if (fread(section_headers, elf_header.e_shentsize, elf_header.e_shnum, fp) != elf_header.e_shnum) {
    Log("Failed to read section headers");
    free(section_headers);
    fclose(fp);
    return;
  }

  // Read section header string table
  Elf32_Shdr *shstrtab_header = &section_headers[elf_header.e_shstrndx];
  char *shstrtab = malloc(shstrtab_header->sh_size);
  if (!shstrtab) {
    Log("Failed to allocate memory for section header string table");
    free(section_headers);
    fclose(fp);
    return;
  }

  fseek(fp, shstrtab_header->sh_offset, SEEK_SET);
  if (fread(shstrtab, shstrtab_header->sh_size, 1, fp) != 1) {
    Log("Failed to read section header string table");
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  // Find symbol table and string table sections
  Elf32_Shdr *symtab_header = NULL;
  Elf32_Shdr *strtab_header = NULL;

  for (int i = 0; i < elf_header.e_shnum; i++) {
    char *section_name = shstrtab + section_headers[i].sh_name;
    
    if (section_headers[i].sh_type == SHT_SYMTAB) {
      symtab_header = &section_headers[i];
    } else if (section_headers[i].sh_type == SHT_STRTAB && 
               strcmp(section_name, ".strtab") == 0) {
      strtab_header = &section_headers[i];
    }
  }

  if (!symtab_header || !strtab_header) {
    Log("Symbol table or string table not found in ELF file");
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  // Read string table
  char *strtab = malloc(strtab_header->sh_size);
  if (!strtab) {
    Log("Failed to allocate memory for string table");
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  fseek(fp, strtab_header->sh_offset, SEEK_SET);
  if (fread(strtab, strtab_header->sh_size, 1, fp) != 1) {
    Log("Failed to read string table");
    free(strtab);
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  // Read symbol table and extract functions
  int sym_count = symtab_header->sh_size / sizeof(Elf32_Sym);
  Elf32_Sym *symbols = malloc(symtab_header->sh_size);
  if (!symbols) {
    Log("Failed to allocate memory for symbol table");
    free(strtab);
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  fseek(fp, symtab_header->sh_offset, SEEK_SET);
  if (fread(symbols, symtab_header->sh_size, 1, fp) != 1) {
    Log("Failed to read symbol table");
    free(symbols);
    free(strtab);
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  // Allocate function table
  func_table = malloc(MAX_FUNC_COUNT * sizeof(FuncInfo));
  if (!func_table) {
    Log("Failed to allocate memory for function table");
    free(symbols);
    free(strtab);
    free(shstrtab);
    free(section_headers);
    fclose(fp);
    return;
  }

  // Extract function symbols
  func_count = 0;
  for (int i = 0; i < sym_count && func_count < MAX_FUNC_COUNT; i++) {
    if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC && symbols[i].st_size > 0) {
      func_table[func_count].name = strdup(strtab + symbols[i].st_name);
      func_table[func_count].start = symbols[i].st_value;
      func_table[func_count].size = symbols[i].st_size;
      func_count++;
    }
  }

  Log("Loaded %d function symbols from ELF file", func_count);

  // Clean up resources
  free(symbols);
  free(strtab);
  free(shstrtab);
  free(section_headers);
  fclose(fp);
}

// Get function name from address
const char* ftrace_func_name(paddr_t addr) {
  for (int i = 0; i < func_count; i++) {
    if (addr >= func_table[i].start && 
        addr < func_table[i].start + func_table[i].size) {
      return func_table[i].name;
    }
  }
  return "???";
}

// Record function call
void ftrace_call(paddr_t pc, paddr_t target) {
  extern FILE* log_fp;
  
  // Ensure we don't overflow the call stack
  if (call_depth >= MAX_CALL_DEPTH) {
    return;
  }
  
  const char* func_name = ftrace_func_name(target);
  
  // Print call trace with indentation
  fprintf(log_fp, "%*s" FMT_PADDR ": call [%s@" FMT_PADDR "]\n", 
          call_depth * 2, "", pc, func_name, target);
  
  // Record the call in our stack
  call_stack[call_depth].caller_pc = pc;
  call_stack[call_depth].callee_addr = target;
  call_depth++;
}

// Record function return
void ftrace_ret(paddr_t pc, paddr_t ret_addr) {
  extern FILE* log_fp;
  
  if (call_depth <= 0) {
    return;  // Stack underflow
  }
  
  call_depth--;
  const char* func_name = ftrace_func_name(call_stack[call_depth].callee_addr);
  
  fprintf(log_fp, "%*s" FMT_PADDR ": ret  [%s]\n", 
          call_depth * 2, "", pc, func_name);
}

// Clean up resources
void ftrace_cleanup(void) {
  if (func_table) {
    for (int i = 0; i < func_count; i++) {
      if (func_table[i].name) {
        free(func_table[i].name);
      }
    }
    free(func_table);
    func_table = NULL;
  }
  func_count = 0;
  call_depth = 0;
}
