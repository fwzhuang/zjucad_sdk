//! @file operation.hh
//! @brief implementation of the basic operations on half_edge,
//! included into operation.h
//! @author Jin Huang, Xianzhong Fang
//! @date 201406-

template <typename TMPL>
bool
is_valid(const cont_tmpl<TMPL> &c, const typename TMPL::const_iterator &i)
{
  typedef TMPL cont_t;
  const cont_t &cont = c();
  for(typename cont_t::const_iterator j = cont.begin(); j != cont.end(); ++j)
    if(i == j)
      return true;
  return false;
}

template <typename TMPL>
bool
is_valid(const mesh_tmpl<TMPL> &m, const typename TMPL::verts_t::const_iterator &vi)
{
  return is_valid(m().verts(), vi) &&
    is_valid(m().edges(), vi->edge());
}

template <typename TMPL>
bool
is_valid(const mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &ei)
{
  return is_valid(m().edges(), ei) &&
    is_valid(m().edges(), ei->prev()) &&
    is_valid(m().edges(), ei->next()) &&
    is_valid(m().edges(), ei->oppo()) && // the convension of null face for boundary
    is_valid(m().verts(), ei->vert()) &&
    (ei->face()?is_valid(m().faces(), ei->face()):true);
}

template <typename TMPL>
bool
is_valid(const mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator &fi)
{
  return is_valid(m().faces(), fi) &&
    is_valid(m().edges(), fi->edge());
}

//! @note may be insert iterator will be better.
template <typename TMPL, typename VC, typename EC, typename FC>
void
find_invalid(const mesh_tmpl<TMPL> &m, VC &vc, EC &ec, FC &fc)
{
  for(auto i = m().verts().begin(); i != m().verts().end(); ++i)
    if(!is_valid(m, i))
      vc.push_back(i);
  for(auto i = m().edges().begin(); i != m().edges().end(); ++i)
    if(!is_valid(m, i))
      ec.push_back(i);
  for(auto i = m().faces().begin(); i != m().faces().end(); ++i)
    if(!is_valid(m, i))
      fc.push_back(i);
}

template <typename TMPL>
bool
is_valid(const mesh_tmpl<TMPL> &m)
{
  std::vector<typename TMPL::verts_t::const_iterator> vc;
  std::vector<typename TMPL::edges_t::const_iterator> ec;
  std::vector<typename TMPL::faces_t::const_iterator> fc;
  find_invalid(m, vc, ec, fc);
  return (vc.size() + ec.size() + fc.size() == 0);
}

//! @brief add a polygon face to mesh without setting opposite edge
template <typename TMPL, typename ITR>
typename TMPL::faces_t::const_iterator
add_face(mesh_tmpl<TMPL> &m, const ITR &vert_loop_beg, std::size_t n)
{
  usr_session::default_usr_session ses;
  return add_face(m, vert_loop_beg, n, ses);
}
template <typename TMPL, typename ITR, typename SES>
typename TMPL::faces_t::const_iterator
add_face(mesh_tmpl<TMPL> &m, const ITR &vert_loop_beg, std::size_t n, SES & ses)
{
  usr_operation<SES> op(ses);
    typename TMPL::faces_t::const_iterator fi = op.add_face(m, typename TMPL::face_t());
  typedef std::vector<typename TMPL::edges_t::const_iterator> e_con;
  e_con edges;
  edges.reserve(n);
  ITR vi = vert_loop_beg;
  for(; edges.size() < n; ++vi) {
    typename TMPL::edges_t::const_iterator ei = op.add_edge(m, typename TMPL::edge_t());
    assert(ei);
    edges.push_back(ei);
    m()[ei].vert() = *vi; // the edge points to this vertex
    m()[ei].face() = fi;  // the edge adjacent to this face
    m()[*vi].edge() = ei; // ei points to vi
  }
  vi = vert_loop_beg;
  for(std::size_t i = 0; i < edges.size(); ++i, ++vi) {
    m()[edges[i]].next() = edges[(i+1)%edges.size()];
    m()[edges[i]].prev() = edges[(i+edges.size()-1)%edges.size()]; // be careful of size_t
  }
  assert(edges[0]);
  m()[fi].edge() = edges[0];
  return fi;
}

//! @brief NOTICE append to border_edges.
template <typename TMPL, typename C>
void
find_border(const cont_tmpl<TMPL> &edges, C &border_edges)
{
  typename C::value_type ci;
  for(ci = edges().begin(); ci != edges().end(); ++ci) {
    if(!(ci->face()))
      border_edges.push_back(ci);
  }
}

//! @brief a boundary vert is a vert with at least one adjacent boundary edge
template <typename TMPL>
bool is_boundary(mesh_tmpl<TMPL>& m, const typename TMPL::verts_t::const_iterator& v)
{
  return !(v->edge()->face());
}
//! @brief a boundary edge is an edge with null face
template <typename TMPL>
bool is_boundary(mesh_tmpl<TMPL>& m, const typename TMPL::edges_t::const_iterator& e)
{
  return !(e->face());
}
//! @brief a boundary face is a face with at least one edge, which has a opposite boundary edge
template <typename TMPL>
bool is_boundary(mesh_tmpl<TMPL>& m, const typename TMPL::faces_t::const_iterator& f)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = f->edge();
  do {
    if (!e->oppo()->face()) return true;
    e = e->next();
  } while (e != f->edge());

  return true;
}

//! @brief an isolated vert is a vert that it doesn't have adjacent edge.
template <typename TMPL>
    bool is_isolated(const typename TMPL::verts_t::const_iterator& v)
{
  return !v->edge();
}
//! @brief an isolated edge is a edge that it and its oppo edge doesn't have adjacent face.
template <typename TMPL>
    bool is_isolated(const typename TMPL::edges_t::const_iterator& e)
{
  return (!e->face() && !e->oppo()->face());
}
//! @brief an isolated face is a face that it doesn't have adjacent face.
template <typename TMPL>
    bool is_isolated(const typename TMPL::faces_t::const_iterator& f)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = f->edge();
  do {
    if (e->oppo()->face()) return false;
    e = e->next();
  } while (e != f->edge());
  return true;
}

//! @brief delete the vertex and the related edges and faces
template <typename TMPL>
int del(mesh_tmpl<TMPL> &m, const typename TMPL::verts_t::const_iterator& v)
{
  usr_session::default_usr_session ses;
  return del(m, v, ses);
}
template <typename TMPL, typename SES >
int del(mesh_tmpl<TMPL> &m, const typename TMPL::verts_t::const_iterator& v, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;

  vert_itr_t vv = v; // copy because v is a reference
  
  std::vector<face_itr_t> vf;
  vert_adj_faces<TMPL>(vv, vf);
  for (auto it = vf.begin(); it != vf.end(); ++it) {
    if (del(m, *it)) return 1;
  }

  std::vector<edge_itr_t> ve;
  vert_adj_out_edges<TMPL>(vv, ve);
  for (auto it = ve.begin(); it != ve.end(); ++it) {
    if (del(m, *it)) return 1;
  }

  op.del(m, vv);

  return 0;
}

