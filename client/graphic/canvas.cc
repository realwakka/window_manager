#include "client/graphic/canvas.h"

#include <algorithm>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace wm {

SharedMemoryCanvasDelegate::SharedMemoryCanvasDelegate(const std::string& path)
    : shm_obj_(boost::interprocess::open_only, path.c_str() ,boost::interprocess::read_write)
{
  
}
SharedMemoryCanvasDelegate::~SharedMemoryCanvasDelegate()
{
}

void SharedMemoryCanvasDelegate::SetColor(int color)
{
  boost::interprocess::mapped_region region(shm_obj_, boost::interprocess::read_write);
  reinterpret_cast<int*>(region.get_address());
  for(int i=0 ; i< region.get_size() / sizeof(int) ; ++i ) {
    reinterpret_cast<int*>(region.get_address())[i] = color;
  }
  // std::fill_n(reinterpret_cast<int*>(region.get_address())
  //             , color, region.get_size() / sizeof(int));
}


Canvas::Canvas()
{}
  

void Canvas::SetColor(int color)
{
  delegate_->SetColor(color);
}

Canvas CreateCanvasSharedMemory(const std::string& path)
{
  Canvas canvas;
  canvas.SetDelegate<SharedMemoryCanvasDelegate>(path);
  return canvas;
}


}  // wm
