#include <proc.h>
#include <elf.h>

/* ELF文件机器类型检查 不包括龙芯 */
#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64  // 在native环境中通常是x86_64
#elif defined(__ISA_X86__)
# define EXPECT_TYPE EM_386     // x86 32位架构
#elif defined(__ISA_MIPS32__)
# define EXPECT_TYPE EM_MIPS    // MIPS架构
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV   // RISC-V架构
#else
# error Unsupported ISA
#endif

// 从ramdisk中`offset`偏移处的`len`字节读入到`buf`中
size_t ramdisk_read(void *buf, size_t offset, size_t len);

// 把`buf`中的`len`字节写入到ramdisk中`offset`偏移处
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

// 返回ramdisk的大小, 单位为字节
size_t get_ramdisk_size();

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  // 读取ELF文件头
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
  
  // 检查ELF文件头是否有效（魔数检查）
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    panic("Invalid ELF file");
  }
  
  // 加载程序段
  uint32_t ph_offset = ehdr.e_phoff;
  uint32_t ph_count = ehdr.e_phnum;
  
  Log("加载ELF文件，包含 %d 个程序头部", ph_count);
  
  for (int i = 0; i < ph_count; i++) {
    Elf_Phdr phdr;
    ramdisk_read(&phdr, ph_offset + i * sizeof(Elf_Phdr), sizeof(Elf_Phdr));
    
    // 只加载类型为PT_LOAD的段
    if (phdr.p_type == PT_LOAD) {
      Log("加载段 %d: 偏移 = 0x%x, 虚拟地址 = 0x%x, 文件大小 = 0x%x, 内存大小 = 0x%x", 
          i, phdr.p_offset, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz);
      
      // 从ELF文件中读取段内容到内存
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      
      // 如果内存大小大于文件大小，则将多余部分清零
      if (phdr.p_memsz > phdr.p_filesz) {
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
      }
    }
  }
  
  // 返回程序入口地址
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}
