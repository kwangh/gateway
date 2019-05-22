/*
 * agent.cc
 *
 *  Created on: 2019. 5. 22.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include <iostream>
#include <array>
#include <sys/inotify.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "common.h"

class agent
{
public:
  enum
  {
    max_body_length = 4096
  };

  agent(boost::asio::io_service* io_service, const char* pathname)
      : io_service_(*io_service)
  {
    init(pathname);
  }

  ~agent()
  {
    inotify_rm_watch(fd_, wd_);
    close(fd_);

    if (sd_)
    {
      delete sd_;
    }
  }

  void init(const char* pathname)
  {
    fd_ = inotify_init();
    if (fd_ < 0)
    {
      LOG_ERR("Couldn't initialize inotify");
      assert(0);
    }

    wd_ = inotify_add_watch(fd_, pathname, IN_ALL_EVENTS);
    if (wd_ == -1)
    {
      LOG_ERR2("Couldn't add watch to ", pathname);
      assert(0);
    }
    else
    {
      LOG2("Watching: ", pathname);
    }

    sd_ = new boost::asio::posix::stream_descriptor(io_service_, fd_);

    sd_->async_read_some(boost::asio::buffer(buffer_),
        boost::bind(&agent::read_handler, this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }

  void read_handler(const boost::system::error_code& ec, std::size_t bt)
  {
    if (!ec)
    {
      buffer_str_.append(buffer_.data(), buffer_.data() + bt);
      while (buffer_str_.size() >= sizeof(inotify_event))
      {
        const inotify_event *ie = reinterpret_cast<const inotify_event *>(buffer_str_.data());

        if (strstr(ie->name, "vds.log"))
        {
          if (ie->mask == IN_CREATE)
          {
            LOG2(ie->name, " is created");
          }
          else if (ie->mask == IN_MODIFY)
          {
            LOG2(ie->name, " is modified");
            system("cat /tmp/vds.log");
          }
          else if (ie->mask == IN_OPEN)
          {
            LOG2(ie->name, " is opened");
          }
        }

        buffer_str_.erase(0, sizeof(inotify_event) + ie->len);
      }

      sd_->async_read_some(boost::asio::buffer(buffer_),
          boost::bind(&agent::read_handler, this, boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
    }
  }

private:
  boost::asio::io_service& io_service_;
  boost::asio::posix::stream_descriptor* sd_;
  int fd_, wd_;
  std::array<char, max_body_length> buffer_;
  std::string buffer_str_;
};

int main()
{
  try
  {
    boost::asio::io_service* io_service = new boost::asio::io_service;
    agent agent(io_service, "/tmp");

    io_service->run();

    if (io_service)
      delete io_service;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
