#ifndef REALWAKKA_CURSOR_H_
#define REALWAKKA_CURSOR_H_

namespace wm {

class DrmInfo;

class Cursor
{
 public:
  Cursor();

  void SetX(int x) { x_ = x; }
  int GetX() const { return x_; }

  void SetY(int y) { y_ = y; }
  int GetY() const { return y_; }

  void Paint(DrmInfo& drm);
  

 private:
  int x_;
  int y_;
};


}  // wm

#endif /* REALWAKKA_CURSOR_H_ */
