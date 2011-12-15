/* MongoDB REST over HTTP interface
 *
 * Copyright 2011 Alex Chamberlain
 * Licensed under the MIT licence with no-advertise clause. See LICENCE.
 *
 */

struct uri {
  std::string collection;
  std::string id;

  uri() : collection(), id() {
    /* Empty */
  }
};

std::ostream& operator<<(std::ostream& out, const uri & u) {
  out << "{\n  collection: " << u.collection << "\n  id: " << u.id << "\n}";
  return out;
}
