#ifndef WM_CANVAS_H_
#define WM_CANVAS_H_

#include <memory>
#include <boost/interprocess/shared_memory_object.hpp>

namespace wm {

class CanvasDelegate
{
 public:
  virtual void SetColor(int color) = 0;
};


class SharedMemoryCanvasDelegate : public CanvasDelegate
{
 public:
  SharedMemoryCanvasDelegate(const std::string& path);
  virtual ~SharedMemoryCanvasDelegate();

  void SetColor(int color) override;

 private:
  boost::interprocess::shared_memory_object shm_obj_;
};

class Canvas
{
 public:
  Canvas();
  void SetColor(int color);

  template<typename DelegateType, typename... Args>
  void SetDelegate(Args&&...);

 private:
  std::unique_ptr<CanvasDelegate> delegate_;
};

template<typename T, typename... Args>
void Canvas::SetDelegate(Args&&... args)
{
  return delegate_.reset(new T(std::forward<Args>(args)...));
}

Canvas CreateCanvasSharedMemory(const std::string& path);


}  // wm

#endif /* WM_CANVAS_H_ */