//! @brief delete the edge through the related faces
template <typename TMPL>
int del(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator& e)
{
  usr_session::default_usr_session ses;
  return del(m, e, ses);
}
template <typename TMPL, typename SES >
int del(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator& e, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;

  edge_itr_t ee = e; // copy, e is a reference, the next operation may modify it
  auto ee_op = ee->oppo();
  if (ee->face()) {
    if (del(m, ee->face())) return 1;
  }
  if (ee_op->face()) { // can't use e->oppo()->face() directly, because the e may be deleted
    if (del(m, ee_op->face())) return 1;
  }

  // adjust the edge that vert point to
  vert_itr_t v = ee->vert();
  if (v->edge() == ee) {
    m()[v].edge() = ee->oppo()->prev();
    if (v->edge() == ee)
    {
      m()[v].edge() = edge_itr_t(); // set null
    }
  }
  v = ee->oppo()->vert();
  if (v->edge() == ee->oppo()) {
    m()[v].edge() = ee->prev();
    if (v->edge() == ee->oppo())
    {
      m()[v].edge() = edge_itr_t();
    }
  }

  // adjust the next and prev edges that the adjacent edges
  m()[ee->next()].prev() = ee->oppo()->prev();
  m()[ee->oppo()->prev()].next() = ee->next();
  m()[ee->prev()].next() = ee->oppo()->next();
  m()[ee->oppo()->next()].prev() = ee->prev();

  op.del(m, ee->oppo());
  op.del(m, ee);
  
  return 0;
}

//! @brief del face
//! @NOTE only delete face
template <typename TMPL>
int del(mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator& f)
{
  usr_session::default_usr_session ses;
  return del(m, f, ses);
}
template <typename TMPL, typename SES>
int del(mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator& f, SES & ses)
{
  usr_operation<SES> op(ses);
  assert(f);
  using namespace std;
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  vector<edge_itr_t> fe;
  face_adj_edges<TMPL>(f, fe);

  const typename TMPL::faces_t::const_iterator ff = f;
  // set null face point
  for (auto eit = fe.begin(); eit != fe.end(); ++eit) {
    m()[(*eit)].face() = face_itr_t(); // null
    assert(!(*eit)->face());
  }

  assert(ff);
  op.del(m, ff);

  return 0;
}

//! @brief set opposite and boundary edge
//! used after adding all faces, the result is a halfedge mesh
template <typename TMPL>
int set_opposite_and_boundary_edge(mesh_tmpl<TMPL> &mt)
{
  using namespace std;
  typedef TMPL mesh_t;
  typedef const typename TMPL::vert_t* vert_ptr_t;
  typedef const typename TMPL::edge_t edge_t;
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  
  map<pair<vert_ptr_t, vert_ptr_t>, pair<edge_itr_t,edge_itr_t> > edges_map;

  mesh_t &m = mt();
  //edge_itr_t cei;
  edge_itr_t cei;
  for (cei=m.edges().begin(); cei!=m.edges().end(); ++cei) {
    pair<vert_ptr_t, vert_ptr_t> vptr;
    pair<edge_itr_t, edge_itr_t> eptr;

    bool is_swap = false;
    vptr.first = &(*(cei->prev()->vert()));
    vptr.second = &(*(cei->vert()));
    if (vptr.first == vptr.second) return 1; // a edge has two same vertices.
    if (vptr.first > vptr.second) {
      swap(vptr.first, vptr.second); // swap the addr
      is_swap = true;
    }
    auto em_it = edges_map.find(vptr);
    if (em_it == edges_map.end()) {  // the edge is the first time added to map
      if (is_swap) eptr.second = cei;
      else eptr.first = cei;
      edges_map.insert(make_pair(vptr,eptr));
    } else {
      if (!em_it->second.second) {
        if (is_swap) em_it->second.second = cei;
        else return 2; // the mesh isn't manifold, or the vertices sort of the corresponding face isn't right.
      } else if (!em_it->second.first) {
        if (!is_swap) em_it->second.first = cei;
        else return 3; // the mesh isn't manifold, or the vertices sort of the corresponding face isn't right.
      } else { return 4; }// the edge has more than two adjacent faces.
    }
  }

  map<vert_ptr_t, vector<edge_itr_t> > bound_vert;

  // add opposite relation
  for (auto em_it = edges_map.begin(); em_it != edges_map.end(); ++em_it) {
    pair<edge_itr_t, edge_itr_t>& eptr = em_it->second;
    if (eptr.first && eptr.second) {
      m[eptr.first].oppo() = eptr.second;
      m[eptr.second].oppo() = eptr.first;
    } else {
      edge_itr_t be = (!eptr.second)? eptr.first:eptr.second;
      assert(be);
      edge_itr_t ne = m.add(edge_t());   // add the new edge
      m[ne].vert() = be->prev()->vert(); // vert that new edge point to
      m[ne].oppo() = be;                 // add oppo of new edge
      m[be].oppo() = ne;                 // add oppo of current edge
      vert_ptr_t vptr = &(*(ne->vert()));
      auto bv_it = bound_vert.find(vptr);
      if (bv_it == bound_vert.end()) {
        std::vector<edge_itr_t> ve_itr;
        ve_itr.push_back(ne);
        bound_vert.insert(make_pair(vptr, ve_itr));
      } else {
        bv_it->second.push_back(ne);
      }
    }
  }

  // add next and prev relation of boundary edges.
  for (auto bv_it = bound_vert.begin(); bv_it != bound_vert.end(); ++bv_it) {
    vector<edge_itr_t> bound_edges;
    vector<edge_itr_t>& v_edges = bv_it->second; // in edges
    for (auto ve_it = v_edges.begin(); ve_it != v_edges.end(); ++ve_it) {
      edge_itr_t out_e = (*ve_it)->oppo();
      while (out_e->face()) { out_e = out_e->prev()->oppo(); }
      bound_edges.push_back(*ve_it); // in edge
      bound_edges.push_back(out_e);  // out edge
    }

    for (size_t i = 1; i+1 < bound_edges.size(); i+=2) {
      m[bound_edges[i]].prev() = bound_edges[i+1];
      m[bound_edges[i+1]].next() = bound_edges[i];
    }
    assert(bound_edges.size() > 1);
    m[bound_edges.front()].next() = bound_edges.back();
    m[bound_edges.back()].prev() = bound_edges.front();
  }

  // adjust the edge that verts point to
  for (auto vi = m().verts().begin(); vi != m.verts().end(); ++vi) {
    adjust_vert_edge(m, vi);
  }
  
  return 0;
}

//! @brief copy a half-edge mesh to another empty mesh
template <typename TMPL_A, typename TMPL_B>
int copy(const mesh_tmpl<TMPL_A>& ma, mesh_tmpl<TMPL_B>& mb)
{
  usr_session::default_usr_session ses;
  return copy(ma, mb, ses);
}
template <typename TMPL_A, typename TMPL_B, typename SES>
int copy(const mesh_tmpl<TMPL_A>& ma, mesh_tmpl<TMPL_B>& mb, SES &ses)
{
  usr_operation<SES> op(ses);
  assert(mb().edges().size() == 0 && mb().faces().size() == 0
         && mb().verts().size() == 0);
  typedef typename TMPL_A::edge_t a_edge_t;
  typedef typename TMPL_B::edge_t b_edge_t;
  typedef typename TMPL_A::face_t a_face_t;
  typedef typename TMPL_B::face_t b_face_t;
  typedef typename TMPL_A::vert_t a_vert_t;
  typedef typename TMPL_B::vert_t b_vert_t;
  typedef typename TMPL_B::edges_t::const_iterator b_edge_itr_t;
  typedef typename TMPL_B::faces_t::const_iterator b_face_itr_t;
  typedef typename TMPL_B::verts_t::const_iterator b_vert_itr_t;
  
  std::map<const a_vert_t*, b_vert_itr_t> vert_map;
  std::map<const a_edge_t*, b_edge_itr_t> edge_map;
  std::map<const a_face_t*, b_face_itr_t> face_map;

  for (auto a_vi = ma().verts().begin(); a_vi != ma().verts().end(); ++a_vi) {
    b_vert_itr_t bv_itr = op.add_vert(mb, *a_vi);
    vert_map.insert(std::make_pair(&(*a_vi), bv_itr));
  }

  for (auto a_ei = ma().edges().begin(); a_ei != ma().edges().end(); ++a_ei) {
    b_edge_itr_t be_itr = op.add_edge(mb, *a_ei);
    edge_map.insert(std::make_pair(&(*a_ei), be_itr));
  }

  for (auto a_fi = ma().faces().begin(); a_fi != ma().faces().end(); ++a_fi) {
    b_face_itr_t bf_itr = op.add_face(mb, *a_fi);
    face_map.insert(std::make_pair(&(*a_fi), bf_itr));
  }

  for (auto a_vi = ma().verts().begin(); a_vi != ma().verts().end(); ++a_vi) {
    auto b_vi = vert_map[&(*a_vi)];
    mb()[b_vi].edge() = edge_map[&(*(a_vi->edge()))];
  }

  for (auto a_ei = ma().edges().begin(); a_ei != ma().edges().end(); ++a_ei) {
    auto b_ei = edge_map[&(*a_ei)];
    if (a_ei->face()) mb()[b_ei].face() = face_map[&(*(a_ei->face()))];
    mb()[b_ei].vert() = vert_map[&(*(a_ei->vert()))];
    mb()[b_ei].next() = edge_map[&(*(a_ei->next()))];
    mb()[b_ei].prev() = edge_map[&(*(a_ei->prev()))];
    mb()[b_ei].oppo() = edge_map[&(*(a_ei->oppo()))];
    assert(b_ei->vert());
    assert(b_ei->next());
    assert(b_ei->prev());
    assert(b_ei->oppo());
  }

  for (auto a_fi = ma().faces().begin(); a_fi != ma().faces().end(); ++a_fi) {
    auto b_fi = face_map[&(*a_fi)];
    mb()[b_fi].edge() = edge_map[&(*(a_fi->edge()))];
  }
  
  return 0;
}

