#ifndef REALWAKKA_DISPLAY_MANAGER_H_
#define REALWAKKA_DISPLAY_MANAGER_H_

#include <linux/fb.h>

#include <string>
#include <vector>
#include <memory>

namespace wm {

class Framebuffer
{
 public:
  static std::shared_ptr<Framebuffer> Create(const std::string& path);
  ~Framebuffer();

  void SetColor(int x, int y);

 private:
  Framebuffer(const std::string& path);
  
  void Init();
  
 private:
  std::string path_;
  int fd_;
  void* fbp_;

  fb_fix_screeninfo finfo_;
  fb_var_screeninfo vinfo_;
  
};


class DisplayManager
{
 public:
  DisplayManager();
  ~DisplayManager();
  
 private:
  std::vector<std::shared_ptr<Framebuffer>> framebuffer_list_;
  int tty_fd_;
  
};


}  // wm

#endif /* REALWAKKA_DISPLAY_MANAGER_H_ */
