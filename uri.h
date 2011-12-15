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
