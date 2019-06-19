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

CDSHttp::CDSHttp(std::string master_ip_port)
    : master_ip_port_(master_ip_port)
{
  LOG("CDSHttp init");
  if (instance_)
  {
    LOG_ERR("CDSHttp Constructor Fail");
    assert(0);
  }
  instance_ = this;
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
  LOG(sb.GetString());

  http::Request request("http://" + master_ip_port_ + "/cds/master/init");
  http::Response response = request.send("POST", sb.GetString());

  if (response.status == http::Response::STATUS_OK)
  {
    rapidjson::Document recv_doc;
    recv_doc.Parse(reinterpret_cast<char*>(response.body.data()));

    LOG(response.body.data());

    desktop_idx_ = recv_doc["dto"]["desktop_idx"].GetInt();
    container_id_ = recv_doc["dto"]["container_id"].GetString();
    user_idx_ = recv_doc["dto"]["user_idx"].GetInt();
    tok_ = recv_doc["dto"]["tok"].GetString();
  }
  else
  {
    LOG_ERR2("POST /cds/master/init error: ", response.status);
  }
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

  http::Request request("http://" + master_ip_port_ + "/cds/master/monitorStatus");
  http::Response response = request.send("POST", sb.GetString());

  if (response.status == http::Response::STATUS_OK)
  {
    rapidjson::Document recv_doc;
    recv_doc.Parse(reinterpret_cast<char*>(response.body.data()));

    LOG2("POST monitorStatus response: ", response.body.data());

    if (!recv_doc["dto"]["err_no"].GetInt())
    {
      rx_session_idx_ = recv_doc["dto"]["rx_session_idx"].GetInt();
    }
  }
  else
  {
    LOG_ERR2("POST /cds/master/monitorStatus error: ", response.status);
  }
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

  http::Request request("http://" + master_ip_port_ + "/cds/master/monitorStatus?action=Update");
  http::Response response = request.send("POST", sb.GetString());

  if (response.status == http::Response::STATUS_OK)
  {
    rapidjson::Document recv_doc;
    recv_doc.Parse(reinterpret_cast<char*>(response.body.data()));

    LOG2("UPDATE monitorStatus response: ", response.body.data());

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
    LOG_ERR2("UPDATE /cds/master/monitorStatus error: ", response.status);
  }
}
