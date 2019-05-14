/*
 * gateway.cc
 *
 *  Created on: 2019. 4. 3.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include <iostream>
#include <vector>
#include <queue>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "chat_message.h"
#include "cds_http.h"

typedef std::queue<chat_message> chat_message_queue;

class socket_info
{
public:
  socket_info(boost::asio::ip::tcp::socket* socket, uint8_t id)
      : socket_(socket), id_(id)
  {
  }

  const uint16_t get_id() const
  {
    return id_;
  }

  void set_id(uint16_t id)
  {
    id_ = id;
  }

  boost::asio::ip::tcp::socket* socket()
  {
    return socket_;
  }

  void socket(boost::asio::ip::tcp::socket* socket)
  {
    socket_ = socket;
  }

private:
  boost::asio::ip::tcp::socket* socket_;
  uint16_t id_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_context* io_context,
      const boost::asio::ip::tcp::endpoint& tx_endpoint,
      const boost::asio::ip::tcp::endpoint& rx_endpoint)
      : io_context_(*io_context),
        tx_acceptor_(*io_context, tx_endpoint),
        rx_acceptor_(*io_context, rx_endpoint)
  {
    cds_http_ = new CDSHttp(io_context);

    start_tx_accept();
    start_rx_accept();
  }

  ~tcp_server()
  {
    if(cds_http_)
      delete cds_http_;
    clear_vector();
  }

private:
  void start_tx_accept()
  {
    boost::asio::ip::tcp::socket* tx_accept_socket = new boost::asio::ip::tcp::socket(io_context_);
    tx_acceptor_.async_accept(*tx_accept_socket,
        boost::bind(&tcp_server::handle_tx_accept, this, tx_accept_socket, boost::asio::placeholders::error));
  }

  void start_rx_accept()
  {
    boost::asio::ip::tcp::socket* rx_accept_socket = new boost::asio::ip::tcp::socket(io_context_);
    rx_acceptor_.async_accept(*rx_accept_socket,
        boost::bind(&tcp_server::handle_rx_accept, this, rx_accept_socket, boost::asio::placeholders::error));
  }

  void handle_tx_accept(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (error)
    {
      std::cerr << "handle_tx_accept error : " << error.message() << "\n";
      socket->close();
      delete socket;
    }
    else if (tx_sockets_.size() >= 1)
    {
      std::cout << "tx socket number is bigger than 1" << "\n";
      socket->close();
      delete socket;
    }
    else
    {
      socket_info si = socket_info(socket, tx_sockets_.size()); // temporary
      tx_sockets_.push_back(si);
      std::cout << "tx socket vector size : " << tx_sockets_.size() << "\n";

      boost::asio::async_read(*socket, boost::asio::buffer(tx_read_msg_.data(), chat_message::header_length),
          boost::bind(&tcp_server::handle_tx_read_header, this, socket, boost::asio::placeholders::error));
    }

    start_tx_accept();
  }

  void handle_rx_accept(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (error)
    {
      std::cerr << "handle_rx_accept error : " << error.message() << "\n";
      socket->close();
      delete socket;
    }
    else if (rx_sockets_.size() >= 1)
    {
      std::cout << "rx socket number is bigger than 1" << "\n";
      socket->close();
      delete socket;
    }
    else
    {
      socket_info si = socket_info(socket, rx_sockets_.size()); // temporary
      rx_sockets_.push_back(si);
      std::cout << "rx socket vector size : " << rx_sockets_.size() << "\n";

      boost::asio::async_read(*socket, boost::asio::buffer(rx_read_msg_.data(), chat_message::header_length),
          boost::bind(&tcp_server::handle_rx_read_header, this, socket, boost::asio::placeholders::error));
    }

    start_rx_accept();
  }

  void handle_tx_read_header(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (!error && tx_read_msg_.decode_header())
    {
      boost::asio::async_read(*socket, boost::asio::buffer(tx_read_msg_.body(), tx_read_msg_.body_length()),
          boost::bind(&tcp_server::handle_tx_read_body, this, socket, boost::asio::placeholders::error));
    }
    else
    {
      std::cerr << "handle_tx_read_header error : " << error.message() << "\n";
      clear_vector();
    }
  }

  void handle_rx_read_header(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (!error && rx_read_msg_.decode_header())
    {
      boost::asio::async_read(*socket, boost::asio::buffer(rx_read_msg_.body(), rx_read_msg_.body_length()),
          boost::bind(&tcp_server::handle_rx_read_body, this, socket, boost::asio::placeholders::error));
    }
    else
    {
      std::cerr << "handle_rx_read_header error : " << error.message() << "\n";
      clear_vector();
    }
  }

  void handle_tx_read_body(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (!error)
    {
      deliver_to_rx(tx_read_msg_, 0);
      boost::asio::async_read(*socket, boost::asio::buffer(tx_read_msg_.data(), chat_message::header_length),
          boost::bind(&tcp_server::handle_tx_read_header, this, socket, boost::asio::placeholders::error));
    }
    else
    {
      std::cerr << "handle_tx_read_body error : " << error.message() << "\n";
      clear_vector();
    }
  }

  void handle_rx_read_body(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (!error)
    {
      deliver_to_tx(rx_read_msg_, 0);
      boost::asio::async_read(*socket, boost::asio::buffer(rx_read_msg_.data(), chat_message::header_length),
          boost::bind(&tcp_server::handle_rx_read_header, this, socket, boost::asio::placeholders::error));
    }
    else
    {
      std::cerr << "handle_rx_read_body error : " << error.message() << "\n";
      clear_vector();
    }
  }

  void deliver_to_rx(const chat_message& msg, uint8_t id)
  {
    bool write_in_progress = !rx_write_msgs_.empty();
    rx_write_msgs_.push(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(*(rx_sockets_[id].socket()),
          boost::asio::buffer(rx_write_msgs_.front().data(), rx_write_msgs_.front().length()),
          boost::bind(&tcp_server::handle_tx_write, this, rx_sockets_[id].socket(),
              boost::asio::placeholders::error));
    }
  }

  void deliver_to_tx(const chat_message& msg, uint8_t id)
  {
    bool write_in_progress = !tx_write_msgs_.empty();
    tx_write_msgs_.push(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(*(tx_sockets_[id].socket()),
          boost::asio::buffer(tx_write_msgs_.front().data(), tx_write_msgs_.front().length()),
          boost::bind(&tcp_server::handle_rx_write, this, tx_sockets_[id].socket(),
              boost::asio::placeholders::error));
    }
  }

  void handle_tx_write(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (!error)
    {
      rx_write_msgs_.pop();
      if (!rx_write_msgs_.empty())
      {
        boost::asio::async_write(*socket,
            boost::asio::buffer(rx_write_msgs_.front().data(), rx_write_msgs_.front().length()),
            boost::bind(&tcp_server::handle_tx_write, this, socket, boost::asio::placeholders::error));
      }
    }
    else
    {
      std::cerr << "handle_tx_write error : " << error.message() << "\n";
      clear_vector();
    }
  }

  void handle_rx_write(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
  {
    if (!error)
    {
      tx_write_msgs_.pop();
      if (!tx_write_msgs_.empty())
      {
        boost::asio::async_write(*socket,
            boost::asio::buffer(tx_write_msgs_.front().data(), tx_write_msgs_.front().length()),
            boost::bind(&tcp_server::handle_rx_write, this, socket, boost::asio::placeholders::error));
      }
    }
    else
    {
      std::cerr << "handle_rx_write error : " << error.message() << "\n";
      clear_vector();
    }
  }

  void clear_vector()
  {
    // tx
    std::cout << "clearing tx socket vector : " << tx_sockets_.size() << "\n";
    for (std::vector<socket_info>::iterator it = tx_sockets_.begin(); it != tx_sockets_.end(); ++it)
    {
      (*it).socket()->close();
      delete (*it).socket();
    }
    tx_sockets_.clear();
    std::cout << "tx socket vector cleared : " << tx_sockets_.size() << "\n";

    // rx
    std::cout << "clearing rx socket vector : " << rx_sockets_.size() << "\n";
    for (std::vector<socket_info>::iterator it = rx_sockets_.begin(); it != rx_sockets_.end(); ++it)
    {
      (*it).socket()->close();
      delete (*it).socket();
    }
    rx_sockets_.clear();
    std::cout << "rx socket vector cleared : " << rx_sockets_.size() << "\n";
  }

  boost::asio::io_context& io_context_;
  boost::asio::ip::tcp::acceptor tx_acceptor_;
  boost::asio::ip::tcp::acceptor rx_acceptor_;
  std::vector<socket_info> tx_sockets_;
  std::vector<socket_info> rx_sockets_;
  chat_message tx_read_msg_;
  chat_message rx_read_msg_;
  chat_message_queue tx_write_msgs_;
  chat_message_queue rx_write_msgs_;

  CDSHttp* cds_http_;
};

int main()
{
  try
  {
    boost::asio::io_context* io_context = new boost::asio::io_context;

    const uint16_t tx_port = 1233;
    const uint16_t rx_port = 1234;

    boost::asio::ip::tcp::endpoint tx_endpoint(boost::asio::ip::tcp::v4(), tx_port);
    boost::asio::ip::tcp::endpoint rx_endpoint(boost::asio::ip::tcp::v4(), rx_port);
    tcp_server server(io_context, tx_endpoint, rx_endpoint);

    io_context->run();

    if(io_context) delete io_context;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
