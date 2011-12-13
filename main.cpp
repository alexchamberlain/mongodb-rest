#include <cstring>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <iostream>

#include "mongoose.h"

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

static void *event_handler(enum mg_event event,
                           struct mg_connection *conn,
                           const struct mg_request_info *request_info) {
  void * processed = (void*) 1;

  if (event == MG_NEW_REQUEST) {
    if (!request_info->is_ssl) {
      redirect_to_ssl(conn, request_info);
    } else if(strcmp(request_info->request_method, "GET") == 0) {
      std::cout << request_info->uri << std::endl;
      processed = NULL;
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
  "num_threads", "5",
  NULL
};

int main(void) {
  struct mg_context *ctx;

  // Initialize random number generator. It will be used later on for
  // the session identifier creation.
  srand((unsigned) time(0));

  // Setup and start Mongoose
  ctx = mg_start(&event_handler, NULL, options);
  assert(ctx != NULL);

  // Wait until enter is pressed, then exit
  printf("MongoDB REST server started on ports %s, press enter to quit.\n",
         mg_get_option(ctx, "listening_ports"));
  getchar();
  mg_stop(ctx);
  printf("%s\n", "MongoDB REST server stopped.");

  return EXIT_SUCCESS;
}
