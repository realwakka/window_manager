#ifndef REALWAKKA_DISPLAY_MANAGER_H_
#define REALWAKKA_DISPLAY_MANAGER_H_

#include <linux/fb.h>

#include <string>
#include <vector>
#include <memory>

namespace wm {

class BufferInfo
{
 public:
  BufferInfo(int width, int height)
      : width_(width),
        height_(height),
        buffer_( new uint32_t[width*height] )
  {

  }

  int width() const { return width_; }
  int height() const { return height_; }
  uint32_t* buffer() { return buffer_; }
  const uint32_t* buffer() const { return buffer_; }
  
 private:
  int width_;
  int height_;
  uint32_t* buffer_;
};


class Framebuffer
{
 public:
  static std::shared_ptr<Framebuffer> Create(const std::string& path);
  ~Framebuffer();

  void SetColor(int x, int y);
  int GetX() const { return x_; }
  int GetY() const { return y_; }

  int GetWidth() const { return vinfo_.xres; }
  int GetHeight() const { return vinfo_.yres; }

  int GetBpp() const { return vinfo_.bits_per_pixel; }

  void WriteBuffer(const BufferInfo& buffer_info);


 private:
  Framebuffer(const std::string& path);
  void Init();
  
 private:
  std::string path_;
  int fd_;
  void* fbp_;

  int x_;
  int y_;

  fb_fix_screeninfo finfo_;
  fb_var_screeninfo vinfo_;
  
};


class DisplayManager
{
 public:
  DisplayManager();
  ~DisplayManager();

  BufferInfo* GetBuffer() { return buffer_.get(); }
  void SwapBuffer();
  
 private:
  std::vector<std::shared_ptr<Framebuffer>> framebuffer_list_;
  int tty_fd_;
  std::unique_ptr<BufferInfo> buffer_;
  
  
};


}  // wm

#endif /* REALWAKKA_DISPLAY_MANAGER_H_ */
