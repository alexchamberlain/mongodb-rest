#include <cstring>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <iostream>
#include <memory>
#include <algorithm>

#include <boost/regex.hpp>

#include <mongo/client/dbclient.h>

#include "mongoose.h"

struct uri {
  std::string collection;
  std::string id;

  uri() : collection(), id() {
    /* Empty */
  }
};

struct persistent_data {
  mongo::DBClientConnection db;
  std::string db_name;

  persistent_data() : db_name("test") {
    db.connect("localhost");
  }
};

std::ostream & operator<<(std::ostream & out, const uri & u) {
  out << "{\n  collection: " << u.collection << "\n  id: " << u.id << "\n}";
  return out;
}

static void redirect_to_ssl(struct mg_connection *conn,
                            const struct mg_request_info *request_info) {
  const char *p, *host = mg_get_header(conn, "Host");
  if (host != NULL && (p = strchr(host, ':')) != NULL) {
    mg_printf(conn, "HTTP/1.1 302 Found\r\n"
              "Location: https://%.*s:8082/%s\r\n\r\n",
              p - host, host, request_info->uri);
  } else {
    mg_printf(conn, "%s", "HTTP/1.1 500 Error\r\n\r\nHost: header is not set");
  }
}

static void process_uri(const char * in, uri & out) {
  boost::regex e("^/([^?/]*)");
  boost::cmatch what;
  //std::cout << "Expression: \"" << 
  unsigned j = 0;
  while(boost::regex_search(in, what, e)) {
    std::cout << "Match found..." << std::endl;
    for(unsigned int i = 0; i < what.size(); ++i)
      std::cout << "    $" << i << " = \"" << what[i] << "\"" << std::endl;
    switch(j) {
      case 0: {
        out.collection = what[1];
      } break;
      case 1: {
        out.id = what[1];
      } break;
    }

    in += what[0].length();
    ++j;
  }
}

