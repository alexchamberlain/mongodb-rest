REST over HTTP Server for MongoDB
=================================

Aims
----

### Security
Security is really important in Web Apps. We will support some form of ACL and
built in support against common attacks.

### Performance
This should perform better than any other REST interface onto MongoDB.

Features
--------

* GZIP Compression

Building
--------

make clean && make && make run

TODO
----

* Abstract away the HTTP transport from the request handling to allow
  integration with Apache/Nginx etc.
* Session support
* Develop a custom iostream. Intelligently adds headers, compresses if
  allowable. Basis for custom server.
* Read config from rest database.


Licence
-------

Copyright 2011 Alex Chamberlain
Licensed under MIT licence with no-advertise clause. See LICENCE.
