#include "display_manager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>


namespace wm {

std::shared_ptr<Framebuffer> Framebuffer::Create(const std::string& path)
{
  auto framebuffer = std::shared_ptr<Framebuffer>(new Framebuffer(path));
  framebuffer->Init();
  return framebuffer;
}

void Framebuffer::Init()
{
  fd_ = open(path_.c_str() ,O_RDWR);
  if( fd_ > 0 ) {
    ioctl(fd_, FBIOGET_VSCREENINFO, &finfo_);
    ioctl(fd_, FBIOGET_FSCREENINFO, &vinfo_);

    //auto screensize = vinfo.yres_virtual * finfo.line_length;
    auto screensize = vinfo_.xres * vinfo_.yres * vinfo_.bits_per_pixel / 8;
    fbp_ = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, (off_t)0);
  }
}

Framebuffer::Framebuffer(const std::string& path)
    : path_(path),
      fd_(0),
      x_(0),
      y_(0),
      fbp_(nullptr)
{

}

Framebuffer::~Framebuffer()
{
}


DisplayManager::DisplayManager()
    : tty_fd_(0)
{
  tty_fd_ = open("/dev/tty0", O_RDWR);
  ioctl(tty_fd_,KDSETMODE,KD_GRAPHICS);
  
  boost::filesystem::path p ("/dev/");

  if(boost::filesystem::exists(p)) {
    
    boost::filesystem::directory_iterator end;
    boost::filesystem::directory_iterator begin(p);

    std::for_each(begin, end, [this] ( auto&& itr ) {
        auto path_str = itr.path().string();
        if( path_str.length() > 2 && path_str.substr(2) == "fb" ) {
          auto fb = Framebuffer::Create(path_str);
          if( fb ) 
            framebuffer_list_.emplace_back(fb);
        }
      });
  }


  int maxwidth = 0;
  int maxheight = 0;
  for( auto&& framebuffer : framebuffer_list_ ) {
    maxwidth = std::max(framebuffer.GetWidth(), maxwidth);
    maxheight = std::max(framebuffer.GetHeight(), maxheight);
  }

  buffer_.reset(new BufferInfo(maxwidth, maxheight));
  
}

DisplayManager::~DisplayManager()
{
  ioctl(tty_fd_,KDSETMODE,KD_TEXT);
}

}  // wm