//! @brief get the number of edges of face f
template<typename TMPL>
size_t valence(const typename TMPL::faces_t::const_iterator &f)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = f->edge();
  size_t r = 0;
  assert(e);
  do {
    ++r; e = e->next();
  } while (e != f->edge());
  return r;
}

//! @brief get the number of in edges of vert v
template<typename TMPL>
size_t valence(const typename TMPL::verts_t::const_iterator &v)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = v->edge();
  size_t r = 0;
  assert(e);
  do {
    ++r; e = e->next()->oppo();
  } while (e != v->edge());
  return r;
}


template <typename TMPL>
typename TMPL::verts_t::const_iterator get_iterator(const typename TMPL::vert_t &v)
{
  return v.edge()->vert();
}

template <typename TMPL>
typename TMPL::edges_t::const_iterator get_iterator(const typename TMPL::edge_t &e)
{
  return e.oppo()->oppo();
}

template <typename TMPL>
typename TMPL::faces_t::const_iterator get_iterator(const typename TMPL::face_t &f)
{
  return f.edge()->face();
}

//! @brief get the adjacent edges of face f
template <typename TMPL, typename CON>
void face_adj_edges(const typename TMPL::faces_t::const_iterator &f, CON& fe)
{
  // assume the f is a valid iterator
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = f->edge();
  assert(e);
  do {
    fe.push_back(e);
    e = e->next();
  } while (e != fe.front());
}

//! @brief get the adjacent verts of face f
template <typename TMPL, typename CON>
void face_adj_verts(const typename TMPL::faces_t::const_iterator &f, CON& fv)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = f->edge();
  do {
    fv.push_back(e->vert());
    e = e->next();
  } while (e != f->edge());
}

//! @brief get the adjacent faces of face f
template <typename TMPL, typename CON>
void face_adj_faces(const typename TMPL::faces_t::const_iterator &f, CON& ff)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  edge_itr_t e = f->edge();
  do {
    face_itr_t f = e->oppo()->face();
    if (f) ff.push_back(f);
    e = e->next();
  } while (e != f->edge());
}

//! @brief get the adjacent faces of edge e
//! there are two faces adjacent an edge except the boundary edge
template <typename TMPL, typename CON>
void edge_adj_faces(const typename TMPL::edges_t::const_iterator &e, CON& ef)
{
  if (e->face()) ef.push_back(e->face());
  if (e->oppo()->face()) ef.push_back(e->oppo()->face());
}

//! @brief get the adjacent faces of vert v
template <typename TMPL, typename CON>
void vert_adj_faces(const typename TMPL::verts_t::const_iterator &v, CON& vf)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  edge_itr_t e = v->edge();
  do {
    face_itr_t f = e->face();
    if (f) vf.push_back(f);
    e = e->next()->oppo();
  } while (e != v->edge());
}

//! @brief get the adjacent verts of vert v
template <typename TMPL, typename CON>
void vert_adj_verts(const typename TMPL::verts_t::const_iterator &v, CON& vv)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;
  edge_itr_t e = v->edge();
  do {
    vert_itr_t v = e->oppo()->vert();
    assert(v);
    vv.push_back(v);
    e = e->next()->oppo();
  } while (e != v->edge());
}

//! @brief get the adjacent out edges of vert v
template <typename TMPL, typename CON>
void vert_adj_out_edges(const typename TMPL::verts_t::const_iterator &v, CON& ve)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  edge_itr_t e = v->edge()->oppo();
  do {
    ve.push_back(e);
    e = e->prev()->oppo();
  } while (e != v->edge()->oppo());
}

//! @brief get the half edge of two vertices.
template <typename TMPL>
typename TMPL::edges_t::const_iterator
get_edge(const typename TMPL::verts_t::const_iterator & v1,
         const typename TMPL::verts_t::const_iterator & v2)
{
  typedef typename TMPL::edges_t::const_iterator CEI;
  CEI e = v2->edge();
  do
  {
    if (!e) break;
    if (e->oppo()->vert() == v1) return e;
    e = e->next()->oppo();
  } while (e != v2->edge());

  //if v2 is boundary vertex, additional step need to be take
  if (!v2->edge() || v2->edge()->face())
    return CEI();
  
  CEI ei = v2->edge();
  do
  {
    if (!ei) break;
    assert(ei);
    assert(ei->oppo());
    if (ei->vert() == v2 && ei->oppo()->vert() == v1)
      return ei;
    if (ei->oppo()->vert() == v2 && ei->vert() == v1)
      return ei->oppo();
    ei = ei->next();
  }
  while (ei != v2->edge());
  return CEI();
}


template <typename TMPL>
void
adjust_vert_edge(mesh_tmpl<TMPL> &m, const typename TMPL::verts_t::const_iterator & vert)
{
  //return for condition already holds
  if (!vert->edge() || !vert->edge()->face())
  {
    return;
  }
  
  typedef typename TMPL::edges_t::const_iterator CEI;
  CEI ei = vert->edge();
  do
  {
    if (!ei->face())
    {
      m()[vert].edge() = ei;
      break;
    }
    ei = ei->next()->oppo();
  }
  while(ei != vert->edge());
}

//! @brief adjust topology arround non-manifold vertex
template <typename TMPL>
int
adjust_nm_vert(mesh_tmpl<TMPL> & m, const std::vector<typename TMPL::edges_t::const_iterator> & bd_in_edges)
{
  typedef typename TMPL::edges_t::const_iterator CEI;
  typedef typename TMPL::faces_t::const_iterator CFI;
  size_t n = bd_in_edges.size();
  for (size_t i=0; i<n; ++i)
  {
    //if ei->next() and ei are in the same sector, it will cause
    //1-ring edges incomplete, need to be adjusted
    CEI tmp = bd_in_edges[i]->oppo();
    do
    {
      tmp = tmp->prev()->oppo();
    }
    while (tmp->face() != CFI());
    
    if (tmp == bd_in_edges[i]->next())
    {
      //exchange next of bd_in_edges[i] and bd_in_edges[i+1]
      m()[bd_in_edges[i]->next()].prev() = bd_in_edges[(i+1)%n];
      m()[bd_in_edges[(i+1)%n]->next()].prev() = bd_in_edges[i];
      CEI i_next = bd_in_edges[i]->next();
      m()[bd_in_edges[i]].next() = bd_in_edges[(i+1)%n]->next();
      m()[bd_in_edges[(i+1)%n]].next() = i_next;
    }
  }
  return 0;
}


