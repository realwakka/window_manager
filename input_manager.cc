#include "input_manager.h"

#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

#include "server.h"

namespace wm {

namespace {

const std::string event_kbd_suffix = "-event-kbd";
const std::string event_mouse_suffix = "-event-mouse";
const std::string mouse_suffix = "-mouse";

bool HasSuffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}

InputDevice::InputDevice(boost::asio::io_service& io_service, Server& server)
    : desc_(io_service),
      server_(server)
{
}

void InputDevice::Open(const std::string& path)
{
  auto fd = open(path.c_str(), O_RDONLY);
  desc_.assign(fd);
  
  boost::asio::async_read(desc_,
                          boost::asio::buffer(&input_event_, sizeof(input_event_)),
                          boost::bind(&InputDevice::OnRead,
                                      this, boost::asio::placeholders::error));

  
  // if(HasSuffix(path, "event-kbd")) {
    
  // } else if(HasSuffix(path, "event-mouse")) {

  // } else if(HasSuffix(path, "mouse")) {
          
  // }
 
}

void InputDevice::OnRead(boost::system::error_code error)
{
  std::cout << "key event!!!!" << std::endl;
  std::cout << input_event_.type << std::endl;
  std::cout << input_event_.code << std::endl;
  std::cout << input_event_.value << std::endl;


  
  server_.OnInputEvent(input_event_);
  
  boost::asio::async_read(desc_,
                          boost::asio::buffer(&input_event_, sizeof(input_event_)),
                          boost::bind(&InputDevice::OnRead,
                                      this, boost::asio::placeholders::error));
}

InputManager::InputManager(boost::asio::io_service& io_service, Server& server)
    : io_service_(io_service),
      server_(server)
{}
InputManager::~InputManager()
{}

void InputManager::OpenInput()
{

  boost::filesystem::path p ("/dev/input/by-id");

  if(boost::filesystem::exists(p)) {
  
    boost::filesystem::directory_iterator end;
    boost::filesystem::directory_iterator begin(p);

    std::for_each(begin, end, [this] ( auto&& itr ) {
        auto device = std::make_shared<InputDevice>(io_service_, server_);
        device->Open(itr.path().string());
        device_list_.emplace_back(device);
      });
  }
  
  // for (boost::filesystem::directory_iterator itr(p); itr != end_itr; ++itr)
  //   {
  //       // If it's not a directory, list it. If you want to list directories too, just remove this check.
  //       if (is_regular_file(itr->path())) {
  //           // assign current file name to current_file and echo it out to the console.
  //           string current_file = itr->path().string();
  //           cout << current_file << endl;
  //       }
  //   }


  // std::string key_path = "/dev/input/event0";
  // auto key_fd = open(key_path.c_str(), O_RDONLY);
  // key_desc_.assign(key_fd);
  // boost::asio::async_read(key_desc_,
  //                         boost::asio::buffer(&input_event_, sizeof(input_event_)),
  //                         boost::bind(&Server::OnKeyEvent,
  //                                     this, boost::asio::placeholders::error));
}

}  // wm
