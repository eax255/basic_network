#include "common.hpp"
#include "log.hpp"
#include <unordered_map>
#include <memory>
const char encodeMap[63] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

namespace netproto{
	using boost::asio::ip::tcp;

	// Turn base-62 repr into integer
	size_t decodeKey(std::string s){
		size_t v = 0;
		for (auto k : s){
			v *= 62;
			if      (k <  '0') return -1;
			else if (k <= '9') v += (k - '0');
			else if (k <  'A') return -1;
			else if (k <= 'Z') v += (k - 'A' + 10);
			else if (k <  'a') return -1;
			else if (k <= 'z') v += (k - 'a' + 36);
			else               return -1;
		}
		return v;
	}

	// Turn integer into base-62 repr
	std::string encodeKey(size_t s){
		std::string v;
		while (s>0){
			v = encodeMap[s % 62] + v;
			s /= 62;
		}
		return v;
	}

	// url-shortening service server
	// TODO: better error handling.
	int server_main(){
		// db, io_service and acceptor
		std::unordered_map<size_t, std::string> urls;
		boost::asio::io_service serv;
		tcp::acceptor acceptor(serv, tcp::endpoint(tcp::v4(),80));

		while (1){
			LOG(trace) << "Waiting for connection.";
			Socket<tcp::socket>* sock = new Socket<tcp::socket>(serv);
			acceptor.accept(sock->ref());
			LOG(trace) << "Conection received.";

			// This should really branch into a different thread if this were largely used service.
			// However this is mainly prototype that is not required to be widely-used.

			// Read request.
			std::vector<char> buf;
			sock->receive(buf);
			std::string str(buf.begin(),buf.end());
			LOG(trace) << "Request: \n" + str;

			std::string url;
			size_t key;

			// Parse url from request
			// If we don't find one, abort connection.
			size_t i = str.find('/');
			if (i == std::string::npos)goto end;
			size_t j = j = str.find(' ', i);
			if (j == std::string::npos)goto end;
			url = str.substr(i + 1, j - i - 1);

			// Decode the string REGARDLESS if not GET-request  NOTE: could be a problem if used on a large scale
			key = decodeKey(url);
			LOG(trace) << url << "->" << key;

			// GET request AND url in db
			if (str.substr(0, 3) == "GET" && urls.count(key)){
				std::string v = "HTTP/1.1 301 Moved Permanently\r\nLocation: " + urls[key] + "\r\n\r\n";
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}

			// GET request to /shorten == show a simple form. NOTE: this probably should be handled somewhere else but I wanted to do a self-contained service.
			else if (str.substr(0, 3) == "GET" && url == "shorten"){
				std::string v = "HTTP/1.1 200 Accepted\r\nContent-Type: text/html\r\n\r\n"
					"<form action=\"shorten\" method=\"POST\">"
					"Link:<br>"
					"<input type=\"text\" name=\"link\"><br><br>"
					"<input type=\"submit\" value=\"Submit\">"
					"</form>";
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}

			// POST request to /shorten. NOTE: This doesn't have much error handling; some urls might not work properly
			else if (str.substr(0, 4) == "POST" && url == "shorten"){
				// TODO: we could always read more of the request instead of just scipping to the link part.
				i = str.find("link=");
				if (i == std::string::npos)goto end;
				j = str.find("\r\n", i);
				if (i == std::string::npos)goto end;
				std::string link = str.substr(i+5,j-i-5);

				// The extracted url has some characters in form %xx, convert them to correct characters before hashing.
				while ((i = link.find_first_of('%')) != std::string::npos){
					auto k = link.substr(i, 3);
					char v = 0;

					// TODO make make hexadecimal -> character conversion cleaner
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

					// Replace %xx witc the correct character.
					link = link.substr(0, i) + v + link.substr(i + 3);
				}

				LOG(trace) << link;

				// Hash using std::hash and moving by one if collission.
				// This manages situation where the same url is submitted fine.
				// Two-way map would also work for that but I'll use this aproach.
				std::string hash;
				auto s = std::hash<std::string>()(link);
				//auto rh = std::hash<size_t>();
				while (urls.count(s)&&urls[s]!=link) ++s;

				// Save to db, send encoded id in response.
				LOG(trace) << "Hash: " << s;
				urls[s] = link;
				hash = encodeKey(s);
				std::string v = "HTTP/1.1 200 Accepted\r\nContent-Type: text/plain\r\n\r\n" + hash;
				LOG(trace) << "Response:\n" + v;
				sock->send((char*)v.c_str(), v.length());
			}

			// Not found error.
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
		return 0;
	}
}
int main(){
	return netproto::server_main();
}