// @brief all edges in sec are border edge, and the region between
// sec[i*2] and sec[i*2+1] are connected by faces.
template <typename CVI, typename Con>
void sectors(const CVI &vi, Con &sec)
{
  auto ei = vi->edge();
  do
  {
    if(!ei->face())
    {
      //add in and out edges of a sector sequencely
      sec.push_back(ei);
      sec.push_back(ei->next());
    }
    ei = ei->next()->oppo();
  }
  while (ei != vi->edge());
}


//! @brief add a new sector from in to out into sec., in and out are
//! outer edge of a sector, but may not be border edges.
template <typename TMPL, typename CEI, typename Con>
int add_face_into_sectors(mesh_tmpl<TMPL> &m, Con &sec, const CEI &in, const CEI &out)
{
  assert(in->vert() == out->oppo()->vert());
  assert(sec.size());
  assert(sec.size()%2 == 0);

  bool is_added = false;
  for (size_t i=0; i<sec.size(); ++i)
  {
    //find existing edges
    if (in == sec[i])//only happened in even numbers
    {
      if (out != sec[i+1])
      {
        m()[out->oppo()].next() = sec[i+1];
        m()[sec[i+1]].prev() = out->oppo();
      }
      is_added = true;
    }

    if (out == sec[i])//only happened in odds numbers
    {
      if (in != sec[i-1])
      {
        m()[in->oppo()].prev() = sec[i-1];
        m()[sec[i-1]].next() = in->oppo();
      }
      is_added = true;
    }
  }

  //add the new face to the first sector
  if (!is_added)
  {
    m()[sec[0]].next() = in->oppo();
    m()[in->oppo()].prev() = sec[0];
    m()[sec[1]].prev() = out->oppo();
    m()[out->oppo()].next() = sec[1];
  }

  adjust_vert_edge(m, in->vert());
}

template <typename TMPL, typename RND_ITR>
typename TMPL::faces_t::const_iterator
add_face_keep_topo(mesh_tmpl<TMPL> &m, const RND_ITR &vert_loop_beg, std::size_t n)
{
  usr_session::default_usr_session ses;
  return add_face_keep_topo(m, vert_loop_beg, n, ses);
}
template <typename TMPL, typename RND_ITR, typename SES>
typename TMPL::faces_t::const_iterator
add_face_keep_topo(mesh_tmpl<TMPL> &m, const RND_ITR &vert_loop_beg, std::size_t n, SES& ses)
{
  usr_operation<SES> op(ses);
  //add vertices
  std::vector<bool> is_v_exist(n, true);//is the vertex already exists
  for (size_t i=0; i<n; ++i)
  {
    is_v_exist[i] =!!(vert_loop_beg[i]->edge());
  }
  
  assert(n > 2); // TODO: may be also good for n=1,2.

  typedef typename TMPL::faces_t::const_iterator CFI;
  typedef typename TMPL::edges_t::const_iterator CEI;
  
  CFI fi = op.add_face(m, typename TMPL::face_t());
  typedef std::vector<std::pair<CEI, bool> > e_con;
  e_con edges(n); // edges[i]: v_i -> v_{i+1}

  // add edges, set vert and oppo
  for(size_t i = 0; i < n; ++i) {
    // find edge v_i to v_{i+1}
    bool is_new = false;
    CEI ei = get_edge<TMPL>(vert_loop_beg[i], vert_loop_beg[(i+1)%n]);
    if(!ei) { // add the pair of half_edge
      is_new = true;
      ei = op.add_edge(m, typename TMPL::edge_t());
      m()[ei].vert() = vert_loop_beg[(i+1)%n];
      CEI eio = op.add_edge(m, typename TMPL::edge_t());
      m()[eio].vert() = vert_loop_beg[i];
      m()[ei].oppo() = eio;
      m()[eio].oppo() = ei;
    }
    
    edges[i] = std::make_pair(ei, is_new);

    if (ei->face() != CFI())
    {
      return CFI();
    }
    
    //assert(!ei->face()); // assert for non-manifold edge
    //m()[ei].face() = fi;//set edge->face()
  }
  //modify prev and next for each vertex of the new face
  for (size_t i=0; i<n; ++i)
  {
    //vertex already exist means that their are other edges need to be
    //consider around this vertex
    if (vert_loop_beg[i]->edge())//judge whether this vertex is new
    {
      //not new vertex, first judge whether two edge both exist
      //in one ring neighbor of this vertex, if do, just change topo
      //this used to fix *topology chaos* when add face to a random
      //sector if non of the two edge exists.
      if (!edges[(i+n-1)%n].second && !edges[i].second)
      {
        CEI e1 = edges[(i+n-1)%n].first;
        CEI e2 = edges[i].first;
        if (e1->next() != e2)
        {
          //save boundary edges prepare for modification
          std::vector<CEI> bd_in_edges;
          bd_in_edges.reserve(3);
          CEI tmp = e1->vert()->edge();
          do
          {
            if (tmp->face() == CFI() && tmp != e1)
              bd_in_edges.push_back(tmp);
            tmp = tmp->oppo()->prev();
          }
          while (tmp != e1->vert()->edge());
          assert(e2->prev() != e1);
          //reset the vertex edge to a boundary edge
          if (e1->vert()->edge() == e1)
            m()[e1->vert()].edge() = e2->prev();
          assert(e1->vert()->edge()->vert() == e1);
          //adjoint e1 and e2
          m()[e1->next()].prev() = e2->prev();
          m()[e2->prev()].next() = e1->next();
          m()[e1].next() = e2;
          m()[e2].prev() = e1;
          //every time fill a sector with a face, topology arround this
          //non-manifold vertex should be modified
          //adjust_vert_edge(m, vert_loop_beg[i]);
          if (bd_in_edges.size() > 1)// ==1 donot need adjust 
            adjust_nm_vert(m, bd_in_edges);
        }
      }
      else
      {
        std::vector<CEI> bd;
        bd.reserve(6);
        //note: even after the new edges are added, sector function 
        //can not find them because their prev and next are abscent
        sectors(vert_loop_beg[i], bd);
        add_face_into_sectors(m, bd, edges[(i+n-1)%n].first, edges[i].first);
      }
    }
    else
    {
      //new vertex with two new edges, simple topology relation
      m()[edges[(i+n-1)%n].first->oppo()].prev() = edges[i].first->oppo();
      m()[edges[i].first->oppo()].next() = edges[(i+n-1)%n].first->oppo();
    }
  }
  // set vert->edge
  for(size_t i = 0; i < n; ++i) {
    m()[edges[i].first].next() = edges[(i+1)%n].first;
    m()[edges[i].first].prev() = edges[(i+n-1)%n].first;
    m()[edges[i].first].face() = fi;
    if(!vert_loop_beg[i]->edge())
    {
      //opposite edge is more likely a boundary edge,
      //for effciency, not use edges[i-1]
      m()[vert_loop_beg[i]].edge() = edges[i].first->oppo();
    }
    assert(vert_loop_beg[i]->edge()->vert() == vert_loop_beg[i]);
  }
  //adjust edge of every vertex to guarantee boundary vertex have bd edge
  for (size_t j=0; j<n; ++j)
  {
    adjust_vert_edge(m, edges[j].first->vert());
  }
  m()[fi].edge() = edges[0].first;
  return fi;
}


