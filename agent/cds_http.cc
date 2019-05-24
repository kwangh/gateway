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

CDSHttp* CDSHttp::instance_ = nullptr;

CDSHttp::CDSHttp(boost::asio::io_context* io_context, std::string master_ip)
    : http_client_(master_ip)
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
  dto_doc.AddMember("container_name", container_name, dto_doc.GetAllocator());

  rapidjson::Document send_doc;
  send_doc.SetObject();
  send_doc.AddMember("dto", dto_doc, send_doc.GetAllocator());

  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> w(sb);
  send_doc.Accept(w);
  LOG(sb.GetString());

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

            LOG(recv_doc["dto"]["message"].GetString());

            post_monitor_status();
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

  http_client_.request("POST", "/cds/master/monitorStatus", sb.GetString(),
      [this](std::shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code& e)
      {
        if (!e)
        {
          std::string response_json = response->content.string();
          rapidjson::Document recv_doc;
          recv_doc.Parse(response_json.c_str());
          LOG("POST monitorStatus response: " + response_json);

          if (!recv_doc["dto"]["err_no"].GetInt())
          {
            rx_session_idx_ = recv_doc["dto"]["rx_session_idx"].GetInt();
          }
        }

        else
        {
          LOG_ERR("POST /cds/master/monitorStatus error: " + e.message());
        }
      });
}

void CDSHttp::update_monitor_status()
{
  http_client_.request("POST", "/cds/master/monitorStatus?action=Update", getStringJson(),
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
            TLOG(LOG_INFO, CDS_AGENT, admin)
            << rapidjson::Pointer("/dto/message").Get(recvDoc)->GetString();
          }
          else
          {
            TLOG(LOG_ERROR, CDS_AGENT, admin)
            << "error : " << rapidjson::Pointer("/dto/message").Get(recvDoc)->GetString();
          }
        }
        else
        {
          LOG_ERR("UPDATE /cds/master/monitorStatus error: " + e.message());
        }
      });
}
