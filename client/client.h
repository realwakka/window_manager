#ifndef REALWAKKA_CLIENT_H_
#define REALWAKKA_CLIENT_H_

#include <boost/asio.hpp>
#include <linux/input.h>

#include "graphic/canvas.h"
#include "view/widget.h"
#include "message/message.h"

namespace wm {

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