//! @brief is an given halfedge can be flipped
//! @param is_valid(i) = true
//! @return 0 for done, 1 for incorrect edge denoting, 2 for non-triangle
//! 3 for special cases
template <typename TMPL>
int try_edge_flip(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;
  //get faces adjacent to edge
  edge_itr_t e_op = e->oppo();
  
  //whether there is two face adjacent to the given edge
  if (!e->face() || !e_op->face())
  {
    return 1;
  }
  
  //whether faces adjacent to the edge are triangles
  if (valence<TMPL>(e->face()) != 3
      ||valence<TMPL>(e_op->face()) != 3)
  {
    return 2;
  }

  //special cases can not flip
  //there is an edge in the position which we want to add new edge
  auto sv = e->next()->vert();
  auto ev = e->oppo()->next()->vert();
  if (!!get_edge<TMPL>(sv, ev))
  {
    return 3;
  }
  
  return 0;
}


//! @brief implement a flip operation on a given edge by rotate the
//! existing faces by 90 degree counter-clockwise.
template <typename TMPL>
void edge_flip_by_rotate(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;

  //record all information before flip
  face_itr_t f1 = e->face();
  face_itr_t f2 = e->oppo()->face();
  
  edge_itr_t e_n = e->next();
  edge_itr_t e_p = e->prev();
  
  edge_itr_t e_op = e->oppo();
  edge_itr_t e_op_n = e_op->next();
  edge_itr_t e_op_p = e_op->prev();
  
  vert_itr_t v_s = e->oppo()->vert();
  vert_itr_t v_e = e->vert();

  vert_itr_t v_target_s = e_op->next()->vert();
  vert_itr_t v_target_e = e->next()->vert();
  ///////////////////////edge//////////////////////////////
  //edge topology in face 1
  m()[e].next() = e_p;
  m()[e_p].prev() = e;

  m()[e].prev() = e_op_n;
  m()[e_op_n].next() = e;

  m()[e_p].next() = e_op_n;
  m()[e_op_n].prev() = e_p;


  //edge topology in face 2
  m()[e_op].next() = e_op_p;
  m()[e_op_p].prev() = e_op;

  m()[e_op].prev() = e_n;
  m()[e_n].next() = e_op;
  
  m()[e_n].prev() = e_op_p;
  m()[e_op_p].next() = e_n;
  ///////////////////////////face//////////////////////////
  //change faces
  m()[e_op_n].face() = f1;
  m()[e_n].face() = f2;

  if (f1->edge() == e_n) m()[f1].edge() = e;
  if (f2->edge() == e_op_n) m()[f2].edge() = e_op;
  
  /////////////////////////vertex//////////////////////////
  if (v_s->edge() == e_op) m()[v_s].edge() = e_p;
  if (v_e->edge() == e) m()[v_e].edge() = e_op_p;
  m()[e].vert() = v_target_e;
  m()[e_op].vert() = v_target_s;
}

//! @param try_edge_flip(m, i) == 0;
template <typename TMPL>
void edge_flip_by_del_add(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e)
{
  usr_session::default_usr_session ses;
  edge_flip_by_del_add(m, e, ses);
}
template <typename TMPL, typename SES>
void edge_flip_by_del_add(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e, SES & ses)
{
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;

  std::vector<vert_itr_t> new_face, new_face2;
  new_face.push_back(e->vert());
  new_face.push_back(e->next()->vert());
  new_face.push_back(e->oppo()->next()->vert());
  new_face2.push_back(e->oppo()->vert());
  new_face2.push_back(e->oppo()->next()->vert());
  new_face2.push_back(e->next()->vert());
  del(m, e);
  add_face_keep_topo(m, new_face.begin(), 3);
  add_face_keep_topo(m, new_face2.begin(), 3);
}

//! @brief test whether an edge can be collapse
template <typename TMPL>
int try_collapse(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &ei)
{
  assert(ei->oppo());
  //! TODO add a condition for cannot collapse that there are more
  //! than one edge between two vertex
  typedef typename TMPL::edges_t::const_iterator CEI;
  typedef typename TMPL::verts_t::const_iterator CVI;
  
  CVI s_vert = ei->oppo()->vert();
  CVI e_vert = ei->vert();

  CEI sitr = s_vert->edge();
  CEI eitr = e_vert->edge();
  //CASE1:there are more than two edge couple share same end vertex  
  do
  {
    do
    {

      if (sitr->oppo()->vert() == eitr->oppo()->vert())
      {
        if (sitr != ei->prev() && sitr != ei->oppo()->next()->oppo()
            && eitr != ei->next()->oppo() && eitr != ei->oppo()->prev())
          return 1;
      }
      eitr = eitr->next()->oppo();
    }
    while(eitr != e_vert->edge());
    sitr = sitr->next()->oppo();
  }
  while(sitr != s_vert->edge());

  //CASE2 when collapse fill a triangle hole,
  //but left some face fliping out of the filled faces
  //                 ___
  //                 \ /  <---face flipping outside
  //                  V    
  //                  ^  
  //                 / \
  //       faces    /   \  faces
  //               / hole\
  //              /_______\
  //                 ei
  CEI e_tmp = ei;
  if (!e_tmp->face())
  {
    if (e_tmp->next()->vert() == e_tmp->prev()->oppo()->vert()
        && e_tmp->next()->next() != e_tmp->prev())
      return 2;
  }
  e_tmp = ei->oppo();
  if (!e_tmp->face())
  {
    if (e_tmp->next()->vert() == e_tmp->prev()->oppo()->vert()
        && e_tmp->next()->next() != e_tmp->prev())
      return 2;
  }

  //CASE3 collapse edge in a triangle which shares same edges with another triangle
  CEI e_op = ei->oppo();
  if (ei->face() && e_op->face()
      && ei->next()->oppo() == e_op->next()->next()
      && ei->prev()->oppo() == e_op->prev()->prev())
  {
    return 3;
  }
  
  return 0;
}

