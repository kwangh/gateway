/*
 * cds_http.h
 *
 *  Created on: 2019. 5. 14.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#ifndef CDS_HTTP_H_
#define CDS_HTTP_H_

#include "http/client_http.hpp"
#include "http/server_http.hpp"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

/* class CDSHttp
 * Session controller that utilizes HTTP protocol.
 */
class CDSHttp
{
public:
  CDSHttp(boost::asio::io_context* io_context);
  ~CDSHttp();
  static CDSHttp* instance();

private:
  void sendSessionInit();

private:
  static CDSHttp* instance_;

  HttpClient http_client_;
  HttpServer http_server_;
};

#endif /* CDS_HTTP_H_ */
