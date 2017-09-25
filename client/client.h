#ifndef REALWAKKA_CLIENT_H_
#define REALWAKKA_CLIENT_H_

#include <boost/asio.hpp>
#include <linux/input.h>
#include "message/message.h"

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
  void OnPaint()
  {
    int col=0x00ff00ff;
    for(int i=0 ; i<width_ * height_; ++i ) {
      bitmap_.get()[i] = col;
    }
  }

  int x_;
  int y_;
  int width_;
  int height_;
  std::unique_ptr<uint32_t> bitmap_;
};

class Client
{
 public:
  Client();
  virtual ~Client();

  void HandleConnect(const boost::system::error_code& error);
  void Run();

  void OnReadMessage(Message& message);
  void OnWriteMessage(boost::system::error_code err);
  void OnPaint();
  void OnKeyEvent(input_event& event);

  void SendInitMessage();
  void SendSchedulePaint();

 private:
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::io_service::work work_;
  Message read_msg_;

  Widget widget_;
  
};


}  // wm

#endif /* REALWAKKA_CLIENT_H_ */
