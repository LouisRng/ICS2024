#include <fs.h>

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;  // 当前文件操作的位置
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* 这里会引入文件表，前三项是标准输入输出 */
static Finfo file_table[] = {
  [FD_STDIN]  = {"stdin", 0, 0, 0},
  [FD_STDOUT] = {"stdout", 0, 0, 0},
  [FD_STDERR] = {"stderr", 0, 0, 0},
  // 后面会通过files.h自动包含其他文件项
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

int fs_open(const char *pathname, int flags, int mode) {
  // 遍历文件表，找到匹配的文件名
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      // 找到文件，重置偏移量并返回文件描述符
      file_table[i].open_offset = 0;
      return i;
    }
  }
  
  // 文件不存在，中止程序
  panic("File %s not found!", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);
  
  // 处理特殊文件
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR) {
    return 0;  // 忽略特殊文件的读取
  }
  
  // 获取文件信息
  Finfo *file = &file_table[fd];
  
  // 计算实际可读取的长度（不能超过文件边界）
  size_t remaining = file->size - file->open_offset;
  size_t read_len = len < remaining ? len : remaining;
  
  if (read_len > 0) {
    // 从ramdisk读取数据
    ramdisk_read(buf, file->disk_offset + file->open_offset, read_len);
    // 更新偏移量
    file->open_offset += read_len;
  }
  
  return read_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);
  
  // 处理特殊文件
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    // 输出到串口
    for (size_t i = 0; i < len; i++) {
      putch(((char *)buf)[i]);
    }
    return len;
  }
  
  if (fd == FD_STDIN) {
    return 0;  // 忽略对stdin的写入
  }
  
  // 获取文件信息
  Finfo *file = &file_table[fd];
  
  // 计算实际可写入的长度（不能超过文件大小）
  size_t remaining = file->size - file->open_offset;
  size_t write_len = len < remaining ? len : remaining;
  
  if (write_len > 0) {
    // 写入数据到ramdisk
    ramdisk_write(buf, file->disk_offset + file->open_offset, write_len);
    // 更新偏移量
    file->open_offset += write_len;
  }
  
  return write_len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);
  
  // 处理特殊文件
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR) {
    return 0;  // 忽略特殊文件的定位
  }
  
  // 获取文件信息
  Finfo *file = &file_table[fd];
  
  // 根据whence参数调整偏移量
  switch (whence) {
    case SEEK_SET: // 从文件开头设置偏移
      file->open_offset = offset;
      break;
    case SEEK_CUR: // 从当前位置设置偏移
      file->open_offset += offset;
      break;
    case SEEK_END: // 从文件末尾设置偏移
      file->open_offset = file->size + offset;
      break;
    default:
      panic("Unsupported whence: %d", whence);
  }
  
  // 确保偏移量不超过文件大小
  if (file->open_offset > file->size) {
    file->open_offset = file->size;
  }
  
  return file->open_offset;
}

int fs_close(int fd) {
  // 检查文件描述符是否有效
  assert(fd >= 0 && fd < NR_FILES);
  // 简单实现，直接返回成功
  return 0;
}

void init_fs() {
  // 初始化文件系统，暂无需额外操作
}
