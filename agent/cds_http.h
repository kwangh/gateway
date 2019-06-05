/*
 * cds_http.h
 *
 *  Created on: 2019. 5. 14.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#ifndef CDS_HTTP_H_
#define CDS_HTTP_H_

/* class CDSHttp
 * Session controller that utilizes HTTP protocol.
 */
class CDSHttp
{
public:
  CDSHttp(std::string master_ip_port);
  ~CDSHttp();
  static CDSHttp* instance();

  void post_init();
  void post_monitor_status();
  void update_monitor_status();

private:
  static CDSHttp* instance_;

  int desktop_idx_, user_idx_, rx_session_idx_;
  std::string container_id_, master_ip_port_, tok_;
};

#endif /* CDS_HTTP_H_ */
