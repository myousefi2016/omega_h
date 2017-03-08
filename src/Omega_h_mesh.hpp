#ifndef OMEGA_H_MESH_HPP
#define OMEGA_H_MESH_HPP

#include <string>
#include <vector>

#include <Omega_h_adj.hpp>
#include <Omega_h_comm.hpp>
#include <Omega_h_dist.hpp>
#include <Omega_h_library.hpp>
#include <Omega_h_tag.hpp>

namespace Omega_h {

enum { DIMS = OMEGA_H_DIMS };

enum {
  VERT = OMEGA_H_VERT,
  EDGE = OMEGA_H_EDGE,
  TRI = OMEGA_H_TRI,
  TET = OMEGA_H_TET
};

namespace inertia {
struct Rib;
}

class Mesh {
 public:
  Mesh(Library* library);
  Library* library() const;
  void set_comm(CommPtr const& comm);
  void set_dim(Int dim);
  void set_verts(LO nverts);
  void set_ents(Int dim, Adj down);
  void keep_canonical_globals(bool yn);
  CommPtr comm() const;
  Omega_h_Parting parting() const;
  inline Int dim() const {
    OMEGA_H_CHECK(0 <= dim_ && dim_ <= 3);
    return dim_;
  }
  LO nents(Int dim) const;
  LO nelems() const;
  LO ntets() const;
  LO ntris() const;
  LO nedges() const;
  LO nverts() const;
  GO nglobal_ents(Int dim);
  template <typename T>
  void add_tag(
      Int dim, std::string const& name, Int ncomps, Int xfer, Int outflags);
  template <typename T>
  void add_tag(Int dim, std::string const& name, Int ncomps, Int xfer,
      Int outflags, Read<T> array, bool internal = false);
  template <typename T>
  void set_tag(
      Int dim, std::string const& name, Read<T> array, bool internal = false);
  TagBase const* get_tagbase(Int dim, std::string const& name) const;
  template <typename T>
  Tag<T> const* get_tag(Int dim, std::string const& name) const;
  template <typename T>
  Read<T> get_array(Int dim, std::string const& name) const;
  void remove_tag(Int dim, std::string const& name);
  bool has_tag(Int dim, std::string const& name) const;
  Int ntags(Int dim) const;
  TagBase const* get_tag(Int dim, Int i) const;
  bool has_ents(Int dim) const;
  bool has_adj(Int from, Int to) const;
  Adj get_adj(Int from, Int to) const;
  Adj ask_down(Int from, Int to);
  LOs ask_verts_of(Int dim);
  LOs ask_elem_verts();
  Adj ask_up(Int from, Int to);
  Graph ask_star(Int dim);
  Graph ask_dual();

 public:
  typedef std::shared_ptr<TagBase> TagPtr;
  typedef std::shared_ptr<Adj> AdjPtr;
  typedef std::shared_ptr<Dist> DistPtr;
  typedef std::shared_ptr<inertia::Rib> RibPtr;

 private:
  typedef std::vector<TagPtr> TagVector;
  typedef TagVector::iterator TagIter;
  typedef TagVector::const_iterator TagCIter;
  TagIter tag_iter(Int dim, std::string const& name);
  TagCIter tag_iter(Int dim, std::string const& name) const;
  void check_dim(Int dim) const;
  void check_dim2(Int dim) const;
  void add_adj(Int from, Int to, Adj adj);
  Adj derive_adj(Int from, Int to);
  Adj ask_adj(Int from, Int to);
  void react_to_set_tag(Int dim, std::string const& name);
  Int dim_;
  CommPtr comm_;
  Int parting_;
  Int nghost_layers_;
  LO nents_[DIMS];
  TagVector tags_[DIMS];
  AdjPtr adjs_[DIMS][DIMS];
  Remotes owners_[DIMS];
  DistPtr dists_[DIMS];
  RibPtr rib_hints_;
  bool keeps_canonical_globals_;
  Library* library_;

 public:
  void add_coords(Reals array);
  Reals coords() const;
  void set_coords(Reals const& array);
  Read<GO> ask_globals(Int dim);
  void reset_globals();
  Reals ask_lengths();
  Reals ask_qualities();
  void set_owners(Int dim, Remotes owners);
  Remotes ask_owners(Int dim);
  Read<I8> owned(Int dim);
  Dist ask_dist(Int dim);
  Int nghost_layers() const;
  void set_parting(Omega_h_Parting parting, Int nlayers, bool verbose);
  void set_parting(Omega_h_Parting parting, bool verbose = false);
  void migrate(Remotes new_elems2old_owners, bool verbose = false);
  void reorder();
  void balance(bool predictive = false);
  Graph ask_graph(Int from, Int to);
  template <typename T>
  Read<T> sync_array(Int ent_dim, Read<T> a, Int width);
  template <typename T>
  Read<T> sync_subset_array(
      Int ent_dim, Read<T> a_data, LOs a2e, T default_val, Int width);
  template <typename T>
  Read<T> reduce_array(Int ent_dim, Read<T> a, Int width, Omega_h_Op op);
  template <typename T>
  Read<T> owned_array(Int ent_dim, Read<T> a, Int width);
  void sync_tag(Int dim, std::string const& name);
  void reduce_tag(Int dim, std::string const& name, Omega_h_Op op);
  bool operator==(Mesh& other);
  Real min_quality();
  Real max_length();
  bool could_be_shared(Int ent_dim) const;
  bool owners_have_all_upward(Int ent_dim) const;
  Mesh copy_meta() const;
  bool keeps_canonical_globals() const;
  RibPtr rib_hints() const;
  void set_rib_hints(RibPtr hints);
  Real imbalance(Int ent_dim = -1) const;
};

bool can_print(Mesh* mesh);

Real repro_sum_owned(Mesh* mesh, Int dim, Reals a);

#define OMEGA_H_EXPL_INST_DECL(T)                                              \
  extern template Tag<T> const* Mesh::get_tag<T>(                              \
      Int dim, std::string const& name) const;                                 \
  extern template Read<T> Mesh::get_array<T>(Int dim, std::string const& name) \
      const;                                                                   \
  extern template void Mesh::add_tag<T>(                                       \
      Int dim, std::string const& name, Int ncomps, Int xfer, Int outflags);   \
  extern template void Mesh::add_tag<T>(Int dim, std::string const& name,      \
      Int ncomps, Int xfer, Int outflags, Read<T> array, bool internal);       \
  extern template void Mesh::set_tag(                                          \
      Int dim, std::string const& name, Read<T> array, bool internal);         \
  extern template Read<T> Mesh::sync_array(Int ent_dim, Read<T> a, Int width); \
  extern template Read<T> Mesh::owned_array(                                   \
      Int ent_dim, Read<T> a, Int width);                                      \
  extern template Read<T> Mesh::sync_subset_array(                             \
      Int ent_dim, Read<T> a_data, LOs a2e, T default_val, Int width);         \
  extern template Read<T> Mesh::reduce_array(                                  \
      Int ent_dim, Read<T> a, Int width, Omega_h_Op op);
OMEGA_H_EXPL_INST_DECL(I8)
OMEGA_H_EXPL_INST_DECL(I32)
OMEGA_H_EXPL_INST_DECL(I64)
OMEGA_H_EXPL_INST_DECL(Real)
#undef OMEGA_H_EXPL_INST_DECL

}

#endif