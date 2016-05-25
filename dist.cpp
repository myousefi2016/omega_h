void Dist::set_comm(CommPtr parent_comm) {
  parent_comm_ = parent_comm;
}

void Dist::set_dest_ranks(Read<I32> items2ranks) {
  auto content2items = sort_by_keys(items2ranks);
  auto content2ranks = unmap(content2items, items2ranks, 1);
  Write<I8> jumps(content2ranks.size());
  auto mark_jumps = LAMBDA(LO i) {
    jumps[i] = (content2ranks[i] != content2ranks[i + 1]);
  };
  parallel_for(jumps.size() - 1, mark_jumps);
  if (jumps.size()) {
    jumps.set(jumps.size() - 1, 1);
  }
  auto content2msgs = offset_scan<LO>(Read<I8>(jumps));
  auto nmsgs = content2msgs.last();
  Write<I32> msgs2ranks(nmsgs);
  auto log_ranks = LAMBDA(LO i) {
    if (jumps[i]) {
      msgs2ranks[content2msgs[i]] = content2ranks[i];
    }
  };
  parallel_for(jumps.size(), log_ranks);
  Write<LO> msgs2content(nmsgs + 1);
  msgs2content.set(0, 0);
  auto log_ends = LAMBDA(LO i) {
    if (jumps[i]) {
      msgs2content[content2msgs[i] + 1] = i + 1;
    }
  };
  parallel_for(jumps.size(), log_ends);
  items2content_[F] = invert_permutation(content2items);
  msgs2content_[F] = msgs2content;
  msgs2ranks_[F] = msgs2ranks;
  comm_[F] = parent_comm_->graph(msgs2ranks);
  comm_[R] = parent_comm_->graph_adjacent(
      comm_[F]->destinations(), comm_[F]->sources());
  msgs2ranks_[R] = comm_[R]->sources();
  auto fdegrees = get_degrees(msgs2content_[F]);
  auto rdegrees = comm_[F]->alltoall(fdegrees);
  msgs2content_[R] = offset_scan<LO>(rdegrees);
}

void Dist::set_dest_idxs(Read<I32> fitems2rroots, LO nrroots) {
  auto rcontent2rroots = exch(fitems2rroots, 1);
  map::invert_by_sorting(rcontent2rroots, nrroots,
      roots2items_[R], items2content_[R]);
}

void Dist::set_roots2items(LOs froots2fitems) {
  roots2items_[F] = froots2fitems;
}

Dist Dist::invert() const {
  Dist out;
  out.parent_comm_ = parent_comm_;
  for (Int i = 0; i < 2; ++i) {
    out.roots2items_[i] = roots2items_[1 - i];
    out.items2content_[i] = items2content_[1 - i];
    out.msgs2content_[i] = msgs2content_[1 - i];
    out.msgs2ranks_[i] = msgs2ranks_[1 - i];
    out.comm_[i] = comm_[1 - i];
  }
  return out;
}

template <typename T>
Read<T> Dist::exch(Read<T> data, Int width) const {
  if (roots2items_[F].size()) {
    data = expand(data, roots2items_[F], width);
  }
  if (items2content_[F].size()) {
    data = permute(data, items2content_[F], width);
  }
  auto sendcounts = multiply_each_by(width,
      get_degrees(msgs2content_[F]));
  auto recvcounts = multiply_each_by(width,
      get_degrees(msgs2content_[F]));
  auto sdispls = offset_scan<LO>(sendcounts);
  auto rdispls = offset_scan<LO>(recvcounts);
  data = comm_[F]->alltoallv(data,
      sendcounts, sdispls,
      recvcounts, rdispls);
  if (items2content_[R].size()) {
    data = unmap(items2content_[R], data, width);
  }
  return data;
}

template Read<I8> Dist::exch(Read<I8> data, Int width) const;
template Read<I32> Dist::exch(Read<I32> data, Int width) const;
template Read<I64> Dist::exch(Read<I64> data, Int width) const;
template Read<Real> Dist::exch(Read<Real> data, Int width) const;