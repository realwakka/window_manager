#include "session.h"

#include <iostream>

#include <boost/bind.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <boost/interprocess/mapped_region.hpp>

#include "server.h"

namespace wm {

Session::Session(boost::asio::io_service& io_service, Server& server)
    : io_service_(io_service),
      socket_(io_service),
      timer_(io_service, boost::posix_time::seconds(1)),
      uuid_(boost::uuids::to_string(boost::uuids::random_generator()())),
      server_(server),
      shm_obj_( boost::interprocess::create_only, uuid_.c_str(), boost::interprocess::read_write )
{
}


void Session::Run()
{
  message::ReadMessage(socket_, read_msg_,
                       std::bind(&Session::OnRead, this, std::placeholders::_1),
                       [](boost::system::error_code err) { });
}

void Session::OnRead(Message& msg)
{
  if( msg.GetHeader()->data_type_ == MessageType::InitWidgetMessage ) {
    std::cout << "received init msg" << std::endl;
    auto message = reinterpret_cast<InitWidgetMessage*>(msg.GetData());
    widget_info_.x_ = message->x_;
    widget_info_.y_ = message->y_;
    widget_info_.width_ = message->width_;
    widget_info_.height_ = message->height_;

    // widget_info_.bitmap_.reset(new uint32_t[widget_info_.width_ * widget_info_.height_]);
    shm_obj_.truncate(sizeof(int) * message->width_ * message->height_);
    
  } else if( msg.GetHeader()->data_type_ == MessageType::PaintResponse ) {
  
    std::cout << "received paint response" << std::endl;
    server_.Paint();
    
    // auto data = msg.MoveData();
    // widget_info_.bitmap_.reset(reinterpret_cast<uint32_t*>(data.release()));
  } else if( msg.GetHeader()->data_type_ == MessageType::SchedulePaint ) {
    Message msg;
    msg.GetHeader()->data_type_ = MessageType::PaintRequest;
    msg.GetHeader()->data_size_ = uuid_.length() + 1;

    msg.AllocData();
    uuid_.copy(msg.GetData(), uuid_.length() + 1);

    message::WriteMessage(socket_, msg, std::bind(&Session::HandleWrite, this, std::placeholders::_1));

  }
  
  message::ReadMessage(socket_, read_msg_, std::bind(&Session::OnRead, this, std::placeholders::_1),[](boost::system::error_code err) { });

}

void Session::HandleRead(const boost::system::error_code& error)
{
  std::cout << std::string(read_msg_.GetData()) << std::endl;
}

void Session::HandleWrite(const boost::system::error_code& error)
{
  //std::cout << "writed" << std::endl;
}
void Session::Paint(BufferInfo& buffer)
{
  boost::interprocess::mapped_region region(shm_obj_, boost::interprocess::read_only);
  auto ptr = reinterpret_cast<uint32_t*>(region.get_address());
  if( ptr != nullptr ) {
    for (int y=0 ; y<widget_info_.height_ ;y++)
      for (int x=0 ; x<widget_info_.width_ ; x++) {
        int sx = x+widget_info_.x_;
        int sy = y+widget_info_.y_;
          
        uint32_t col = ptr[y*widget_info_.width_+x];
        int location=sy*(buffer.width()) + sx;
        *((buffer.buffer())+location)=col;
      }
  }

}

void Session::Paint(DrmInfo& drm_info)
{
  //std::cout << "write paint req" << std::endl;
  
  boost::interprocess::mapped_region region(shm_obj_, boost::interprocess::read_only);
  auto ptr = reinterpret_cast<uint32_t*>(region.get_address());
  if( ptr != nullptr ) {
    for (int j=0;j<drm_info.cnt_;j++) {
      for (int y=0 ; y<widget_info_.height_ ;y++)
        for (int x=0 ; x<widget_info_.width_ ; x++) {
          int sx = x+widget_info_.x_;
          int sy = y+widget_info_.y_;
          
          uint32_t col = ptr[y*widget_info_.width_+x];
          int location=sy*(drm_info.fb_w_[j]) + sx;
          *(((uint32_t*)drm_info.fb_base_[j])+location)=col;
        }
    }
  }
}



void Session::OnKey(input_event& ev)
{
  Message msg;
  msg.GetHeader()->data_type_ = MessageType::Keyboard;
  msg.GetHeader()->data_size_ = sizeof(input_event);

  auto data = new input_event();
  data->type = ev.type;
  data->code = ev.code;
  data->value = ev.value;

  msg.SetData(reinterpret_cast<char*>(data));

  message::WriteMessage(socket_, msg, std::bind(&Session::HandleWrite, this, std::placeholders::_1));
}

}  // wm
