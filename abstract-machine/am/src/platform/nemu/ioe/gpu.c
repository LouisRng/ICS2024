#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  int i;
  uint32_t screen_info = inl(VGACTL_ADDR);
  int w = (screen_info >> 16) & 0xffff;  // 从VGA控制器获取宽度
  int h = screen_info & 0xffff;          // 从VGA控制器获取高度
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i++) fb[i] = i;
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t screen_info = inl(VGACTL_ADDR);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = (screen_info >> 16) & 0xffff,
    .height = screen_info & 0xffff,
    .vmemsz = ((screen_info >> 16) & 0xffff) * (screen_info & 0xffff) * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  uint32_t *pixels = ctl->pixels;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t screen_info = inl(VGACTL_ADDR);
  uint32_t screen_w = (screen_info >> 16) & 0xffff;
  
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if ((x + i) < screen_w && (y + j) < (screen_info & 0xffff)) {
        fb[(y + j) * screen_w + (x + i)] = pixels[j * w + i];
      }
    }
  }

  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
