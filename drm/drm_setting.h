#ifndef REALWAKKA_KERNEL_MODE_SETTING_H_
#define REALWAKKA_KERNEL_MODE_SETTING_H_

#include <memory>
#include <vector>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace wm {

class DRMBuffer
{
 public:
  DRMBuffer() : width_(0), height_(0), stride_(0), size_(0), handle_(0), map_(nullptr), fb_(0) {}
  
  uint32_t width_;
  uint32_t height_;
  uint32_t stride_;
  uint32_t size_;
  uint32_t handle_;
  uint8_t* map_;
  uint32_t fb_;
};

class DRMDevice
{
 public:
  DRMDevice() : front_buffer_(0), conn_(0), crtc_(0) {}
  uint32_t front_buffer_;
  DRMBuffer buffer_[2];
  
  drmModeModeInfo mode_;
  uint32_t conn_;
  uint32_t crtc_;
  drmModeCrtc* saved_crtc_;
};
class DirectRenderingManager 
{
 public:
  DirectRenderingManager();
  void Prepare(); 
  int SetupDevice(int fd, drmModeRes* res, drmModeConnector* conn, std::shared_ptr<DRMDevice> device);
  int FindCRTC(int fd, drmModeRes* res, drmModeConnector* conn, std::shared_ptr<DRMDevice> device);
  int CreateFB(int fd, DRMBuffer* buf);

 private:
  int fd_;
  std::vector<std::shared_ptr<DRMDevice>> device_list_;

};

}

#endif

