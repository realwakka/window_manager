#include "client.h"

#include <iostream>
#include <boost/bind.hpp>

namespace wm {

Client::Client() : socket_(io_service_), work_(io_service_)
{
  boost::asio::ip::tcp::resolver resolver(io_service_);
  auto iterator = resolver.resolve({ "127.0.0.1", "1234" });
  boost::asio::async_connect(socket_, iterator, boost::bind(&Client::HandleConnect, this, boost::asio::placeholders::error));
}

Client::~Client()
{
}

void Client::HandleConnect(const boost::system::error_code& error)
{
  message::ReadMessage(socket_, read_msg_,
                       std::bind(&Client::OnReadMessage, this, std::placeholders::_1), [](boost::system::error_code err) { });
  SendInitMessage();

}

void Client::SendInitMessage()
{
  auto init_msg = new InitWidgetMessage();
  init_msg->x_ = widget_.x_;
  init_msg->y_ = widget_.y_;
  init_msg->width_ = widget_.width_;
  init_msg->height_ = widget_.height_;

  Message msg;
  msg.GetHeader()->data_type_ = MessageType::InitWidgetMessage;
  msg.GetHeader()->data_size_ = sizeof(InitWidgetMessage);
  msg.SetData(reinterpret_cast<char*>(init_msg));
  
  message::WriteMessage(socket_, msg, std::bind(&Client::OnWriteMessage, this, std::placeholders::_1));

}

void Client::OnWriteMessage(boost::system::error_code err)
{
  if( err ) {
    std::cout << "write err" << err << std::endl;
  }
}

void Client::OnReadMessage(Message& message)
{
  if( message.GetHeader()->data_type_ == MessageType::PaintRequest ) {
    OnPaint();
  }
  else if( message.GetHeader()->data_type_ == MessageType::Keyboard ) {
    auto ev = reinterpret_cast<input_event*>(message.GetData());
    OnKeyEvent(*ev);
  }
  
  message::ReadMessage(socket_, read_msg_,
                       std::bind(&Client::OnReadMessage, this, std::placeholders::_1), [](boost::system::error_code err) {});  
}

void Client::Run()
{
  io_service_.run();
}

void Client::OnPaint()
{
  std::cout << "received paint req" << std::endl;

  widget_.OnPaint();
  
  Message msg;
  msg.GetHeader()->data_type_ = MessageType::PaintResponse;
  msg.GetHeader()->data_size_ = widget_.width_ * widget_.height_ * sizeof(uint32_t);
  msg.SetData(reinterpret_cast<char*>(widget_.bitmap_.release()));

  widget_.AllocBitmap();

  message::WriteMessage(socket_, msg, std::bind(&Client::OnWriteMessage, this, std::placeholders::_1));
}

void Client::OnKeyEvent(input_event& event)
{
  std::cout << "key event!!!!" << std::endl;
  std::cout << event.type << std::endl;
  std::cout << event.code << std::endl;
  std::cout << event.value << std::endl;

  if( event.value == 1 ) {
    if( event.code == KEY_UP ) {
      --widget_.y_;
      SendInitMessage();
    } else if ( event.code == KEY_DOWN ) {
      ++widget_.y_;
      SendInitMessage();
    } else if ( event.code == KEY_LEFT ) {
      --widget_.x_;
      SendInitMessage();
    } else if ( event.code == KEY_RIGHT ) {
      ++widget_.x_;
      SendInitMessage();
    } 
  }
}


}  // wm