//! @brief implement a collapse operation on a given halfedge
template <typename TMPL>
const typename TMPL::verts_t::const_iterator&
collapse_edge(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e)
{
  usr_session::default_usr_session ses;
  return collapse_edge(m, e, ses);
}
template <typename TMPL, typename SES>
const typename TMPL::verts_t::const_iterator&
collapse_edge(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e, SES & ses)
{
  usr_operation<SES> op(ses);
  assert(e);
  const typename TMPL::verts_t::const_iterator& rtn = e->vert();
  
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  
  //save e->oppo() and start vertex before it maybe invisable later
  auto e_op = e->oppo();
  auto e_vert = e->vert(), s_vert = e_op->vert();
  
  //special cases
  //#1: the last edges
  if (e->next() == e->prev())
  {
    m()[e->vert()].edge() = edge_itr_t();//set to null
    //delete the collapsed edge
    op.del(m, e);
    op.del(m, e_op);
    //delete the start vertex
    op.del(m, s_vert);
    return rtn;
  }
  //#2: the last two face share same three edges
  
  
  //NOTE:1. e->vert()->edge() should not be used in the remaining
  //part of this function for it had been changed
  //2. can not move this part to the end because if topology information
  //changes, we may fail to find vert->edge();
  //3. a basic thought, not change topology info unless it's necessary
  
  //modify e->vert->edge() for two condition
  //#1 e->oppo()->prev() will be deleted if valence of that face is 3
  if (e->oppo()->face()
      && valence<TMPL>(e->oppo()->face()) == 3
      && e->vert()->edge() == e->oppo()->prev())
  {
    m()[e->vert()].edge() = e->oppo()->next()->oppo();
  }

  //#2 
  if (e->vert()->edge() == e)
  {
    if (e->face())
    {
      m()[e->vert()].edge() = e->next()->oppo();
    }
    else
    {
      if (e->oppo()->face())
      {
        m()[e->vert()].edge() = e->oppo()->next()->oppo();
      }
      else
      {
        //in this else scope, both edge have no faces
        if (e->next() != e->oppo())
        {
          m()[e->vert()].edge() = e->next()->oppo();
        }
        else
        {
          if (e->prev() != e->oppo())
          {
            m()[e->vert()].edge() = e->prev();
          }
          else
          {
            m()[e->vert()].edge() = edge_itr_t();
          }
        }
      }
    }
  }

  
  //normal procedure
  //repoint all the edges to the end vertex of e
  std::vector<edge_itr_t> edges_adj_svert;
  vert_adj_out_edges<TMPL>(s_vert, edges_adj_svert);
  
  auto eitr = edges_adj_svert.begin();
  for (;eitr!=edges_adj_svert.end(); ++eitr)
  {
    if (*eitr == e) continue;

    //delete loop
    if ((*eitr)->vert() == e_vert())
    {
      if ((*eitr)->vert()->edge() == (*eitr)->oppo())
      {
        m()[(*eitr)->vert()].edge() = (*eitr)->next();
      }
      if ((*eitr)->oppo()->vert()->edge() == (*eitr))
      {
        m()[(*eitr)->oppo()->vert()].edge() = (*eitr)->oppo()->next();
      }
      m()[(*eitr)->next()].prev() = (*eitr)->prev();
      m()[(*eitr)->prev()].next() = (*eitr)->next();
      m()[(*eitr)->oppo()->next()].prev() = (*eitr)->oppo()->prev();
      m()[(*eitr)->oppo()->prev()].next() = (*eitr)->oppo()->next();
      op.del(m, (*eitr)->oppo());
      op.del(m, *eitr);
    }
    //repoint
    m()[(*eitr)->oppo()].vert() = e_vert;
  }

  
  //modify edges
  if (e->face())
  {
    if (valence<TMPL>(e->face()) == 3)
    {
      
      m()[e->prev()->oppo()].oppo() = e->next()->oppo();
      m()[e->next()->oppo()].oppo() = e->prev()->oppo();

      if (e->next()->vert()->edge() == e->next())
      {
        m()[e->next()->vert()].edge() = e->prev()->oppo();
      }
      
      op.del(m, e->next());
      op.del(m, e->prev());
      op.del(m, e->face());
    }
    else
    {
      //faces with more than 3 edges
      if (e->face()->edge() == e)
      {
        m()[e->face()].edge() = e->next();
      }
      m()[e->prev()].next() = e->next();
      m()[e->next()].prev() = e->prev();
    }
    //if edges with no face need to be delete, add it here
  }
  else
  {
    //NOTE: e->prev() or e->next() may equal to e_op
    if (e->next() == e_op)
      m()[e->prev()].next() = e_op->next();
    else
      m()[e->prev()].next() = e->next();

    if (e->prev() == e_op)
      m()[e->next()].prev() = e_op->prev();
    else
      m()[e->next()].prev() = e->prev();
  }

  //if this assertion fail, edges which cannot collapse may be collapsed.
  assert(e_op == e->oppo());

  if (e_op->face())
  {
    if (valence<TMPL>(e_op->face()) == 3)
    {
      m()[e_op->next()->oppo()].oppo() = e_op->prev()->oppo();
      m()[e_op->prev()->oppo()].oppo() = e_op->next()->oppo();

      //modify edge of vertex before delete edge
      if (e_op->next()->vert()->edge() == e_op->next())
      {
        m()[e_op->next()->vert()].edge() = e_op->prev()->oppo();
      }
      op.del(m, e_op->next());
      op.del(m, e_op->prev());
      op.del(m, e_op->face());
    }
    else
    {
      if (e_op->face()->edge() == e_op)
      {
        m()[e_op->face()].edge() = e_op->next();
      }
      
      m()[e_op->prev()].next() = e_op->next();
      m()[e_op->next()].prev() = e_op->prev();
    }
  }
  else
  {
    //NOTE: e_op->prev() or e_op->next() may equal to e
    if (e_op->next() == e)
      m()[e_op->prev()].next() = e->next();
    else
      m()[e_op->prev()].next() = e_op->next();

    if (e_op->prev() == e)
      m()[e_op->next()].prev() = e->prev();
    else
      m()[e_op->next()].prev() = e_op->prev();
  }

  //delete the collapsed edge
  op.del(m, e);
  op.del(m, e_op);
  //delete the start vertex

  op.del(m, s_vert);

  adjust_vert_edge(m, rtn);
  return rtn;
}

template <typename TMPL>
int is_collapse_ok(mesh_tmpl<TMPL> &m, const typename TMPL::verts_t::const_iterator &v)
{
  usr_session::default_usr_session ses;
  return is_collapse_ok(m, v, ses);
}
template <typename TMPL, typename SES>
int is_collapse_ok(mesh_tmpl<TMPL> &m, const typename TMPL::verts_t::const_iterator &v, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  
  std::vector<edge_itr_t> ve;
  vert_adj_out_edges<TMPL>(v, ve);

  //merge two edges which link same vertices
  //compare from the last edge
  auto last_e = ve.end();
  --last_e;
  auto itr = ve.begin();

  while (itr != ve.end())
  {
    if (!(*itr)->face() && !(*itr)->oppo()->face())
    {
      collapse_edge(m, *itr);
      ++itr;
      continue;
    }

    //NOTE this part is prepare for non-manifold cases
    if ((*itr)->vert() == (*last_e)->vert())
    {
      //merge two adjcent edges together
      if ((*itr)->face() || (*last_e)->oppo()->face())
      {
        //case1
        if (v->edge() == (*last_e))
        {
          m()[v].edge() = *itr;
        }
        if ((*last_e)->vert()->edge() == (*itr)->oppo())
        {
          m()[(*last_e)->vert()].edge() = (*last_e)->oppo();
        }

        m()[(*last_e)->oppo()].oppo() = *itr;
        m()[(*itr)].oppo() = (*last_e)->oppo();
        op.del(m, *last_e);
        op.del(m, (*itr)->oppo());
      }
      else
      {
        //case2
        if (v->edge() == *itr)
        {
          m()[v].edge() = *last_e;
        }
        if ((*itr)->vert()->edge() == (*last_e)->oppo())
        {
          m()[(*itr)->vert()].edge() = (*itr)->oppo();
        }
        
        m()[(*last_e)].oppo() = (*itr)->oppo();
        m()[(*itr)->oppo()].oppo() = *last_e;
        op.del(m, (*last_e)->oppo());
        op.del(m, *itr);
      }
    }
  
    last_e = itr;
    ++itr;
  }
  return 0;
}

