#include <am.h>
#include <nemu.h>
#include <klib.h>  // 如果需要printf

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t key_code = inl(KBD_ADDR);
  
  // 添加调试输出，查看是否能获取到键盘码
  /* printf("Debug: Read key_code = 0x%08x from KBD_ADDR\n", key_code); */
  
  if (key_code != AM_KEY_NONE) {
    kbd->keydown = (key_code & KEYDOWN_MASK) ? true : false;
    kbd->keycode = key_code & ~KEYDOWN_MASK;
  } else {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
}
