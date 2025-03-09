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

#include "sdb.h"
#include <memory/vaddr.h>

#define NR_WP 32
#define MAX_EXPR_LEN 128

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  /* 监视点需要的额外字段 */
  char expr[MAX_EXPR_LEN]; // 存储表达式字符串
  word_t old_val;          // 存储表达式上一次的值
  bool active;             // 是否激活
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

/* 初始化监视点池，将所有监视点放入空闲链表 */
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].active = false;
    wp_pool[i].expr[0] = '\0';
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
/* 申请一个新的监视点 */
WP* new_wp() {
  if (free_ == NULL) {
    printf("Error: No free watchpoint available\n");
    assert(0);
    return NULL;
  }

  /* 从空闲链表获取一个监视点 */
  WP *wp = free_;
  free_ = free_->next;

  /* 将监视点加入使用中链表 */
  wp->next = head;
  head = wp;
  wp->active = true;

  return wp;
}

/* 释放一个监视点，将其归还到空闲链表 */
void free_wp(WP *wp) {
  if (wp == NULL) return;

  /* 从使用中链表溢出 */
  if (wp == head) {
    head = head->next;
  } else {
    WP *prev = head;
    while (prev && prev->next != wp) {
      prev = prev->next;
    }
    if (prev) {
      prev->next = wp->next;
    }
  }
  /* 重置监视点状态 */
  wp->active = false;
  wp->expr[0] = '\0';

  /* 归还至空闲链表 */
  wp->next = free_;
  free_ = wp;
}

/* 根据编号查找监视点 */
WP* find_wp(int NO) {
  WP *p = head;
  while (p != NULL) {
    if (p->NO == NO) {
      return p;
    }
    p = p->next;
  }
  return NULL;
}

/* 设置监视点，返回监视点编号 */
int set_watchpoint(char *e) {
  bool success = true;
  word_t val = expr(e, &success);
  if (!success) {
    printf("Failed to set watchpoint: invalid expression\n");
    return -1;
  }

  WP *wp = new_wp();
  if (wp == NULL) return -1;

  /* 拷贝表达式字符串（确保不会溢出）*/
  strncpy(wp->expr, e, MAX_EXPR_LEN - 1);
  wp->expr[MAX_EXPR_LEN - 1] = '\0';

  /* 存储初始值 */
  wp->old_val = val;

  printf("watchpoint %d: %s\n", wp->NO, wp->expr);
  return wp->NO;
}

/* 删除监视点 */
bool delete_watchpoint(int NO) {
  WP *wp = find_wp(NO);
  if (wp == NULL) {
    printf("No watchpoint number %d\n", NO);
    return false;
  }

  free_wp(wp);
  printf("Deleted watchpoint %d\n", NO);
  return true;
}

/* 显示所有监视点信息 */
void list_watchpoints() {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tType\tExpr\tValue\n");
  WP *p = head;
  while (p != NULL) {
    printf("%d\twatchpoint\t%s\t" FMT_WORD "\n", p->NO, p->expr, p->old_val);
    p = p->next;
  }
}

/* 检查所有监视点是否发生变化 */
bool check_watchpoints() {
  WP *p = head;
  bool changed = false;

  while (p != NULL) {
    bool success = true;
    word_t new_val = expr(p->expr, &success);

    if (success && new_val != p->old_val) {
      printf("\nwatchpoint %d: %s\n", p->NO, p->expr);
      printf("Old value = " FMT_WORD "\n", p->old_val);
      printf("New_value = " FMT_WORD "\n", new_val);
      p->old_val = new_val;
      changed = true;
    }

    p = p->next;
  }

  return changed;
}


