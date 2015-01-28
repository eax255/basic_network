#include "common.hpp"
#include "log.hpp"
#include <unordered_map>
namespace netproto{
	using boost::asio::ip::tcp;
	int server_main(){
		std::unordered_map<std::string, std::string> urls;
		boost::asio::io_service serv;

		tcp::acceptor acceptor(serv, tcp::endpoint(tcp::v4(),80));

		while (1){
			LOG(trace) << "Waiting for connection.";
			Socket<tcp::socket>* sock = new Socket<tcp::socket>(serv);
			acceptor.accept(sock->ref());
			LOG(trace) << "Conection received.";

			std::vector<char> buf;
			sock->receive(buf);
			std::string str(buf.begin(),buf.end());
			LOG(trace) << "Request: \n" + str;

			std::string url;

			size_t i = str.find('/');
			if (i == std::string::npos)goto end;
			size_t j = j = str.find(' ', i);
			if (j == std::string::npos)goto end;

			url = str.substr(i + 1, j - i - 1);
			std::cout << url << '\n';

			if (str.substr(0, 3) == "GET" && urls.count(url)){
				std::string v = "HTTP/1.1 301 Moved Permanently\r\nLocation: " + urls[url] + "\r\n\r\n";
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}
			else if (str.substr(0, 3) == "GET" && url == "shorten-form"){
				std::string v = "HTTP/1.1 200 Accepted\r\nContent-Type: text/html\r\n\r\n"
					"<form action=\"shorten\" method=\"POST\">"
					"Link:<br>"
					"<input type=\"text\" name=\"link\"><br><br>"
					"<input type=\"submit\" value=\"Submit\">"
					"</form>";
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}
			else if (str.substr(0, 4) == "POST" && url == "shorten"){
				i = str.find("link=");
				if (i == std::string::npos)goto end;
				j = str.find("\r\n", i);
				if (i == std::string::npos)goto end;
				std::string link = str.substr(i+5,j-i-5);
				while ((i = link.find_first_of('%')) != std::string::npos){
					auto k = link.substr(i, 3);
					char v = 0;

					if      (k[1] <  '0') continue;
					else if (k[1] <= '9') v += (k[1] - '0') * 16;
					else if (k[1] <  'A') continue;
					else if (k[1] <= 'F') v += (k[1] - 'A' + 10) * 16;
					else if (k[1] <  'a') continue;
					else if (k[1] <= 'f') v += (k[1] - 'a' + 10) * 16;
					else                  continue;

					if      (k[2] <  '0') continue;
					else if (k[2] <= '9') v += (k[2] - '0');
					else if (k[2] <  'A') continue;
					else if (k[2] <= 'F') v += (k[2] - 'A' + 10);
					else if (k[2] <  'a') continue;
					else if (k[2] <= 'f') v += (k[2] - 'a' + 10);
					else                  continue;

					link = link.substr(0, i) + v + link.substr(i + 3);
				}
				LOG(trace) << link;
				std::string hash = "asd";
				urls["asd"] = link;
				std::string v = "HTTP/1.1 200 Accepted\r\n\r\n" + hash;
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}
			else{
				std::string v = "HTTP/1.1 404 Not Found\r\n\r\n Not Found";
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}
end:		std::cout << '\n';
			//sock->send(buf);
			LOG(trace) << "Conection closed.";
			delete sock;
		}
	}
}
int main(){
	return netproto::server_main();
}