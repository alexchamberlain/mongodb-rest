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

#include <boost/regex.hpp>

#include <mongo/client/dbclient.h>

#include "mongoose.h"

#include "uri.h"
#include "persistent_data.h"
#include "response.h"

static void redirect_to_ssl(struct mg_connection *conn,
                            const struct mg_request_info *request_info,
			    response& r) {
  const char *p, *host = mg_get_header(conn, "Host");
  if (host != NULL && (p = strchr(host, ':')) != NULL) {
    std::string location("https://");
    location.append(host, p - host);
    location += ":8082";
    location += request_info->uri;

    r.status_code = 302;
    r.set_header("Location", location);
    r.contents = "Transport over SSL only.";
  } else {
    r.status_code = 500;
    r.contents = "Host header not set.";
  }
}

static void process_uri(const char * in, uri & out) {
  boost::regex e("^/([^?/]*)");
  boost::cmatch what;
  //std::cout << "Expression: \"" << 
  unsigned j = 0;
  while(boost::regex_search(in, what, e)) {
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

static void get_request_handler(mg_connection* conn,
                                const mg_request_info* request_info,
				const uri& processed_uri,
				response& r) {
  persistent_data * pd = static_cast<persistent_data*>(request_info->user_data);

  std::list<std::string> collection_names = pd->db.getCollectionNames(pd->db_name);
  const boost::regex e(pd->db_name + "\\.");

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

    r.status_code = 200;
    r.contents = contents.str();
  } else {
    if(find(collection_names.begin(), collection_names.end(), processed_uri.collection) == collection_names.end()) {
      r.status_code = 404;
    } else {
      std::string qualified_collection(pd->db_name);
      qualified_collection += ".";
      qualified_collection += processed_uri.collection;

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

	r.status_code = 200;
	r.contents = contents.str();
      } else {
	std::auto_ptr<mongo::DBClientCursor> cursor =
	  pd->db.query(qualified_collection, QUERY("_id" << mongo::OID(processed_uri.id)));

	if(!cursor->more()) {
	  r.status_code = 404;
	} else {
	  mongo::BSONObj b = cursor->next();
	  std::string j = b.jsonString();

	  r.status_code = 200;
	  r.contents = j;
	}
      }
    }
  }
}

static void post_request_handler(mg_connection* conn,
                                 const mg_request_info* request_info,
				 const uri& processed_uri,
				 response& r) {
  persistent_data * pd = static_cast<persistent_data*>(request_info->user_data);

  std::string qualified_collection(pd->db_name);
  qualified_collection += ".";
  qualified_collection += processed_uri.collection;

  const char *clh = mg_get_header(conn, "Content-Length");

  std::cout << "post_request_handler responding." << std::endl;

  if(clh == NULL) {
    r.status_code = 411;
    r.contents = "Content-Length header malformed or not set.";
  } else {
    int cl = atoi(clh);
    char * c = static_cast<char*>(malloc(cl+1));
    if(c == NULL) {
      r.status_code = 413;
    } else {
      mongo::BSONObjBuilder ob;
      ob.genOID();
      mg_read(conn, c, cl);
      c[cl] = '\0';
      std::cout << c << std::endl;
      mongo::BSONObj b(mongo::fromjson(c));
      ob.appendElements(b);
      b = ob.obj();
      std::cout << b << std::endl;
      pd->db.insert(qualified_collection, b);

      std::string location("/");
      location += processed_uri.collection;
      location += "/";
      location += b["_id"].OID().str();

      r.status_code = 303;
      r.set_header("Location", location);
      r.contents = "Resource created";
    }
  }
}

static void put_request_handler(mg_connection* conn,
				   const mg_request_info* request_info,
				   const uri& processed_uri,
				   response& r) {
  persistent_data * pd = static_cast<persistent_data*>(request_info->user_data);

  std::string qualified_collection(pd->db_name);
  qualified_collection += ".";
  qualified_collection += processed_uri.collection;

  if(processed_uri.collection.length() == 0 || processed_uri.id.length() == 0) {
    r.status_code = 405;
  } else {
    mongo::OID id(processed_uri.id);
    mongo::Query q(BSON("_id" << id));
    std::auto_ptr<mongo::DBClientCursor> cursor
      = pd->db.query(qualified_collection, q);
    if(!cursor->more()) {
      r.status_code = 404;
    } else {
      const char *clh = mg_get_header(conn, "Content-Length");

      if(clh == NULL) {
	r.status_code = 411;
	r.contents = "Content-Length header malformed or not set.";
      } else {
	int cl = atoi(clh);
	char * c = static_cast<char*>(malloc(cl+1));
	if(c == NULL) {
	  r.status_code = 413;
	} else {
	  mg_read(conn, c, cl);
	  c[cl] = '\0';
	  std::cout << c << std::endl;
	  mongo::BSONObj b(mongo::fromjson(c));
	  std::cout << b << std::endl;

	  if(b["_id"].OID() != id) {
            r.status_code = 409;
	  } else {
	    pd->db.update(qualified_collection, q, b);
	    if(pd->db.getLastError() != "") {
	      /* Bad */ 
	      r.status_code = 500;
	      r.type = "text/plain";
	      r.contents = "Update failed";
	    } else {
	      /* Good */
	      std::string location("/");
	      location += processed_uri.collection;
	      location += "/";
	      location += b["_id"].OID().str();

	      r.status_code = 303;
	      r.set_header("Location", location);
	      r.contents = "Resource updated";
	    }
	  }
	}
      }
    }
  }
}

static void delete_request_handler(mg_connection* conn,
                                const mg_request_info* request_info,
				const uri& processed_uri,
				response& r) {
  persistent_data * pd = static_cast<persistent_data*>(request_info->user_data);

  std::string qualified_collection(pd->db_name);
  qualified_collection += ".";
  qualified_collection += processed_uri.collection;

  if(processed_uri.collection.length() == 0 || processed_uri.id.length() == 0) {
    r.status_code = 405;
  } else {
    mongo::OID id(processed_uri.id);
    mongo::Query q(BSON("_id" << id));
    std::auto_ptr<mongo::DBClientCursor> cursor
      = pd->db.query(qualified_collection, q);
    if(!cursor->more()) {
      r.status_code = 404;
    } else {
      pd->db.remove(qualified_collection, q, true);
      if(pd->db.getLastError() != "") {
        /* Bad */ 
	r.status_code = 500;
	r.type = "text/plain";
	r.contents = "Deletion failed.";
      } else {
        /* Good */
	r.status_code = 200;
      }
    }
  }
}

void request_handler(mg_connection* conn, const mg_request_info* request_info, response& r) {
  uri processed_uri;

  process_uri(request_info->uri, processed_uri);

  std::cout << request_info->request_method << std::endl;

  if (!request_info->is_ssl) {
    redirect_to_ssl(conn, request_info, r);
  } else if(strcmp(request_info->request_method, "GET") == 0) {
    get_request_handler(conn, request_info, processed_uri, r);
  } else if(strcmp(request_info->request_method, "POST") == 0) {
    post_request_handler(conn, request_info, processed_uri, r);
  } else if(strcmp(request_info->request_method, "PUT") == 0) {
    put_request_handler(conn, request_info, processed_uri, r);
  } else if(strcmp(request_info->request_method, "DELETE") == 0) {
    delete_request_handler(conn, request_info, processed_uri, r);
  } else {
    r.status_code = 405;
  }
}

