#include <mongo/client/dbclient.h>

struct persistent_data {
  mongo::DBClientConnection db;
  std::string db_name;

  persistent_data() : db_name("test") {
    db.connect("localhost");
  }
};