//! @brief split 1 edge into 2 edges, 2 faces into 4 faces and fix 
//! the adj of relational verts, edges, faces.
//! the adjacent faces of the edge should be triangles.
template <typename TMPL>
const typename TMPL::verts_t::const_iterator
split_edge(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e)
{
  usr_session::default_usr_session ses;
  return split_edge(m, e, ses);
}
template <typename TMPL, typename SES>
const typename TMPL::verts_t::const_iterator
split_edge(mesh_tmpl<TMPL> &m, const typename TMPL::edges_t::const_iterator &e, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::verts_t::const_iterator vert_itr_t;  
  if (!e) return vert_itr_t();

  const size_t is_triangle = 3;

  if ((e->face()) && valence<TMPL>(e->face())!=is_triangle)
    return vert_itr_t();
  if ((e->oppo()->face()) && valence<TMPL>(e->oppo()->face())!=is_triangle)
    return vert_itr_t();

  vert_itr_t new_vert = op.add_vert(m, typename TMPL::vert_t());
  m()[new_vert].edge() = e->oppo();

  typedef typename TMPL::edges_t::const_iterator edge_itr_t;
  typedef typename TMPL::faces_t::const_iterator face_itr_t;
  
  edge_itr_t mid_e1 = op.add_edge(m, typename TMPL::edge_t());
  edge_itr_t mid_e2 = op.add_edge(m, typename TMPL::edge_t());
  
  m()[mid_e1].vert() = new_vert; m()[mid_e1].oppo() = mid_e2;
  m()[mid_e1].next() = e; m()[mid_e1].prev() = e->prev();
  m()[mid_e1].face() = face_itr_t();

  m()[e->prev()].next() = mid_e1;
  
  m()[mid_e2].vert() = e->oppo()->vert();
  m()[mid_e2].oppo() = mid_e1; m()[mid_e2].next() = e->oppo()->next();  
  m()[mid_e2].prev() = e->oppo(); m()[mid_e2].face() = face_itr_t();

  m()[e->oppo()->next()].prev() = mid_e2;

  if (e->oppo()->vert()->edge() == e->oppo())
    m()[e->oppo()->vert()].edge() = mid_e2;

  m()[e].prev() = mid_e1;
  m()[e->oppo()].next() = mid_e2;
  m()[e->oppo()].vert() = new_vert;

  // split left face
  if (e->face()) {
    m()[e->face()].edge() = e;
    
    face_itr_t left_f = op.add_face(m, typename TMPL::face_t());
    m()[left_f].edge() = mid_e1;
    
    m()[mid_e1].face() = left_f; m()[mid_e1->prev()].face() = left_f;
    
    edge_itr_t left_e1 = op.add_edge(m, typename TMPL::edge_t());
    edge_itr_t left_e2 = op.add_edge(m, typename TMPL::edge_t());
    
    m()[left_e1].vert() = new_vert;
    m()[left_e1].next() = e; m()[left_e1].prev() = e->next();
    m()[left_e1].oppo() = left_e2; m()[left_e1].face() = e->face();
    
    m()[left_e2].vert() = e->next()->vert();
    m()[left_e2].next() = mid_e1->prev(); m()[left_e2].prev() = mid_e1;
    m()[left_e2].oppo() = left_e1; m()[left_e2].face() = left_f;
    
    m()[e].prev() = left_e1; m()[e->next()].next() = left_e1;
    m()[mid_e1].next() = left_e2; m()[mid_e1->prev()].prev() = left_e2;
  }

  // split right face
  if (e->oppo()->face()) {
    m()[e->oppo()->face()].edge() = e->oppo();
    
    face_itr_t right_f = op.add_face(m, typename TMPL::face_t());
    m()[right_f].edge() = mid_e2;

    m()[mid_e2].face() = right_f; m()[mid_e2->next()].face() = right_f;

    edge_itr_t right_e1 = op.add_edge(m, typename TMPL::edge_t());
    edge_itr_t right_e2 = op.add_edge(m, typename TMPL::edge_t());

    m()[right_e1].vert() = mid_e2->next()->vert();
    m()[right_e1].next() = e->oppo()->prev();
    m()[right_e1].prev() = e->oppo();
    m()[right_e1].oppo() = right_e2;
    m()[right_e1].face() = e->oppo()->face();
    m()[right_e2].vert() = new_vert;
    m()[right_e2].next() = mid_e2;
    m()[right_e2].prev() = mid_e2->next();
    m()[right_e2].oppo() = right_e1;
    m()[right_e2].face() = right_f;

    m()[e->oppo()->prev()].prev() = right_e1;
    m()[mid_e2->next()].next() = right_e2;
    m()[e->oppo()].next() = right_e1;
    m()[mid_e2].prev() = right_e2;
  }

  if (!e->face()) m()[new_vert].edge() = mid_e1;

  return new_vert;
}

//! @brief split a face by a given vertex
template <typename TMPL>
const typename TMPL::verts_t::const_iterator
split_face(mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator &f, const typename TMPL::verts_t::const_iterator &v)
{
  usr_session::default_usr_session ses;
  return split_face(m, f, v, ses);
}
template <typename TMPL, typename SES>
const typename TMPL::verts_t::const_iterator
split_face(mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator &f, const typename TMPL::verts_t::const_iterator &v, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::verts_t::const_iterator CVI;
  typedef typename TMPL::edges_t::const_iterator CEI;
  typedef typename TMPL::faces_t::const_iterator CFI;

  CEI ei = f->edge();
  CEI ei_end = ei;
  
  CEI first_ei;
  CEI last_ei;
  bool is_first = true;
  do //cycle for each edge of the face
  {
    CEI new_out_ei = op.add_edge(m, typename TMPL::edge_t());
    CEI new_in_ei = op.add_edge(m, typename TMPL::edge_t());
    CFI new_fi = op.add_face(m, typename TMPL::face_t());

    if (is_first)
    {
      first_ei = new_out_ei;
      is_first = false;
    }
    else
    {
      m()[new_out_ei].oppo() = last_ei;
      m()[last_ei].oppo() = new_out_ei;
    }

    //edge topology
    m()[new_out_ei].vert() = ei->oppo()->vert();
    m()[new_out_ei].next() = ei;
    m()[new_out_ei].prev() = new_in_ei;
    m()[new_out_ei].face() = new_fi;
        
    m()[new_in_ei].vert() = v;
    m()[new_in_ei].next() = new_out_ei;
    m()[new_in_ei].prev() = ei;
    m()[new_in_ei].face() = new_fi;

    CEI ei_next = ei->next();//save before modify
    m()[ei].next() = new_in_ei;
    m()[ei].prev() = new_out_ei;
    m()[ei].face() = new_fi;

    //edge of a face
    m()[new_fi].edge() = ei;
    last_ei = new_in_ei;

    ei = ei_next;
  }
  while(ei != ei_end);

  m()[first_ei].oppo() = last_ei;
  m()[last_ei].oppo() = first_ei;
  m()[v].edge() = last_ei;
  op.del(m, f);

  return v;
}

//! @brief subdivide a face
template <typename TMPL>
const typename TMPL::verts_t::const_iterator
split_face2(mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator &f)
{
  usr_session::default_usr_session ses;
  split_face2(m, f, ses);
}
template <typename TMPL, typename SES>
const typename TMPL::verts_t::const_iterator
split_face2(mesh_tmpl<TMPL> &m, const typename TMPL::faces_t::const_iterator &f, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::verts_t::const_iterator CVI;
  std::vector<CVI> fv_list;
  face_adj_verts<TMPL>(f, fv_list);
  del(m, f);
  assert(fv_list.size()>2);
  CVI center_v = op.add_vert(m, typename TMPL::vert_t());
  auto fvi = fv_list.begin();
  std::vector<CVI> face_v;
  face_v.reserve(3);
  while (1)
  {
    face_v.clear();
    face_v.push_back(center_v);
    face_v.push_back(*fvi);
    ++fvi;
    if (fvi == fv_list.end())
    {
      face_v.push_back(*(fv_list.begin()));
      add_face_keep_topo(m, face_v.begin(), 3);
      break;
    }
    else
    {
      face_v.push_back(*fvi);
      add_face_keep_topo(m, face_v.begin(), 3);
    }
  }
  return center_v;
}

