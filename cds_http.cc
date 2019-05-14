/*
 * cds_http.cc
 *
 *  Created on: 2019. 5. 14.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include "cds_http.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

CDSHttp* CDSHttp::instance_ = nullptr;

CDSHttp::CDSHttp(boost::asio::io_context* io_context)
    : http_client_("192.168.6.120:8080")
{
  std::cout << "CDSHttp init\n";
  if (instance_)
  {
    std::cerr << "CDSHttp Constructor Fail\n";
    assert(0);
  }
  instance_ = this;

  http_server_.io_service = std::shared_ptr < boost::asio::io_service > (io_context);
  http_server_.config.port = 8080;
  http_client_.io_service = std::shared_ptr < boost::asio::io_service > (io_context);

  http_server_.resource["^/shutdown$"]["POST"] = [this](
      std::shared_ptr<HttpServer::Response> response,
      std::shared_ptr<HttpServer::Request> request)
  {
    response->write("shutdown ok\r\n");
  };

  http_server_.start();
}

CDSHttp::~CDSHttp()
{
  instance_ = nullptr;
}

CDSHttp* CDSHttp::instance()
{
  return instance_;
}
