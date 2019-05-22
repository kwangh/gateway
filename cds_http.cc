/*
 * cds_http.cc
 *
 *  Created on: 2019. 5. 14.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include "cds_http.h"
#include "common.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

CDSHttp* CDSHttp::instance_ = nullptr;

CDSHttp::CDSHttp(boost::asio::io_context* io_context)
    : http_client_("192.168.6.120:8080")
{
  LOG("CDSHttp init");
  if (instance_)
  {
    LOG_ERR("CDSHttp Constructor Fail");
    assert(0);
  }
  instance_ = this;

  http_server_.io_service = std::shared_ptr < boost::asio::io_service > (io_context);
  http_server_.config.port = 8080;
  http_client_.io_service = std::shared_ptr < boost::asio::io_service > (io_context);

  http_server_.resource["/sessionStart"]["POST"] = [this](
      std::shared_ptr<HttpServer::Response> response,
      std::shared_ptr<HttpServer::Request> request)
  {
    std::string request_json = request->content.string();
    LOG("[Tx->GW] Session Start request: " + request_json);
    response->write("{\"err_no\" : 0}");

    rapidjson::Document io_context;
    io_context.Parse(request_json.c_str());

    int user_idx=-1, desktop_idx=-1;

    if(io_context.HasMember("user_idx") && io_context["user_idx"].IsInt())
    {
      user_idx = io_context["user_idx"].GetInt();
    }

    if(io_context.HasMember("desktop_idx") && io_context["desktop_idx"].IsInt())
    {
      desktop_idx = io_context["desktop_idx"].GetInt();
    }

    rapidjson::Document dto_doc;
    dto_doc.SetObject();
    dto_doc.AddMember("user_idx", user_idx, dto_doc.GetAllocator());
    dto_doc.AddMember("desktop_idx", desktop_idx, dto_doc.GetAllocator());

    rapidjson::Document send_doc;
    send_doc.SetObject();
    send_doc.AddMember("dto", dto_doc, send_doc.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
    send_doc.Accept(w);
    LOG("[GW->Master] Send Session Init request: ");
    LOG(sb.GetString());

  };




  http_server_.resource["/test"]["POST"] = [this](
      std::shared_ptr<HttpServer::Response> response,
      std::shared_ptr<HttpServer::Request> request)
  {
    rapidjson::Document dto_doc;
    dto_doc.SetObject();
    dto_doc.AddMember("user_idx", 1, dto_doc.GetAllocator());
    dto_doc.AddMember("desktop_idx", 2, dto_doc.GetAllocator());

    rapidjson::Document send_doc;
    send_doc.SetObject();
    send_doc.AddMember("dto", dto_doc, send_doc.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
    send_doc.Accept(w);
    LOG(sb.GetString());

    response->write("{\"err_no\" : 0}");
  };

  /*http_server_.resource["^/sessionClose"]["POST"] = [this](
   std::shared_ptr<HttpServer::Response> response,
   std::shared_ptr<HttpServer::Request> request)
   {

   TLOG(LOG_INFO, CDS_AGENT, admin) << "TX <-> Master Session Close";
   session_status_ = 0;
   sendSessionStatusToMaster();
   sendCloseSessionToMaster();
   };*/

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
