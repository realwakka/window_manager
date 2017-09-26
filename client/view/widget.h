#include "client/graphic/canvas.h"

namespace wm {


class Widget
{
 public:
  Widget()
      : x_(100),
        y_(100),
        width_(100),
        height_(100),
        bitmap_(new uint32_t[width_ * height_])
  {}

  void AllocBitmap() { bitmap_.reset(new uint32_t[width_ * height_]); }
  void OnPaint(Canvas& canvas)
  {
    int col=0x00ff00ff;
    canvas.SetColor(col);
  }

  int x_;
  int y_;
  int width_;
  int height_;
  std::unique_ptr<uint32_t> bitmap_;
};


}  // wm

