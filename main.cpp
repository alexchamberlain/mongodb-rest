/* MongoDB REST over HTTP interface
 *
 * Copyright 2011 Alex Chamberlain
 * Licensed under the MIT licence with no-advertise clause. See LICENCE.
 *
 */

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <iostream>
#include <memory>
#include <algorithm>

#include "mongoose.h"

#include "response.h"
#include "persistent_data.h"

#include "request_handler.h"

static void *event_handler(enum mg_event event,
                           struct mg_connection *conn,
                           const struct mg_request_info *request_info) {
  void * processed = (void*) 1;

  if (event == MG_NEW_REQUEST) {
    response r(conn);
    request_handler(conn, request_info, r);
    r.send();
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
