#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <boost/asio.hpp>
#include <vector>
namespace netproto{
	template<class T>
	class Socket{
	private:
		T sock;
	public:
		Socket(boost::asio::io_service& service) : sock(service){}
		~Socket(){ sock.close(); }
		size_t receive(std::vector<char>& buf){
			buf.resize(sock.available());
			return sock.read_some(boost::asio::buffer(buf));
		}
		size_t receive(void** buf, size_t* siz){
			*siz = sock.available();
			*buf = new char[siz];
			return sock.read_some(boost::asio::buffer(buf, siz));
		}
		size_t send(const std::vector<char>& buf){
			return sock.write_some(boost::asio::buffer(buf));
		}
		size_t send(void* buf, size_t siz){
			return sock.write_some(boost::asio::buffer(buf,siz));
		}
		T& ref(){ return sock; }
	};
}