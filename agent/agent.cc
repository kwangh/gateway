/*
 * agent.cc
 *
 *  Created on: 2019. 5. 22.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include <iostream>
#include <array>
#include <fstream>
#include <limits>
#include <sys/inotify.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

#include "cds_http.h"
#include "common.h"

class agent
{
public:
  enum
  {
    max_body_length = 4096
  };

  agent(boost::asio::io_service* io_service, const char* pathname, std::string master_ip)
      : io_service_(*io_service), cds_http_(new CDSHttp(io_service, master_ip))
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

    if (cds_http_)
      delete cds_http_;
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

  std::istream& ignore_line(std::ifstream& in, std::ifstream::pos_type& pos)
  {
    pos = in.tellg();
    return in.ignore(std::numeric_limits < std::streamsize > ::max(), '\n');
  }

  std::string get_last_line(std::ifstream& in)
  {
    std::ifstream::pos_type pos, last_pos;

    while (in >> std::ws && ignore_line(in, pos))
      last_pos = pos;

    in.clear();
    in.seekg(last_pos);
    std::string line;
    std::getline(in, line);
    return line;
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
          if (ie->mask == IN_MODIFY)
          {
            LOG2(ie->name, " is modified");

            std::ifstream file("/zone/normal/rootfs/tmp/vds.log");

            if (file)
            {
              std::string line = get_last_line(file);
              LOG(line);
              size_t pos;

              if (line.find("Rx_connection_status : Done") != std::string::npos)
              {
                CDSHttp::instance()->post_monitor_status();
              }

            }
            else
            {
              LOG_ERR("file open fail");
              file.close();
              assert(0);
            }

            file.close();
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

  CDSHttp* cds_http_;
};

int main()
{
  try
  {
    boost::asio::io_service* io_service = new boost::asio::io_service;
    const char* vds_log_path = "/zone/normal/rootfs/tmp";

    std::ifstream ifs("/zone/normal/rootfs/etc/vds/config.json");
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document d;
    d.ParseStream(isw);

    assert(d.IsObject());
    assert(d.HasMember("agent"));
    assert(d["agent"].HasMember("master_ip"));
    assert(d["agent"].HasMember("master_port"));

    std::string master_ip = d["agent"]["master_ip"].GetString();
    std::string master_port = std::to_string(d["agent"]["master_port"].GetInt());
    LOG2("master ip: ", master_ip);
    LOG2("master_port: ", master_port)

    agent agent(io_service, vds_log_path, master_ip + ":" + master_port);

    io_service->run();

    if (io_service)
      delete io_service;
  }
  catch (std::exception& e)
  {
    LOG_ERR2("Exception: ", e.what());
  }

  return 0;
}
