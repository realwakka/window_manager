#ifndef REALWAKKA_EVENT_LOOP_SERVER_H_
#define REALWAKKA_EVENT_LOOP_SERVER_H_

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>

#include "session.h"
#include "input_manager.h"
#include <linux/input.h>

namespace wm {

struct DrmInfo
{
  void *fb_base_[10];
  long fb_w_[10];
  long fb_h_[10];
  int cnt_;
};

class Server
{
 public:
  Server();
  ~Server();

  void SetTimer();
  void RepeatWork();

  void Run();
  void Paint();

  void OnKeyEvent(boost::system::error_code err);
  void OnInputEvent(input_event& event);

 private:
  void StartAccept();
  void OnAccept(const auto& err);
  std::vector<std::shared_ptr<Session>> session_list_;
  std::shared_ptr<Session> focused_session_;
  
  boost::asio::io_service io_service_;
  boost::asio::deadline_timer timer_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;

  DrmInfo drm_info_;
  InputManager input_manager_;

  //boost::asio::posix::stream_descriptor key_desc_;
  //input_event input_event_;
};




}  // wm







#endif /* EVENT_LOOP_SERVER_H_ */
