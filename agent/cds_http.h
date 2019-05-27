/*
 * cds_http.h
 *
 *  Created on: 2019. 5. 14.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#ifndef CDS_HTTP_H_
#define CDS_HTTP_H_

#include "http/client_http.hpp"

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

/* class CDSHttp
 * Session controller that utilizes HTTP protocol.
 */
class CDSHttp
{
public:
  CDSHttp(boost::asio::io_context* io_context, std::string master_ip_port);
  ~CDSHttp();
  static CDSHttp* instance();
  void post_monitor_status();
  void update_monitor_status();

private:
  void post_init();

private:
  static CDSHttp* instance_;

  HttpClient http_client_;

  int desktop_idx_, user_idx_, rx_session_idx_;
  std::string container_id_, tok_;
};

#endif /* CDS_HTTP_H_ */
