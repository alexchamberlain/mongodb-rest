/* MongoDB REST over HTTP interface
 *
 * Copyright 2011 Alex Chamberlain
 * Licensed under the MIT licence with no-advertise clause. See LICENCE.
 *
 */

#ifndef REST_RESPONSE_H
#define REST_RESPONSE_H
#include <string>
#include <map>

#include "mongoose.h"

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

  void send();
};

#endif // REST_RESPONSE_H