static void *event_handler(enum mg_event event,
                           struct mg_connection *conn,
                           const struct mg_request_info *request_info) {
  void * processed = (void*) 1;
  uri processed_uri;

  persistent_data * pd = static_cast<persistent_data*>(request_info->user_data);

  if (event == MG_NEW_REQUEST) {
    if (!request_info->is_ssl) {
      redirect_to_ssl(conn, request_info);
    } else if(strcmp(request_info->request_method, "GET") == 0) {
      std::cout << request_info->uri << std::endl;
      process_uri(request_info->uri, processed_uri);
      std::cout << processed_uri << std::endl;

      std::list<std::string> collection_names = pd->db.getCollectionNames(pd->db_name);
      const boost::regex e(pd->db_name + "\\.");

      std::cout << "Collection Names" << std::endl;
      for(std::list<std::string>::iterator i = collection_names.begin();
	  i != collection_names.end();
	  ++i) {
	std::cout << *i << std::endl;
        *i = regex_replace(*i, e, "");
	std::cout << *i << std::endl;
      }

      if(processed_uri.collection.length() == 0) {
        std::ostringstream contents;
	contents << "{\"collections\": [";
        std::list<std::string>::const_iterator i = collection_names.begin();
	if(i != collection_names.end()) {
	  while(true) {
	    contents << "\"" << *i << "\"";
	    ++i;
	    if(i == collection_names.end()) {
	      break;
	    } else {
	      contents << ",";
	    }
	  }
	}
	contents << "]}";

	std::ostringstream ret;
	ret << "HTTP/1.1 200 OK\r\n"
	    << "Content-Length: " << contents.str().length() << "\r\n"
	    << "Content-Type: text/json\r\n"
	    << "\r\n"
	    << contents.str();

	std::cout << ret.str() << std::endl;

	mg_write(conn, ret.str().c_str(), ret.str().length());
      } else {
	/*std::string qualified_collection(pd->db_name);
	qualified_collection += ".";
	qualified_collection += processed_uri.collection;

	std::cout << "qualified_collection: " << qualified_collection << std::endl;*/

	/*std::auto_ptr<mongo::DBClientCursor> cursor =
	  pd->db.query("system.namespaces", BSON("name" << qualified_collection));

	if(!cursor->more()) {
	  std::cout << "Collection does *NOT* exist." << std::endl;
	} else {
	  std::cout << "Collection exists." << std::endl;
	}*/

	if(find(collection_names.begin(), collection_names.end(), processed_uri.collection) == collection_names.end()) {
	  std::cout << "Collection does *NOT* exist." << std::endl;
          const char * e = "HTTP/1.1 404 Not Found\r\n\r\n";
	  mg_write(conn, e, strlen(e));
	} else {
	  std::cout << "Collection exists." << std::endl;
	  std::string qualified_collection(pd->db_name);
	  qualified_collection += ".";
	  qualified_collection += processed_uri.collection;

	  std::cout << "qualified_collection: " << qualified_collection << std::endl;

	  if(processed_uri.id.length() == 0) {
	    mongo::BSONObj empty_bson;

	    std::auto_ptr<mongo::DBClientCursor> cursor =
	      pd->db.query(qualified_collection, empty_bson);

	    std::ostringstream contents;
	    contents << "[";
	    if(cursor->more()) {
	      while(true) {
	        mongo::BSONObj b = cursor->next();
		contents << "\"" << b["_id"].OID().str() << "\"";
		if(!cursor->more()) {
		  break;
		} else {
		  contents << ",";
		}
	      }
	    }
	    contents << "]";

	    std::ostringstream ret;
	    ret << "HTTP/1.1 200 OK\r\n"
		<< "Content-Length: " << contents.str().length() << "\r\n"
		<< "Content-Type: text/json\r\n"
		<< "\r\n"
		<< contents.str();

	    std::cout << ret.str() << std::endl;

	    mg_write(conn, ret.str().c_str(), ret.str().length());
	  } else {
	    std::auto_ptr<mongo::DBClientCursor> cursor =
	      pd->db.query(qualified_collection, QUERY("_id" << mongo::OID(processed_uri.id)));

	    if(!cursor->more()) {
	      const char * e = "HTTP/1.1 404 Not Found\r\n\r\n";
	      mg_write(conn, e, strlen(e));
	    } else {
	      mongo::BSONObj b = cursor->next();
	      std::cout << b << std::endl;
              std::string j = b.jsonString();

	      std::ostringstream ret;
	      ret << "HTTP/1.1 200 OK\r\n"
		  << "Content-Length: " << j.length() << "\r\n"
		  << "Content-Type: text/json\r\n"
		  << "\r\n"
		  << j;

	      std::cout << j << std::endl;

	      mg_write(conn, j.c_str(), j.length());
	    }
	  }
	}
      }

      //processed = NULL;
    } else if(strcmp(request_info->request_method, "POST") == 0){
      const char *clh = mg_get_header(conn, "Content-Length");

      if(clh != NULL) {
	int cl = atoi(clh);
	char * c = static_cast<char*>(malloc(cl+1));
	if(c == NULL) {
          const char * e = "HTTP/1.1 413 Request Entity Too Large\r\n\r\n";
	  mg_write(conn, e, strlen(e));
	} else {
	  mg_read(conn, c, cl);
	  c[cl] = '\0';
	  std::cout << c << std::endl;
	  // No suitable handler found, mark as not processed. Mongoose will
	  // try to serve the request.
	  processed = NULL;
	}
      } else {
        const char * e = "HTTP/1.1 411 Length Required\r\n\r\n"
	                 "Content-Length header not set or malformed.";
        mg_write(conn, e, strlen(e));
      }
    } else if(strcmp(request_info->request_method, "PUT") == 0) {
      processed = NULL;
    } else if(strcmp(request_info->request_method, "DELETE") == 0) {
      processed = NULL;
    } else {
      const char * e = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
      mg_write(conn, e, strlen(e));
    }
  } else {
    processed = NULL;
  }

  return processed;
}

static const char *options[] = {
  //"document_root", "html",
  "listening_ports", "8081,8082s",
  "ssl_certificate", "ssl_cert.pem",
  "num_threads", "1",
  NULL
};

int main(void) {
  struct mg_context *ctx;
  persistent_data pd;

  // Initialize random number generator. It will be used later on for
  // the session identifier creation.
  srand((unsigned) time(0));

  // Setup and start Mongoose
  ctx = mg_start(&event_handler, static_cast<void*>(&pd), options);
  //assert(ctx != NULL);

  // Wait until enter is pressed, then exit
  printf("MongoDB REST server started on ports %s, press enter to quit.\n",
         mg_get_option(ctx, "listening_ports"));
  getchar();
  mg_stop(ctx);
  printf("%s\n", "MongoDB REST server stopped.");

  return EXIT_SUCCESS;
}
