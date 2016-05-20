Mesh::Mesh():
  dim_(-1) {
  for (Int i = 0; i <= 3; ++i)
    nents_[i] = -1;
}

void Mesh::set_dim(Int dim) {
  CHECK(dim_ == -1);
  CHECK(dim >= 2);
  CHECK(dim <= 3);
  dim_ = dim;
}

void Mesh::set_verts(LO nverts) {
  nents_[VERT] = nverts;
}

void Mesh::set_ents(Int dim, Adj down) {
  check_dim(dim);
  CHECK(!has_ents(dim));
  LOs hl2l = down.ab2b;
  CHECK(hl2l.size() % simplex_degrees[dim][dim - 1] == 0);
  nents_[dim] = hl2l.size() / simplex_degrees[dim][dim - 1];
  add_adj(dim, dim - 1, down);
}

Int Mesh::dim() const {
  return dim_;
}

LO Mesh::nents(Int dim) const {
  check_dim2(dim);
  return nents_[dim];
}

template <typename T>
void Mesh::add_tag(Int dim, std::string const& name, Int ncomps) {
  check_dim2(dim);
  CHECK(!has_tag(dim, name));
  CHECK(ncomps >= 0);
  CHECK(tags_[dim].size() < static_cast<std::size_t>(INT8_MAX));
  tags_[dim].push_back(TagPtr(new Tag<T>(name, ncomps)));
}

template <typename T>
void Mesh::set_tag(Int dim, std::string const& name, Read<T> data) {
  CHECK(has_tag(dim, name));
  Tag<T>* tag = to<T>(tag_iter(dim, name)->get());
  CHECK(data.size() == nents(dim) * tag->ncomps());
  tag->set_data(data);
}

template <typename T>
Tag<T> const& Mesh::get_tag(Int dim, std::string const& name) const {
  check_dim2(dim);
  CHECK(has_tag(dim, name));
  return *(to<T>(tag_iter(dim, name)->get()));
}

void Mesh::remove_tag(Int dim, std::string const& name) {
  check_dim2(dim);
  CHECK(has_tag(dim, name));
  tags_[dim].erase(tag_iter(dim, name));
}

bool Mesh::has_tag(Int dim, std::string const& name) const {
  check_dim2(dim);
  return tag_iter(dim, name) != tags_[dim].end();
}

Int Mesh::count_tags(Int dim) const {
  check_dim2(dim);
  return static_cast<Int>(tags_[dim].size());
}

TagBase const* Mesh::get_tag(Int dim, Int i) const {
  check_dim2(dim);
  CHECK(0 <= i);
  CHECK(i <= count_tags(dim));
  return tags_[dim][static_cast<std::size_t>(i)].get();
}

bool Mesh::has_ents(Int dim) const {
  check_dim(dim);
  return nents_[dim] >= 0;
}

bool Mesh::has_adj(Int from, Int to) const {
  check_dim(from);
  check_dim(to);
  return bool(adjs_[from][to]);
}

Adj Mesh::get_adj(Int from, Int to) const {
  check_dim2(from);
  check_dim2(to);
  CHECK(has_adj(from, to));
  return *(adjs_[from][to]);
}

Adj Mesh::ask_adj(Int from, Int to) {
  check_dim2(from);
  check_dim2(to);
  if (has_adj(from, to)) {
    return get_adj(from,to);
  }
  Adj derived = derive_adj(from, to);
  adjs_[from][to] = AdjPtr(new Adj(derived));
  return derived;
}

Read<GO> Mesh::ask_globals(Int dim) {
  check_dim2(dim);
  if (!has_tag(dim, "global")) {
    return Read<GO>(nents(dim), 0, 1);
  }
  return get_tag<GO>(dim, "global").data();
}

struct HasName {
  std::string const& name_;
  HasName(std::string const& name):name_(name) {}
  bool operator()(std::shared_ptr<TagBase> const& a) {
    return a->name() == name_;
  }
};

Mesh::TagIter Mesh::tag_iter(Int dim, std::string const& name) {
  return std::find_if(begin(tags_[dim]), end(tags_[dim]), HasName(name));
}

Mesh::TagCIter Mesh::tag_iter(Int dim, std::string const& name) const {
  return std::find_if(begin(tags_[dim]), end(tags_[dim]), HasName(name));
}

void Mesh::check_dim(Int dim) const {
  CHECK(0 <= dim);
  CHECK(dim <= this->dim());
}

void Mesh::check_dim2(Int dim) const {
  check_dim(dim);
  CHECK(has_ents(dim));
}

void Mesh::add_adj(Int from, Int to, Adj adj) {
  check_dim2(from);
  check_dim2(to);
  if (to < from) {
    CHECK(adj.a2ab.size() == 0);
    if (to == VERT)
      CHECK(adj.codes.size() == 0);
    CHECK(adj.ab2b.size() == nents(from) * simplex_degrees[from][to]);
  } else {
    if (from < to) {
      CHECK(adj.ab2b.size() == nents(to) * simplex_degrees[to][from]);
    }
    CHECK(adj.a2ab.size() == nents(from) + 1);
  }
  adjs_[from][to] = AdjPtr(new Adj(adj));
}

Adj Mesh::derive_adj(Int from, Int to) {
  check_dim(from);
  check_dim2(to);
  if (from < to) {
    Adj down = ask_adj(to, from);
    Int nlows_per_high = simplex_degrees[to][from];
    LO nlows = nents(from);
    Read<GO> high_globals = ask_globals(to);
    Adj up = invert(down, nlows_per_high, nlows, high_globals);
    return up;
  } else if (to < from) {
    CHECK(to + 1 < from);
    Adj h2m = ask_adj(from, to + 1);
    Adj m2l = ask_adj(to + 1, to);
    Adj h2l = transit(h2m, m2l, from, to);
    return h2l;
  } else {
    if (from == VERT && to == VERT) {
      return verts_across_edges(ask_adj(EDGE,VERT), ask_adj(VERT,EDGE));
    }
    if (from == EDGE && to == EDGE) {
      CHECK(dim() >= 2);
      Graph g = edges_across_tris(ask_adj(TRI,EDGE),ask_adj(EDGE,TRI));
      if (dim() == 3) {
        g = add_edges(g, edges_across_tets(
              ask_adj(TET,EDGE), ask_adj(EDGE,TET)));
      }
      return g;
    }
    /* todo: element-to-element dual */
  }
  fail("can't derive adjacency from %s to %s\n",
      plural_names[from], plural_names[to]);
}

#define INST_T(T) \
template Tag<T> const& Mesh::get_tag<T>( \
    Int dim, std::string const& name) const; \
template void Mesh::add_tag<T>(Int dim, std::string const& name, Int ncomps); \
template void Mesh::set_tag(Int dim, std::string const& name, Read<T> data);
INST_T(I8)
INST_T(I32)
INST_T(I64)
INST_T(Real)
#undef INST_T