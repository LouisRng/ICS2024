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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

/* 函数追踪 */
#include <cpu/ftrace.h>

/* 环形缓冲区 */
#include <cpu/iringbuf.h>

/* 异常追踪 */
#include <cpu/etrace.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

/* 
 * 补充指令格式:
 * TYPE_I: imm[11:0] rs1(19:15) funct3(14:12) rd(11:7) opcode(6:0)
 * TYPE_U: imm[31:12] rd(11:7) opcode(6:0)
 * TYPE_S: imm[11:5] rs2(24:20) rs1(19:15) funct3(14:12) imm[4:0] opcode(6:0)
 * TYPE_R: funct7(31:25) rs2(24:20) rs1(19:15) funct3(14:12) rd(11:7) opcode(6:0)
 * TYPE_B: imm[12|10:5] rs2(24:20) rs1(19:15) funct3(14:12) imm[4:1|11] opcode(6:0)
 * TYPE_J: imm[20|10:1|11|19:12] rd(11:7) opcode(6:0)
 */

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_R, TYPE_B, TYPE_J,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0) // SEXT 宏用于符号扩展
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | (BITS(i, 7, 7) << 11) | \
                          (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1); } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 19, 12) << 12) | \
                          (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1); } while(0)

/* 根据传入的指令类型 type 进行操作数的译码，译码结果记录到 rd, src1, src2, imm */
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  /* INSTPAT中的第二个字段不参与宏展开，它只是STRLEN(pattern) */
  INSTPAT_START();
  
  /* RV32I Base Instructions */
  
  /* Load instructions */
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  
  /* Store instructions */
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  
  /* Arithmetic and Logical instructions (Register-Immediate) */
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = (int32_t)src1 < (int32_t)imm ? 1 : 0);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = src1 < imm ? 1 : 0);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << (imm & 0x1f));
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> (imm & 0x1f));
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (int32_t)src1 >> (imm & 0x1f));
  
  /* Arithmetic and Logical instructions (Register-Register) */
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << (src2 & 0x1f));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = (int32_t)src1 < (int32_t)src2 ? 1 : 0);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = src1 < src2 ? 1 : 0);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> (src2 & 0x1f));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int32_t)src1 >> (src2 & 0x1f));
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  
  /* Upper-immediate instructions */
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  
  /* Branch instructions */
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc = (src1 == src2) ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s->dnpc = (src1 != src2) ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, s->dnpc = ((int32_t)src1 < (int32_t)src2) ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc = ((int32_t)src1 >= (int32_t)src2) ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc = (src1 < src2) ? s->pc + imm : s->dnpc);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc = (src1 >= src2) ? s->pc + imm : s->dnpc);

  /* CSR instructions */
  INSTPAT("??????? ????? 001 ????? 1110011", csrrw  , I, {
    // 从指令中提取CSR寄存器编号
    uint32_t csr = BITS(s->isa.inst, 31, 20);
    word_t t = 0;
    // 根据CSR寄存器编号进行相应操作
    switch (csr) {
      case 0x305: // mtvec
        t = cpu.csr.mtvec;
        cpu.csr.mtvec = src1;
        break;
      case 0x300: // mstatus
        t = cpu.csr.mstatus;
        cpu.csr.mstatus = src1;
        break;
      case 0x341: // mepc
        t = cpu.csr.mepc;
        cpu.csr.mepc = src1;
        break;
      case 0x342: // mcause
        t = cpu.csr.mcause;
        cpu.csr.mcause = src1;
        break;
      default:
        // 不支持的CSR寄存器
        panic("Unsupported CSR register 0x%x at PC = " FMT_WORD, csr, s->pc);
    }
    // 如果rd不为0，则写入读取的旧值
    if (rd != 0) {
      R(rd) = t;
    }
  });

  /* CSR读取指令 - csrr rd, csr (实际上是csrrs rd, csr, x0的别名) */
  INSTPAT("??????? 00000 010 ????? 1110011", csrrs  , I, {
    uint32_t csr = BITS(s->isa.inst, 31, 20);
    word_t t = 0;
    switch (csr) {
      case 0x305: // mtvec
        t = cpu.csr.mtvec;
        break;
      case 0x300: // mstatus
        t = cpu.csr.mstatus;
        break;
      case 0x341: // mepc
        t = cpu.csr.mepc;
        break;
      case 0x342: // mcause
        t = cpu.csr.mcause;
        break;
      default:
        panic("Unsupported CSR register 0x%x at PC = " FMT_WORD, csr, s->pc);
    }
    if (rd != 0) {
      R(rd) = t;
    }
  });
  
  /* Jump instructions */
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, {
    R(rd) = s->snpc; 
    s->dnpc = s->pc + imm;
    // 如果是函数调用（rd == ra/x1）
    if (rd == 1) {
      IFDEF(CONFIG_FTRACE, ftrace_call(s->pc, s->dnpc));
    }
  });
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, {
    word_t t = s->snpc; 
    s->dnpc = (src1 + imm) & ~1; 
    // 从指令中直接提取rs1字段
    int rs1_val = BITS(s->isa.inst, 19, 15);
    // 如果是函数返回（rd == x0 && rs1 == ra/x1）
    if (rd == 0 && rs1_val == 1) {
      IFDEF(CONFIG_FTRACE, ftrace_ret(s->pc, src1 + imm));
    } else if (rd == 1) { // 如果是函数调用（rd == ra/x1，间接调用）
      IFDEF(CONFIG_FTRACE, ftrace_call(s->pc, src1 + imm));
    }
    R(rd) = t;
  }); 

  /* Machine-mode Return 指令 */
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , N, {
    #ifdef CONFIG_ETRACE
      etrace_return(cpu.csr.mcause, s->pc, cpu.csr.mepc);
    #endif
    s->dnpc = cpu.csr.mepc;  // 从mepc恢复程序计数器
  });

  /* RV32M Extension (Multiplication and Division) */
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = (int32_t)src1 * (int32_t)src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (int64_t)((int32_t)src1) * (int64_t)((int32_t)src2) >> 32);
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R, R(rd) = (int64_t)((int32_t)src1) * (uint64_t)src2 >> 32);
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = (uint64_t)src1 * (uint64_t)src2 >> 32);
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = src2 == 0 ? -1 : (int32_t)src1 / (int32_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src2 == 0 ? -1U : src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = src2 == 0 ? (int32_t)src1 : (int32_t)src1 % (int32_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src2 == 0 ? src1 : src1 % src2);
  
  /* System/Environment call instructions */
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , N, s->dnpc = isa_raise_intr(R(17), s->pc)); // R(17) is a7 containing syscall ID
  
  /* Default instruction handler */
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4); 

  // 获取指令后立即记录
#ifdef CONFIG_ITRACE
  iringbuf_record(s->pc, s->isa.inst);
#endif

  return decode_exec(s);
}
