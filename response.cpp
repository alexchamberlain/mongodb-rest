#include "response.h"

#include <sstream>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <zlib.h>

void response::send() {
    if(!dont_send) {
      std::ostringstream r;

      const char *aeh = mg_get_header(conn, "Accept-Encoding");

      int data_length = contents.length();
      unsigned char * data = static_cast<unsigned char *>(malloc(data_length));
      std::copy(contents.begin(), contents.end(), data);

      if(aeh == NULL) {
        /* Do Nothing */
	std::cout << "No Accept-Encoding header." << std::endl;
      } else {
        std::vector<std::string> methods;
	boost::algorithm::split(methods, aeh, boost::algorithm::is_any_of(" ,"));

        for(std::vector<std::string>::const_iterator i = methods.begin();
	    i != methods.end();
	    ++i) {
          std::cout << *i << std::endl;
	}


	if(std::find(methods.begin(), methods.end(), "gzip") == methods.end()) {
          /* Do Nothing */
	  std::cout << "Encoding methods not supported." << std::endl;
	} else {
          z_stream z;
	  z.zalloc    = Z_NULL;
	  z.zfree     = Z_NULL;
	  z.opaque    = Z_NULL;
	  z.total_out = 0;
	  z.next_in   = data;
	  z.avail_in  = data_length;

	  int initError = deflateInit2(&z,
	                               Z_DEFAULT_COMPRESSION,
				       Z_DEFLATED,
				       (15+16), // Use gzip compression
				       8,
				       Z_DEFAULT_STRATEGY);  
	  if(initError != Z_OK) {
            /*switch(initError) {
              case Z_MEM_ERROR:

	      
	    }*/
	    /* There has been an error... */
	  } else {
	    int N = deflateBound(&z, z.avail_in);
            unsigned char * compressedData = static_cast<unsigned char*>(malloc(N));

	    int z_status;

            do {
	      z.next_out  = compressedData + z.total_out;
	      z.avail_out = N - z.total_out;

	      z_status = deflate(&z, Z_FINISH);

	    } while(z_status == Z_OK);
            
	    if(z_status != Z_STREAM_END) {
              /* There has been an error... */
	    } else {
              deflateEnd(&z);
	      free(data);
	      data = compressedData;
	      compressedData = NULL;
	      data_length = N;
	      set_header("Content-Encoding", "gzip");
	    }
	  }
	}
      }
      
      r << "HTTP/1.1 " << status_code << "\r\n"
	<< "Content-Type: " << type << "\r\n"
	<< "Content-Length: " << data_length << "\r\n";

      for(std::map<std::string, std::string>::const_iterator i = headers.begin();
          i != headers.end();
	  ++i) {
        r << i->first << ": " << i->second << "\r\n";
      }

      r << "\r\n";

      //std::cout << r.str() << std::endl;

      mg_write(conn, r.str().c_str(), r.str().length());
      mg_write(conn, data, data_length);

      free(data);
    }
}
