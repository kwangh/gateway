/*
 * cds_http.cc
 *
 *  Created on: 2019. 5. 14.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include <fstream>

#include "cds_http.h"
#include "common.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "HTTPRequest.hpp"

CDSHttp* CDSHttp::instance_ = nullptr;

CDSHttp::CDSHttp(boost::asio::io_context* io_context, std::string master_ip_port)
    : http_client_(master_ip_port)
{
  LOG("CDSHttp init");
  if (instance_)
  {
    LOG_ERR("CDSHttp Constructor Fail");
    assert(0);
  }
  instance_ = this;

  http_client_.io_service = std::shared_ptr < boost::asio::io_service > (io_context);

  post_init();
}

CDSHttp::~CDSHttp()
{
  instance_ = nullptr;
}

CDSHttp* CDSHttp::instance()
{
  return instance_;
}

void CDSHttp::post_init()
{
  std::ifstream f("/etc/hosts", std::ifstream::in);
  std::string line, uuid;
  size_t pos;
  while (std::getline(f, line))
  {
    if ((pos = line.find("tmax-")) != std::string::npos)
    {
      uuid = line.substr(pos + 5);
      LOG("instance uuid: " + uuid);
      break;
    }
  }
  f.close();

  rapidjson::Document dto_doc;
  dto_doc.SetObject();
  rapidjson::Value container_name;
  container_name.SetString(uuid.c_str(), strlen(uuid.c_str()), dto_doc.GetAllocator());
  dto_doc.AddMember("container_id", container_name, dto_doc.GetAllocator());

  rapidjson::Document send_doc;
  send_doc.SetObject();
  send_doc.AddMember("dto", dto_doc, send_doc.GetAllocator());

  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> w(sb);
  send_doc.Accept(w);
  //LOG(sb.GetString());

  http_client_.request("POST", "/cds/master/init", sb.GetString(),
      [this](std::shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code& e)
      {
        if (!e)
        {
          std::string response_json = response->content.string();
          rapidjson::Document recv_doc;
          recv_doc.Parse(response_json.c_str());

          if (recv_doc["header"]["responseCode"] == "NON-0200")
          {
            LOG(response_json);

            desktop_idx_ = recv_doc["dto"]["desktop_idx"].GetInt();
            container_id_ = recv_doc["dto"]["container_id"].GetString();
            user_idx_ =recv_doc["dto"]["user_idx"].GetInt();
            tok_ = recv_doc["dto"]["tok"].GetString();

          }
          else
          {
            LOG_ERR(recv_doc["exception"]["message"].GetString());
          }

        }
        else
        {
          LOG_ERR("POST /cds/master/init error: " + e.message());
        }
      });
}

//session init
void CDSHttp::post_monitor_status()
{
  rapidjson::Document dto_doc;
  dto_doc.SetObject();
  dto_doc.AddMember("user_idx", user_idx_, dto_doc.GetAllocator());
  dto_doc.AddMember("desktop_idx", desktop_idx_, dto_doc.GetAllocator());

  rapidjson::Document send_doc;
  send_doc.SetObject();
  send_doc.AddMember("dto", dto_doc, send_doc.GetAllocator());

  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> w(sb);
  send_doc.Accept(w);
  LOG(sb.GetString());

  SimpleWeb::CaseInsensitiveMultimap header;
  header.emplace("Content-Type", "application/json");

  /*http_client_.request("POST", "/cds/master/monitorStatus", sb.GetString(), header,
      [this](std::shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code& e)
      {
        if (!e)
        {
          std::string response_json = response->content.string();
          rapidjson::Document recv_doc;
          rapidjson::ParseResult ok = recv_doc.Parse(response_json.c_str());

          if(!ok)
          {
            LOG("POST monitorStatus response: " + response_json);
          }
          else
          {
            if (!recv_doc["dto"]["err_no"].GetInt())
            {
              rx_session_idx_ = recv_doc["dto"]["rx_session_idx"].GetInt();
            }
          }
        }

        else
        {
          LOG_ERR("POST /cds/master/monitorStatus error: " + e.message());
        }
      });*/

    http::Request request("http://172.22.5.2:9083/cds/master/monitorStatus");
    http::Response response = request.send("POST", sb.GetString(), {
          "Content-Type: application/json"
      });
    std::cout << response.body.data() << std::endl;
}

void CDSHttp::update_monitor_status()
{
  rapidjson::Document rx_session_info_doc;
  rx_session_info_doc.SetObject();
  rx_session_info_doc.AddMember("rx_session_idx", rx_session_idx_, rx_session_info_doc.GetAllocator());
  rx_session_info_doc.AddMember("status", 0, rx_session_info_doc.GetAllocator());

  rapidjson::Document dto_doc;
  dto_doc.SetObject();
  dto_doc.AddMember("rxSessionInfo", rx_session_info_doc, dto_doc.GetAllocator());

  rapidjson::Document send_doc;
  send_doc.SetObject();
  send_doc.AddMember("dto", dto_doc, send_doc.GetAllocator());

  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> w(sb);
  send_doc.Accept(w);
  LOG(sb.GetString());

  http_client_.request("POST", "/cds/master/monitorStatus?action=Update", sb.GetString(),
      [this](std::shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code& e)
      {
        if (!e)
        {
          std::string response_json = response->content.string();
          rapidjson::Document recv_doc;
          recv_doc.Parse(response_json.c_str());
          LOG("UPDATE monitorStatus response: " + response_json);

          if (!recv_doc["dto"]["err_no"].GetInt())
          {
            LOG(recv_doc["dto"]["message"].GetString());
          }
          else
          {
            LOG_ERR(recv_doc["dto"]["message"].GetString());
          }
        }
        else
        {
          LOG_ERR("UPDATE /cds/master/monitorStatus error: " + e.message());
        }
      });
}
