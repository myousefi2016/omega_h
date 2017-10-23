#include <Omega_h_adapt.hpp>
#include <Omega_h_mesh.hpp>
#include <Omega_h_metric.hpp>

#ifdef OMEGA_H_USE_EGADS
#include <Omega_h_egads.hpp>
#endif

#include <iostream>

namespace Omega_h {

static void compute_metric(Omega_h::Mesh* mesh) {
  auto metrics = Omega_h::get_pure_implied_metrics(mesh);
  metrics = Omega_h::limit_metric_gradation(mesh, metrics, 1.0);
  mesh->remove_tag(Omega_h::VERT, "metric");
  mesh->add_tag(
      Omega_h::VERT, "metric", Omega_h::symm_ncomps(mesh->dim()), metrics);
}

void fix(Mesh* mesh
    , AdaptOpts const& adapt_opts
    , bool verbose
    ) {
  verbose = verbose && can_print(mesh);
  if (verbose) std::cout << "computing implied metric tag\n";
  compute_metric(mesh);
  if (verbose) std::cout << "computing minimum quality\n";
  auto minqual = mesh->min_quality();
  auto maxlen = mesh->max_length();
  if (verbose) std::cout << "minimum quality " << minqual << '\n';
  if (verbose) std::cout << "maximum length " << maxlen << '\n';
  Omega_h::AdaptOpts opts = adapt_opts;
  while (true) {
    auto minqual_old = minqual;
    opts.min_quality_allowed = minqual;
    opts.max_length_allowed = max2(maxlen, opts.min_length_desired * 2.0);
    opts.verbosity = Omega_h::EXTRA_STATS;
    opts.nsliver_layers = 10;
    opts.min_quality_desired = Omega_h::min2(minqual + 0.1, 1.0);
    Omega_h::adapt(mesh, opts);
    minqual = mesh->min_quality();
    maxlen = mesh->max_length();
    if (verbose) std::cout << "minimum quality " << minqual << '\n';
    if (verbose) std::cout << "maximum length " << maxlen << '\n';
    if (minqual == minqual_old) break;  // stalled
  }
}

}