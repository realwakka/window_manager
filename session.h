#ifndef REALWAKKA_SESSION_H_
#define REALWAKKA_SESSION_H_

#include <linux/input.h>
#include <boost/asio.hpp>

#include "message/message.h"

namespace wm {

struct DrmInfo;

class WidgetInfo
{
 public:
  int x_;
  int y_;
  int width_;
  int height_;
  std::unique_ptr<uint32_t> bitmap_;
};

class Session
{
 public:
  Session(boost::asio::io_service& io_service)
      : io_service_(io_service),
        socket_(io_service),
        timer_(io_service, boost::posix_time::seconds(1))
  {}
  virtual ~Session() {}

  boost::asio::ip::tcp::socket& GetSocket() { return socket_; }
  void SetSocket(boost::asio::ip::tcp::socket&& socket) { socket_ = std::move(socket); }
  void Run();
  void HandleRead(const boost::system::error_code& error);
  void HandleWrite(const boost::system::error_code& error);
  void OnRead(Message& msg);

  void Paint(DrmInfo& drm_info);
  void OnKey(input_event& ev);

 private:
  boost::asio::io_service& io_service_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::deadline_timer timer_;
  Message read_msg_;
  WidgetInfo widget_info_;
  
};


}  // wm

#endif /* REALWAKKA_SESSION_H_ */
