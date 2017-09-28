#include "cursor.h"

#include "server.h"

namespace wm {

Cursor::Cursor() : x_(0), y_(0) {}

void Cursor::Paint(DrmInfo& drm)
{
  for (int j=0;j<drm.cnt_;j++) {
    int col=0x0;
    for (int y=y_;y<10;y++)
      for (int x=x_;x<10;x++) {
        int location=y*(drm.fb_w_[j]) + x;
        *(((uint32_t*)drm.fb_base_[j])+location)=col;
      }
  }

}
  


}  // wm
