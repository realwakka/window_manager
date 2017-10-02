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

void Framebuffer::WriteBuffer(const BufferInfo& buffer_info)
{
  if( vinfo_.bits_per_pixel == 16 ) {
    auto buffer = buffer_info.buffer();
    for( auto x = 0 ; x < vinfo_.xres ; ++x ) {
      for( auto y = 0 ; x < vinfo_.yres ; ++x ) {
        auto color = buffer[y * buffer_info.width() + x];
        auto a = ((uint8_t*)&color)[0];
        auto r = ((uint8_t*)&color)[1];
        auto g = ((uint8_t*)&color)[2];
        auto b = ((uint8_t*)&color)[3];
        uint16_t t = r<<11 | g << 5 | b;
        auto location = (x+vinfo_.xoffset) * (vinfo_.bits_per_pixel/8) +
                        (y+vinfo_.yoffset) * finfo_.line_length;

        *((uint16_t*)(fbp_ + location)) = t;
      }
    }
  }
}


DisplayManager::DisplayManager()
    : tty_fd_(0)
{
  tty_fd_ = open("/dev/tty0", O_RDWR);
  ioctl(tty_fd_,KDSETMODE,KD_GRAPHICS);
  
  boost::filesystem::path p("/dev");

  if(boost::filesystem::exists(p)) {
    
    boost::filesystem::directory_iterator end;
    boost::filesystem::directory_iterator begin(p);

    std::for_each(begin, end, [this] ( auto&& itr ) {
        auto path_str = itr.path().string();
        auto filename = itr.path().filename().string();
        
        if( filename.length() > 2 && filename.substr(0, 2) == "fb" ) {
          auto fb = Framebuffer::Create(path_str);
          if( fb ) 
            framebuffer_list_.emplace_back(fb);

        }
      });
  }


  int maxwidth = 0;
  int maxheight = 0;
  for( auto&& framebuffer : framebuffer_list_ ) {
    maxwidth = std::max(framebuffer->GetWidth(), maxwidth);
    maxheight = std::max(framebuffer->GetHeight(), maxheight);
  }

  buffer_.reset(new BufferInfo(maxwidth, maxheight));
  
}

DisplayManager::~DisplayManager()
{
  ioctl(tty_fd_,KDSETMODE,KD_TEXT);
}

void DisplayManager::SwapBuffer()
{
  for( auto framebuffer : framebuffer_list_ ) {
    framebuffer->WriteBuffer(*buffer_);
  }
}

}  // wm
