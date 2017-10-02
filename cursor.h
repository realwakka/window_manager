#ifndef REALWAKKA_CURSOR_H_
#define REALWAKKA_CURSOR_H_

#include <memory>
#include "display_manager.h"

namespace wm {

class DrmInfo;
class Cursor;

class CursorDelegate
{
 public:
  //virtual void Paint(const Cursor& cursor, DrmInfo& drm) = 0;
  virtual void Paint(const Cursor& cursor, BufferInfo& buffer) = 0;
};

class NormalCursorDelegate : public CursorDelegate
{
 public:
  //void Paint(const Cursor& cursor, DrmInfo& drm) override;
  void Paint(const Cursor& cursor, BufferInfo& buffer) override;
};

class Cursor
{
 public:
  Cursor();

  void SetX(int x) { x_ = x; }
  int GetX() const { return x_; }

  void SetY(int y) { y_ = y; }
  int GetY() const { return y_; }

  //void Paint(DrmInfo& drm);
  void Paint(BufferInfo& drm);
  

 private:
  int x_;
  int y_;
  std::unique_ptr<CursorDelegate> delegate_;
};

enum class CursorType
{
  kNormal,
};



}  // wm

#endif /* REALWAKKA_CURSOR_H_ */
