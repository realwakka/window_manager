#include "server.h"

extern "C" {
#include "drm/kms.h"
}

namespace wm {

Server::Server()
    : timer_(io_service_, boost::posix_time::seconds(1)),
      acceptor_(io_service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 1234)),
      socket_(io_service_),
      key_desc_(io_service_)
{
  StartAccept();
  drm_info_.cnt_ = KernelModelSetting(drm_info_.fb_base_,
                                      drm_info_.fb_w_,
                                      drm_info_.fb_h_);

  SetTimer();

  std::string key_path = "/dev/input/event0";

  auto key_fd = open(key_path.c_str(), O_RDONLY);
  key_desc_.assign(key_fd);
  boost::asio::async_read(key_desc_,
                          boost::asio::buffer(&input_event_, sizeof(input_event_)),
                          boost::bind(&Server::OnKeyEvent,
                                      this, boost::asio::placeholders::error));
}

void Server::OnKeyEvent(boost::system::error_code err)
{
  boost::asio::async_read(key_desc_,
                          boost::asio::buffer(&input_event_, sizeof(input_event_)),
                          boost::bind(&Server::OnKeyEvent,
                                      this, boost::asio::placeholders::error));

  if( input_event_.type == 1 ) {
    if( session_list_.empty() == false ) {
      session_list_.front()->OnKey(input_event_);
    }
  
  }
  
}

void Server::Run()
{
  io_service_.run();
}

void Server::Paint()
{
  for (int j=0;j<drm_info_.cnt_;j++) {
    int col=0x00ffffff;
    for (int y=0;y<drm_info_.fb_h_[j];y++)
      for (int x=0;x<drm_info_.fb_w_[j];x++) {
        int location=y*(drm_info_.fb_w_[j]) + x;
        *(((uint32_t*)drm_info_.fb_base_[j])+location)=col;
      }
  }

  for_each(session_list_.begin(), session_list_.end(), [this](auto session) {
      session->Paint(drm_info_);
    });
}

Server::~Server()
{
}


void Server::SetTimer()
{
  timer_.async_wait(std::bind(&Server::RepeatWork, this));
}

void Server::RepeatWork()
{
  Paint();
  timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(1));
  timer_.async_wait(std::bind(&Server::RepeatWork, this));
}

void Server::StartAccept()
{
  acceptor_.async_accept(socket_,
                         [this] (const auto& err) {
                           auto session = std::make_shared<Session>(io_service_);
                           session_list_.emplace_back(session);
                           session->SetSocket(std::move(socket_));
                           session->Run();
                           std::cout << "accepted!!" << std::endl;
                           this->StartAccept();
                         });
  
}


}  // wm