//! @brief split edges
template <typename TMPL, typename RND_ITR, typename SES>
bool
split_edges(mesh_tmpl<TMPL> &m, const RND_ITR &edge_loop_beg, std::size_t n)
{
  usr_session::default_usr_session ses;
  return split_edges(m, edge_loop_beg, n, ses);
}
template <typename TMPL, typename RND_ITR, typename SES>
bool
split_edges(mesh_tmpl<TMPL> &m, const RND_ITR &edge_loop_beg, std::size_t n, SES & ses)
{
  usr_operation<SES> op(ses);
  typedef typename TMPL::edges_t::const_iterator CEI;
  typedef typename TMPL::verts_t::const_iterator CVI;

  std::vector<CEI> edges_to_split;
  for (size_t i=0; i<n; ++i)
  {
    //new vertex, vi->edge remains null for next step procedure
    CVI vi = op.add_vert(m, typename TMPL::vert_t());
    
    //create new edges
    CEI new_e_1 = op.add_edge(m, typename TMPL::edge_t());
    CEI new_e_2 = op.add_edge(m, typename TMPL::edge_t());
    CEI new_o_1 = op.add_edge(m, typename TMPL::edge_t());
    CEI new_o_2 = op.add_edge(m, typename TMPL::edge_t());

    CEI ei = edge_loop_beg[i];
    CEI oi = ei->oppo();
    //save new edges for split
    if (ei->face())
      edges_to_split.push_back(new_e_1);
    if (oi->face())
      edges_to_split.push_back(new_e_2);

    //start split face
    if (ei->face())
      del(m, ei->face());
    if (oi->face())
      del(m, oi->face());
    
    m()[new_e_1].prev() = ei->prev();
    m()[new_e_1].next() = new_e_2;
    m()[new_e_1].vert() = vi;
    
    m()[new_e_2].prev() = new_e_1;
    m()[new_e_2].next() = ei->next();
    m()[new_e_2].vert() = ei->vert();

    m()[ei->next()].prev() = new_e_2;
    m()[ei->prev()].next() = new_e_1;
    
    //additional infomation about split
    if (ei->split_info.root == -1)
    {
      m()[ei].split_info.root = ei->id_;
      m()[ei].split_info.level = 0;
    }
    m()[new_e_1].split_info.root = ei->split_info.root;
    m()[new_e_1].split_info.level = ei->split_info.level+1;
    m()[new_e_2].split_info.root = ei->split_info.root;
    m()[new_e_2].split_info.level = ei->split_info.level+1;

    //modify opposite edge topology
    m()[new_o_1].prev() = oi->prev();
    m()[new_o_1].next() = new_o_2;
    m()[new_o_1].vert() = vi;
    
    m()[new_o_2].prev() = new_o_1;
    m()[new_o_2].next() = oi->next();
    m()[new_o_2].vert() = oi->vert();

    m()[oi->next()].prev() = new_o_2;
    m()[oi->prev()].next() = new_o_1;

    //additional infomation about split
    if (oi->split_info.root == -1)
    {
      m()[oi].split_info.root = oi->id_;
      m()[oi].split_info.level = 0;
    }
    m()[new_o_1].split_info.root = oi->split_info.root;
    m()[new_o_1].split_info.level = oi->split_info.level+1;
    m()[new_o_2].split_info.root = oi->split_info.root;
    m()[new_o_2].split_info.level = oi->split_info.level+1;

    //set opposite relations
    m()[new_e_1].oppo() = new_o_2;
    m()[new_o_2].oppo() = new_e_1;
    m()[new_e_2].oppo() = new_o_1;
    m()[new_o_1].oppo() = new_e_2;

    //set vert->edge
    if (ei->vert()->edge() == ei)
      m()[ei->vert()].edge() = new_e_2;
    if (oi->vert()->edge() == oi)
      m()[oi->vert()].edge() = new_o_2;

    op.del(m, ei);
    op.del(m, oi);
  }

  //Note:1.face of new_e_X is null means this face hasn't been splited
  //2.use edge of vertex is null to find newly added vertex
  for (auto esi=edges_to_split.begin(); esi!=edges_to_split.end(); ++esi)
  {
    split_face_by_edge(m, *esi);
  }
}


//! @brief test whether a mesh has right topology
//! @param need elements specifier id_
//! @return 0 for ok,
//!         1 for vertex <--> edge error: 11 error at vert, 12 at edge
//!         2 for edge   <--> edge error: 21 oppo, 22 next, 23 prev, 
//!                           24 extra edge
//!         3 for edge   <--> face error: 32 error at edge, 33 at face
template <class TriMesh>
int topology_test(const TriMesh& m, int & error_id)
{
  typedef typename TriMesh::verts_t::const_iterator CVI;
  typedef typename TriMesh::edges_t::const_iterator CEI;
  typedef typename TriMesh::faces_t::const_iterator CFI;

  for (CVI vi=m.verts().begin(); vi!=m.verts().end(); ++vi)
  {
    //test vert->edge()
    if (!vi->edge()) continue;
    if (vi->edge()->vert() != vi)
    {
      std::cout << "err-edge" << std::endl;
      error_id = vi->id_;
      return 12;
    }
  }

  for (CEI ei=m.edges().begin(); ei!=m.edges().end(); ++ei)
  {
    if (!ei->oppo())
    {
      std::cout << "non-oppo " << std::endl;
      error_id = ei->id_;
      return 21;
    }
    if (!ei->next())
    {
      std::cout << "non-next " << std::endl;
      error_id = ei->id_;
      return 22;
    }
    if (!ei->prev())
    {
      std::cout << "non-prev " << std::endl;
      error_id = ei->id_;
      return 23;
    }
    if (ei->oppo()->oppo() != ei)
    {
      std::cout << "err-oppo " << std::endl;
      error_id = ei->id_;
      return 21;
    }
    if (ei->next()->prev() != ei)
    {
      std::cout << "err-next " << std::endl;
      error_id = ei->id_;
      return 22;
    }
    if (ei->prev()->next() != ei)
    {
      std::cout << "err-prev " << std::endl;
      error_id = ei->id_;
      return 23;
    }
    
    if (!ei->vert())
    {
      std::cout << "non-vert " << std::endl;
      error_id = ei->id_;
      return 12;
    }
    //boundary
    CEI e_tmp = ei;
    CFI pre_fi = e_tmp->face();
    size_t max = m.edges().size();
    do
    {
      //test loop consitistancy
      if (e_tmp->face() != pre_fi)
      {
        std::cout << "err-loop" << std::endl;
        return 22;
      }

      //test loop integrity
      --max;
      if (max == 0)
      {
        std::cout << "endless-loop " << std::endl;
        error_id = ei->id_;
        return 22;
      }
      
      e_tmp = e_tmp->next();
    }
    while(e_tmp != ei);
  }
  //
  //find repeat edge between vertices couple
  for (CEI ei=m.edges().begin(); ei!=m.edges().end(); ++ei)
  {
    size_t v1, v2;
    v1 = ei->oppo()->vert()->id_;
    v2 = ei->vert()->id_;
    for (CEI e=m.edges().begin(); e!=m.edges().end(); ++e)
    {
      if (v1 == e->oppo()->vert()->id_
          && v2 == e->vert()->id_
          && e != ei)
      {
        std::cout << "ext-edge" << std::endl;
        return 24;
      }
    }
  }
  //*/
  for (CFI fi=m.faces().begin(); fi!=m.faces().end(); ++fi)
  {
    if (!fi->edge())
    {
      std::cout << "non-edge" << std::endl;
      error_id = fi->id_;
      return 33;
    }
    CEI ei = fi->edge();
    size_t max = m.edges().size();
    do
    {
      if (ei->face() != fi)
      {
        std::cout << "err-edge2face " << std::endl;
        error_id = fi->id_;
        return 32;
      }
      --max;
      if (max == 0)
      {
        std::cout << "endless-1ring " << std::endl;
        error_id = fi->id_;
        return 33;
      }
      ei = ei->next();
    }
    while (ei != fi->edge());
  }

  return 0;
}
