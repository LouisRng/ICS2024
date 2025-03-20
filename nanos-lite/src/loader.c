#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

// 从ramdisk中`offset`偏移处的`len`字节读入到`buf`中
size_t ramdisk_read(void *buf, size_t offset, size_t len);

// 把`buf`中的`len`字节写入到ramdisk中`offset`偏移处
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

// 返回ramdisk的大小, 单位为字节
size_t get_ramdisk_size();

static uintptr_t loader(PCB *pcb, const char *filename) {
  // 由于目前ramdisk中只有一个文件，所以忽略filename参数
  
  // 读取ELF头部信息
  Elf_Ehdr elf_header;
  ramdisk_read(&elf_header, 0, sizeof(Elf_Ehdr));
  
  // 检查ELF魔数
  if (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
    panic("Not a valid ELF file!");
  }
  
  // 获取程序入口点地址
  uintptr_t entry = elf_header.e_entry;
  
  // 读取程序头表
  int ph_count = elf_header.e_phnum;  // 程序头表项数量
  int ph_size = elf_header.e_phentsize;  // 每个表项大小
  int ph_offset = elf_header.e_phoff;  // 程序头表偏移
  
  // 遍历程序头表
  Elf_Phdr ph;
  for (int i = 0; i < ph_count; i++) {
    // 读取一个程序头表项
    ramdisk_read(&ph, ph_offset + i * ph_size, ph_size);
    
    // 判断是否为可加载段
    if (ph.p_type == PT_LOAD) {
      // 将段内容加载到内存
      ramdisk_read((void *)ph.p_vaddr, ph.p_offset, ph.p_filesz);
      
      // 如果内存大小大于文件大小，将多余部分清零
      if (ph.p_memsz > ph.p_filesz) {
        memset((void *)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
      }
    }
  }
  
  return entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

