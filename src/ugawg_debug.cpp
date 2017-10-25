#include <Omega_h_adapt.hpp>
#include <Omega_h_cmdline.hpp>
#include <Omega_h_file.hpp>
#include <Omega_h_library.hpp>
#include <Omega_h_array_ops.hpp>

#ifdef OMEGA_H_USE_EGADS
#include <Omega_h_egads.hpp>
#endif

#include <iostream>

int main(int argc, char** argv) {
  auto lib = Omega_h::Library(&argc, &argv);
  Omega_h::CmdLine cmdline;
  cmdline.add_arg<std::string>("mesh_in.osh");
#ifdef OMEGA_H_USE_EGADS
  auto& model_flag = cmdline.add_flag("--model", "optional EGADS model");
  model_flag.add_arg<std::string>("model.step");
#endif
  if (!cmdline.parse_final(lib.world(), &argc, argv)) return -1;
  auto path_in = cmdline.get<std::string>("mesh_in.osh");
  Omega_h::Mesh mesh(&lib);
  std::cout << "reading in " << path_in << '\n';
  Omega_h::binary::read(path_in, lib.world(), &mesh);
#ifdef OMEGA_H_USE_EGADS
  Omega_h::Egads* eg = nullptr;
  if (cmdline.parsed("--model")) {
    auto model_path = cmdline.get<std::string>("--model", "model.step");
    std::cout << "reading in " << model_path << '\n';
    eg = Omega_h::egads_load(model_path);
  }
#endif
  Omega_h::AdaptOpts opts(&mesh);
#ifdef OMEGA_H_USE_EGADS
  opts.egads_model = eg;
#endif
  auto minqual = mesh.min_quality();
  auto maxlen = mesh.max_length();
  opts.min_quality_allowed = minqual;
  opts.max_length_allowed = Omega_h::max2(maxlen, opts.max_length_desired * 2.0);
  std::cout << "max_length_allowed(" << opts.max_length_allowed << ") = max("
    << "maxlen(" << maxlen << "), max_length_desired*2(" << opts.max_length_desired * 2.0 << "))\n";
  opts.verbosity = Omega_h::EXTRA_STATS;
  opts.nsliver_layers = 10;
  opts.min_quality_desired = Omega_h::min2(minqual + 0.1, 1.0);
  auto es2s = mesh.ask_down(3, 2).ab2b;
  auto min_es2s = Omega_h::get_min(es2s);
  std::cout << "min_es2s " << min_es2s << '\n';
  Omega_h::adapt(&mesh, opts);
  mesh.remove_tag(Omega_h::VERT, "metric");
#ifdef OMEGA_H_USE_EGADS
  if (eg != nullptr) {
    Omega_h::egads_free(eg);
  }
#endif
}
