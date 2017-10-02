#ifndef REALWAKKA_SESSION_H_
#define REALWAKKA_SESSION_H_

#include <linux/input.h>
#include <boost/asio.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "message/message.h"

namespace wm {

struct DrmInfo;
class Server;
class BufferInfo;

class WidgetInfo
{
 public:
  int x_;
  int y_;
  int width_;
  int height_;
  //std::unique_ptr<uint32_t> bitmap_;
  void* shm_;
};

class Session
{
 public:
  Session(boost::asio::io_service& io_service, Server& server);
  virtual ~Session() {}

  boost::asio::ip::tcp::socket& GetSocket() { return socket_; }
  void SetSocket(boost::asio::ip::tcp::socket&& socket) { socket_ = std::move(socket); }
  void Run();
  void HandleRead(const boost::system::error_code& error);
  void HandleWrite(const boost::system::error_code& error);
  void OnRead(Message& msg);

  void Paint(DrmInfo& drm_info);
  void Paint(BufferInfo& buffer);
  void OnKey(input_event& ev);

  std::string GetUuid() const { return uuid_; }
  //const WidgetInfo& GetWidgetInfo() const { return WidgetInfo; }
  

 private:
  boost::asio::io_service& io_service_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::deadline_timer timer_;
  Message read_msg_;
  WidgetInfo widget_info_;
  std::string uuid_;
  boost::interprocess::shared_memory_object shm_obj_;
  Server& server_;
  
};


}  // wm

#endif /* REALWAKKA_SESSION_H_ */
