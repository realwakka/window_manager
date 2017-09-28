#ifndef WM_INPUT_MANAGER_H_
#define WM_INPUT_MANAGER_H_

#include <boost/asio.hpp>
#include <linux/input.h>

namespace wm {

class Server;

class InputDevice
{
 public:
  InputDevice(boost::asio::io_service& io_service, Server& server);
  bool Open(const std::string& path);
  void OnRead(boost::system::error_code err);

 private:
  Server& server_;
  boost::asio::posix::stream_descriptor desc_;
  input_event input_event_;
  
};

class InputManager
{
 public:
  InputManager(boost::asio::io_service& io_service, Server& server);
  virtual ~InputManager();
  
  void OpenInput();

 private:
  Server& server_;
  std::vector<std::shared_ptr<InputDevice>> device_list_;
  boost::asio::io_service& io_service_;
};


}  // wm

#endif /* WM_INPUT_MANAGER_H_ */
