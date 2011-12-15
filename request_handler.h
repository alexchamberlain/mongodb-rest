/* MongoDB REST over HTTP interface
 *
 * Copyright 2011 Alex Chamberlain
 * Licensed under the MIT licence with no-advertise clause. See LICENCE.
 *
 */

#include "response.h"

void request_handler(mg_connection* conn, const mg_request_info* request_info, response& r);
