#include "cursor.h"

#include "server.h"

namespace wm {

Cursor::Cursor() : x_(0), y_(0), delegate_(new NormalCursorDelegate) {}

void Cursor::Paint(BufferInfo& buffer)
{
  delegate_->Paint(*this, buffer);
}

void NormalCursorDelegate::Paint(const Cursor& cursor, BufferInfo& buffer)
{
  int col=0x0;
  for (int y=cursor.GetY();y<10;y++)
    for (int x=cursor.GetX();x<10;x++) {
      int location=y*(buffer.width()) + x;
      *((buffer.buffer())+location)=col;
    }
}
  


}  // wm
