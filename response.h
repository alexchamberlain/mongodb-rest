#ifndef REST_RESPONSE_H
#define REST_RESPONSE_H
#include <sstream>
#include <map>

struct response {
  int status_code;
  std::string type;
  std::string contents;
  std::map<std::string, std::string> headers;

  bool dont_send;

  mg_connection* conn;

  response(mg_connection* c) : status_code(200), type("text/json"), contents(""), headers(), dont_send(false), conn(c) {
    /* Empty */
  }

  void set_header(const std::string & n, const std::string & v) {
    headers.insert(std::pair<std::string, std::string>(n,v));
  }

  void send() {
    if(!dont_send) {
      std::ostringstream r;
      r << "HTTP/1.1 " << status_code << "\r\n"
	<< "Content-Type: " << type << "\r\n"
	<< "Content-Length: " << contents.length() << "\r\n";

      for(std::map<std::string, std::string>::const_iterator i = headers.begin();
          i != headers.end();
	  ++i) {
        r << i->first << ": " << i->second << "\r\n";
      }

      r << "\r\n"
	<< contents;

      mg_write(conn, r.str().c_str(), r.str().length());
    }
  }
};

#endif // REST_RESPONSE_H
