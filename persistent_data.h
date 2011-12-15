/* MongoDB REST over HTTP interface
 *
 * Copyright 2011 Alex Chamberlain
 * Licensed under the MIT licence with no-advertise clause. See LICENCE.
 *
 */

#include <mongo/client/dbclient.h>

struct persistent_data {
  mongo::DBClientConnection db;
  std::string db_name;

  persistent_data() : db_name("test") {
    db.connect("localhost");
  }
};
