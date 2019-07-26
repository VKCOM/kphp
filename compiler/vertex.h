#pragma once

#include "auto/compiler/vertex/vertex-all.h"

#include "compiler/data/data_ptr.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

size_t vertex_total_mem_used __attribute__ ((weak));

ProfilerRaw& get_vertex_inner_prof();

ProfilerRaw& get_vertex_inner_data_prof();

inline bool operator==(const VertexPtr &a, const VertexPtr &b) {
  return a->id == b->id;
}

template<Operation Op>
int get_index(const VertexAdaptor<Op> &v) {
  return v->id;
}

template<Operation Op>
void set_index(VertexAdaptor<Op> &v, int id) {
  v->id = id;
}

inline void set_location(VertexPtr v, const Location &location) {
  v->location = location;
}

template<Operation op>
inline void set_location(const Location &location, VertexAdaptor<op> &v) {
  v->location = location;
}

template<Operation op, Operation ...ops>
inline void set_location(const Location &location, VertexAdaptor<op> &v, VertexAdaptor<ops>& ...vs) {
  set_location(location, v);
  set_location(location, vs...);
}

inline const Location &get_location(const VertexPtr &v) {
  return v->get_location();
}

template<Operation Op>
size_t vertex_inner_size(int args_n) {
  return sizeof(vertex_inner<Op>) + sizeof(VertexPtr) * args_n;
}

template<Operation Op>
size_t vertex_inner_shift(int args_n) {
  return sizeof(VertexPtr) * args_n;
}

template<Operation Op>
vertex_inner<Op> *raw_create_vertex_inner(int args_n) {
  size_t size = vertex_inner_size<Op>(args_n);
  size_t shift = vertex_inner_shift<Op>(args_n);
  get_vertex_inner_prof().alloc_memory(size);
  get_vertex_inner_data_prof().alloc_memory(size - shift);

  auto ptr = reinterpret_cast<vertex_inner<Op> *>((char *)malloc(size) + shift);
  new(ptr) vertex_inner<Op>();
  ptr->raw_init(args_n);
  ptr->type_ = Op;
  return ptr;
}

template<Operation Op>
vertex_inner<Op> *raw_clone_vertex_inner(const vertex_inner<Op> &from) {
  size_t size = vertex_inner_size<Op>((int)from.size());
  size_t shift = vertex_inner_shift<Op>((int)from.size());
  get_vertex_inner_prof().alloc_memory(size);
  get_vertex_inner_data_prof().alloc_memory(size - shift);

  auto ptr = reinterpret_cast<vertex_inner<Op> *>((char *)malloc(size) + shift);
  new(ptr) vertex_inner<Op>(from);
  ptr->raw_copy(from);
  return ptr;
}

template <typename... Args>
VertexPtr create_vertex(Operation op, Args&& ...args) {
  switch (op) {
#define FOREACH_OP(x) case x: return VertexAdaptor<x>::create_vararg(std::forward<Args>(args)...);

#include "auto/compiler/vertex/foreach-op.h"

    default:
    kphp_fail();
  }
  return {};
}

template<Operation Op>
inline VertexRange get_function_params(VertexAdaptor<Op> func) {
  return func->params()->params();
}

// op_int_const string representation can be "123", "0x123", "0002", "-123", "0b0010101"
// but it is guaranteed to be a valid int
inline long parse_int_from_string(VertexAdaptor<op_int_const> v) {
  auto start = v->get_string().c_str();

  if (start[0] == '0' && start[1] == 'b') {
    return std::strtol(start + 2, nullptr, 2);
  }

  return std::strtol(start, nullptr, 0);
}
