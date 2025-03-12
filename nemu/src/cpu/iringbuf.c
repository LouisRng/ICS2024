#include <cpu/iringbuf.h>
#include <isa.h>

/* 添加 disassemble 函数的前向声明 */
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

/* 环形缓冲区和相关索引 */
static iringbuf_entry_t iringbuf[IRINGBUF_SIZE];
static int iringbuf_head = 0; // 下一个要写入的位置
static bool iringbuf_full = false;

/* 初始化环形缓冲区 */
void init_iringbuf() {
  iringbuf_head = 0;
  iringbuf_full = false;
  memset(iringbuf, 0, sizeof(iringbuf));
}

/* 记录指令到环形缓冲区 */
void iringbuf_record(vaddr_t pc, uint32_t inst) {
  /* 存储PC和指令 */
  iringbuf[iringbuf_head].pc = pc;
  iringbuf[iringbuf_head].inst = inst;

  /* 反汇编指令 */
  disassemble(iringbuf[iringbuf_head].disasm,
              sizeof(iringbuf[iringbuf_head].disasm),
              pc,
              (uint8_t*)&inst,
              4); // 假设所有指令都是4字节
  /* 更新环形缓冲区头指针 */
  iringbuf_head = (iringbuf_head + 1) % IRINGBUF_SIZE;
  if (iringbuf_head == 0) {
    iringbuf_full = true;
  }
}

/* 打印环形缓冲区内容 */
void iringbuf_display(vaddr_t current_pc) {
  // 确定起始位置和有效数据数量
  int count;
  if (iringbuf_full) {
    count = IRINGBUF_SIZE;
  } else {
    count = iringbuf_head;
  }
  
  // 如果没有数据要显示，直接返回
  if (count == 0) {
    printf("\nInstruction Ring Buffer is empty\n");
    return;
  }

  // 创建临时数组存储缓冲区内容
  iringbuf_entry_t sorted[IRINGBUF_SIZE];
  
  // 复制环形缓冲区内容到临时数组
  if (iringbuf_full) {
    // 缓冲区已满，从head开始复制环形数组的内容
    for (int i = 0; i < count; i++) {
      sorted[i] = iringbuf[(iringbuf_head + i) % IRINGBUF_SIZE];
    }
  } else {
    // 缓冲区未满，从0开始复制
    for (int i = 0; i < count; i++) {
      sorted[i] = iringbuf[i];
    }
  }
  
  // 对临时数组按PC地址排序（使用简单的冒泡排序）
  for (int i = 0; i < count - 1; i++) {
    for (int j = 0; j < count - i - 1; j++) {
      if (sorted[j].pc > sorted[j + 1].pc) {
        iringbuf_entry_t temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  
  // 显示排序后的结果
  printf("\n============= Instruction Ring Buffer ==============\n");
  
  for (int i = 0; i < count; i++) {
    bool is_current = (sorted[i].pc == current_pc);
    uint8_t *inst_bytes = (uint8_t *)&sorted[i].inst;
    
    // 解析反汇编结果
    char opcode[16] = {0};
    char operands[32] = {0};
    sscanf(sorted[i].disasm, "%15s %31[^\n]", opcode, operands);
    
    // 使用完全固定宽度的格式
    printf("%s0x%08x: %-8s %-16s %02x %02x %02x %02x\n", 
           is_current ? "--> " : "    ",
           sorted[i].pc, 
           opcode,            
           operands,          
           inst_bytes[0], inst_bytes[1], inst_bytes[2], inst_bytes[3]);
  }
  
  printf("====================================================\n\n");
}
