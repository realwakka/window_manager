#ifndef REALWAKKA_EVENTLOOP_MESSAGE_H_
#define REALWAKKA_EVENTLOOP_MESSAGE_H_

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <iostream>

namespace wm {

enum class MessageType
{
  InitWidgetMessage = 1,
  PaintRequest = 2,
  PaintResponse = 3,
  Keyboard = 4,
};

struct MessageHeader
{
  MessageType data_type_;
  int data_size_;
};

struct InitWidgetMessage
{
  int x_;
  int y_;
  int width_;
  int height_;
};

class Message
{
 public:
  static std::size_t HeaderSize() { return sizeof(MessageHeader); }
  
 public:
  Message() {}
  virtual ~Message() {}

  MessageHeader* GetHeader() { return &header_; }
  void AllocData() { data_.reset(new char[header_.data_size_]); }
  char* GetData() { return data_.get(); }
  void SetData(char* data) { data_.reset(data); }

  std::unique_ptr<char> MoveData() { return std::move(data_); }
  std::size_t GetTotalSize() const { return sizeof(MessageHeader) + header_.data_size_; }

 private:
  MessageHeader header_;
  std::unique_ptr<char> data_;
};

namespace message {


inline void OnReadMessageData(Message& msg, std::function<void(Message&)> func,
                              std::function<void(boost::system::error_code)> err_func,
                              const boost::system::error_code& error)
{
  if(error) {
    err_func(error);
  }
  func(msg);
}


inline void OnReadMessageHeader(boost::asio::ip::tcp::socket& socket, Message& msg,
                                std::function<void(Message&)> func,
                                
                                std::function<void(boost::system::error_code)> err_func,
                                const boost::system::error_code& error)
{
  if( error ) {
    std::cout << "ERROR!" << error << std::endl;
    err_func(error);
    return;
  }
  if( msg.GetHeader()->data_size_ > 0 ) {
    msg.AllocData();
  
    boost::asio::async_read(socket,
                            boost::asio::buffer(msg.GetData(), msg.GetHeader()->data_size_),
                            boost::bind(&OnReadMessageData,
                                        std::ref(msg), func, err_func, boost::asio::placeholders::error));
  }
  else {
    func(msg);
  }
  
}

inline void ReadMessage(boost::asio::ip::tcp::socket& socket, Message& msg,
                        std::function<void(Message&)> func,
                        std::function<void(boost::system::error_code)> err_func)
{
  auto b = boost::bind(&OnReadMessageHeader,
                       std::ref(socket),
                       std::ref(msg),
                       func,
                       err_func,
                       boost::asio::placeholders::error);
  boost::asio::async_read(socket,
                          boost::asio::buffer(msg.GetHeader(),
                                              sizeof(MessageHeader)),
                          b);

}

template<typename HandlerType>
void WriteMessage(boost::asio::ip::tcp::socket& socket, Message& msg, HandlerType handler )
{
  boost::asio::async_write(socket,
                           boost::asio::buffer(msg.GetHeader(), sizeof(MessageHeader)),
                           handler);

  if( msg.GetHeader()->data_size_ > 0 ) {
    boost::asio::async_write(socket,
                             boost::asio::buffer(msg.GetData(), msg.GetHeader()->data_size_),
                             handler);
  }
}

}  // message
}  // wm

#endif /* REALWAKKA_EVENTLOOP_MESSAGE_H_ */
