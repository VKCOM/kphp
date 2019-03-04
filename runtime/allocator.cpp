#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <signal.h>
#include <utility>
#include <climits>
#include <unistd.h>

#include "common/wrappers/likely.h"

#include "runtime/allocator.h"
#include "runtime/php_assert.h"

//#define DEBUG_MEMORY

void check_stack_overflow() __attribute__ ((weak));

void check_stack_overflow() {
  //pass;
}

namespace dl {

const size_type MAX_BLOCK_SIZE = 16384;
const size_type MAX_ALLOC = 0xFFFFFF00;


volatile int in_critical_section = 0;
volatile long long pending_signals = 0;


void enter_critical_section() {
  check_stack_overflow();
  php_assert (in_critical_section >= 0);
  in_critical_section++;
}

void leave_critical_section() {
  in_critical_section--;
  php_assert (in_critical_section >= 0);
  if (pending_signals && in_critical_section <= 0) {
    for (int i = 0; i < (int)sizeof(pending_signals) * 8; i++) {
      if ((pending_signals >> i) & 1) {
        raise(i);
      }
    }
  }
}

//custom allocator for multimap
template<class T>
class script_allocator {
public:
  typedef dl::size_type size_type;
  typedef ptrdiff_t difference_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef T value_type;

  template<class U>
  struct rebind {
    typedef script_allocator<U> other;
  };

  script_allocator() noexcept = default;

  script_allocator(const script_allocator &) noexcept = default;

  template<class U>
  explicit script_allocator(const script_allocator<U> &) noexcept {}

  ~script_allocator() noexcept = default;

  pointer address(reference x) const {
    return &x;
  }

  const_pointer address(const_reference x) const {
    return &x;
  }

  pointer allocate(size_type n, void const * = nullptr) {
    php_assert (n == 1 && sizeof(T) < MAX_BLOCK_SIZE);
    auto result = static_cast <pointer> (dl::allocate(sizeof(T)));
    if (unlikely(!result)) php_critical_error ("not enough memory to continue");
    return result;
  }

  void deallocate(pointer p, size_type n) {
    php_assert (n == 1 && sizeof(T) < MAX_BLOCK_SIZE);
    dl::deallocate(static_cast <void *> (p), sizeof(T));
  }

  size_type max_size() const noexcept {
    return INT_MAX / sizeof(T);
  }

  void construct(pointer p, const T &val) {
    new(p) T(val);
  }

  void destroy(pointer p) {
    p->~T();
  }
};


long long query_num = 0;
bool script_runned = false;
volatile bool use_script_allocator = false;

bool allocator_inited = false;

size_t memory_begin = 0;
size_t memory_end = 0;
size_type memory_limit = 0;
size_type memory_used = 0;
size_type max_memory_used = 0;
size_type max_real_memory_used = 0;

size_type static_memory_used = 0;

using map_type = std::multimap<size_type, void *, std::less<size_type>, script_allocator<std::pair<const size_type, void *>>>;
char left_pieces_storage[sizeof(map_type)] = {0};
map_type *left_pieces = nullptr;
void *piece = nullptr;
void *piece_end = nullptr;
void *free_blocks[MAX_BLOCK_SIZE >> 3] = {0};

const bool stupid_allocator = false;


size_type memory_get_total_usage() {
  return (size_type)((size_t)piece - memory_begin);
}

void check_script_memory_piece(void *p, size_t s) {
  const size_t p_begin = reinterpret_cast<size_t>(p);
  const size_t p_end = p_begin + s;
  if (memory_begin <= p_begin && p_end <= memory_end) {
    return;
  }

  php_critical_error(
    "Found unexpected script memory piece:\n"
    "ptr:          %p\n"
    "size:         %zu\n"
    "piece:        %p\n"
    "piece_end:    %p\n"
    "memory_begin: %p\n"
    "memory_end:   %p\n"
    "memory_limit:         %u\n"
    "memory_used:          %u\n"
    "max_memory_used:      %u\n"
    "max_real_memory_used: %u\n"
    "static_memory_used:   %u\n"
    "query_num:            %lld\n"
    "script_runned:        %d\n"
    "use_script_allocator: %d\n"
    "in_critical_section:  %d\n"
    "pending_signals:      %lld\n",
    p, s, piece, piece_end, reinterpret_cast<void *>(memory_begin), reinterpret_cast<void *>(memory_end),
    memory_limit, memory_used, max_memory_used, max_real_memory_used, static_memory_used,
    query_num, script_runned, use_script_allocator, in_critical_section, pending_signals);
}

inline void allocator_add(void *block, size_type size) {
  check_script_memory_piece(block, size);

  if ((char *)block + size == (char *)piece) {
    piece = block;
    return;
  }

  if (size < MAX_BLOCK_SIZE) {
    size >>= 3;
    *(void **)block = free_blocks[size];
    free_blocks[size] = block;
  } else {
    php_assert(left_pieces);
    left_pieces->insert(std::make_pair(size, block));
  }
}

void global_init_script_allocator() {
  enter_critical_section();
  memset(free_blocks, 0, sizeof(free_blocks));
  query_num++;
  leave_critical_section();
}

void init_script_allocator(void *buf, size_type n) {
  in_critical_section = 0;
  pending_signals = 0;

  enter_critical_section();

  allocator_inited = true;
  use_script_allocator = false;
  memory_begin = (size_t)buf;
  memory_end = (size_t)buf + n;
  memory_limit = n;

  memory_used = 0;
  max_memory_used = 0;
  max_real_memory_used = 0;

  piece = buf;
  piece_end = (void *)((char *)piece + n);

  new(left_pieces_storage) map_type();
  left_pieces = reinterpret_cast <map_type *> (left_pieces_storage);
  memset(free_blocks, 0, sizeof(free_blocks));

  query_num++;
  script_runned = true;

  leave_critical_section();
}

void free_script_allocator() {
  enter_critical_section();

  dl::script_runned = false;
  left_pieces = nullptr;
  php_assert (!dl::use_script_allocator);

  leave_critical_section();
}

static inline void *allocate_stack(size_type n) {
  if ((char *)piece_end - (char *)piece < n) {
    php_warning("Can't allocate %d bytes", (int)n);
    raise(SIGUSR2);
    return nullptr;
  }

  void *result = piece;
  piece = (void *)((char *)piece + n);
  return result;
}

void *allocate(size_type n) {
  if (!allocator_inited) {
    return static_allocate(n);
  }

  if (!script_runned) {
    php_critical_error("Trying to call allocate for non runned script, n = %u", n);
    return nullptr;
  }
  enter_critical_section();

  php_assert (n);

  void *result;
  if (stupid_allocator) {
    result = allocate_stack(n);
  } else {
    n = (n + 7) & -8;

    if (n < MAX_BLOCK_SIZE) {
      size_type nn = n >> 3;
      if (free_blocks[nn] == nullptr) {
        result = allocate_stack(n);
#ifdef DEBUG_MEMORY
        fprintf (stderr, "allocate %d, chunk not found, allocating from stack at %p\n", n, result);
#endif
      } else {
        result = free_blocks[nn];
#ifdef DEBUG_MEMORY
        fprintf (stderr, "allocate %d, chunk found at %p\n", n, result);
#endif
        free_blocks[nn] = *(void **)result;
      }
    } else {
      php_assert(left_pieces);
      auto p = left_pieces->lower_bound(n);
#ifdef DEBUG_MEMORY
      fprintf (stderr, "allocate %d from %d, map size = %d\n", n, p == left_pieces->end() ? -1 : (int)p->first, (int)left_pieces->size());
#endif
      if (p == left_pieces->end()) {
        result = allocate_stack(n);
      } else {
        size_type left = p->first - n;
        result = p->second;
        left_pieces->erase(p);
        if (left) {
          allocator_add((char *)result + n, left);
        }
      }
    }
  }

  if (result != nullptr) {
    memory_used += n;
    if (memory_used > max_memory_used) {
      max_memory_used = memory_used;
    }
    auto real_memory_used = (size_type)((size_t)piece - memory_begin);
    if (real_memory_used > max_real_memory_used) {
      max_real_memory_used = real_memory_used;
    }
  }

  leave_critical_section();
  return result;
}

void *allocate0(size_type n) {
  if (!allocator_inited) {
    return static_allocate0(n);
  }

  if (!script_runned) {
    php_critical_error("Trying to call allocate0 for non runned script, n = %u", n);
    return nullptr;
  }

  return memset(allocate(n), 0, n);
}

void *reallocate(void *p, size_type new_n, size_type old_n) {
  if (!allocator_inited) {
    static_reallocate(&p, new_n, &old_n);
    return p;
  }

  if (!script_runned) {
    php_critical_error("Trying to call reallocate for non runned script, p = %p, new_n = %u, old_n = %u", p, new_n, old_n);
    return p;
  }
  enter_critical_section();

  php_assert (new_n > old_n);

  //real reallocate
  size_type real_old_n = (old_n + 7) & -8;
  if ((char *)p + real_old_n == (char *)piece) {
    size_type real_new_n = (new_n + 7) & -8;
    size_type add = real_new_n - real_old_n;
    if ((char *)piece_end - (char *)piece >= add) {
#ifdef DEBUG_MEMORY
      fprintf (stderr, "reallocate %d at %p to %d\n", old_n, p, new_n);
#endif
      piece = (void *)((char *)piece + add);
      memory_used += add;
      if (memory_used > max_memory_used) {
        max_memory_used = memory_used;
      }
      auto real_memory_used = (size_type)((size_t)piece - memory_begin);
      if (real_memory_used > max_real_memory_used) {
        max_real_memory_used = real_memory_used;
      }

      leave_critical_section();
      return p;
    }
  }

  void *result = allocate(new_n);
  if (result != nullptr) {
    memcpy(result, p, old_n);
    deallocate(p, old_n);
  }

  leave_critical_section();
  return result;
}

void deallocate(void *p, size_type n) {
//  fprintf (stderr, "deallocate %d: allocator_inited = %d, script_runned = %d\n", n, allocator_inited, script_runned);
  if (!allocator_inited) {
    return static_deallocate(&p, &n);
  }

  if (stupid_allocator) {
    return;
  }

  if (!script_runned) {
    return;
  }
  enter_critical_section();

  php_assert (n);

  n = (n + 7) & -8;
#ifdef DEBUG_MEMORY
  fprintf (stderr, "deallocate %d at %p\n", n, p);
#endif
  memory_used -= n;
  allocator_add(p, n);

  leave_critical_section();
}


void *static_allocate(size_type n) {
  php_assert (!query_num || !use_script_allocator);
  enter_critical_section();

  php_assert (n);

  void *result = malloc(n);
#ifdef DEBUG_MEMORY
  fprintf (stderr, "static allocate %d at %p\n", n, result);
#endif
  if (result == nullptr) {
    php_warning("Can't static_allocate %d bytes", (int)n);
    raise(SIGUSR2);
    leave_critical_section();
    return nullptr;
  }

  static_memory_used += n;
  leave_critical_section();
  return result;
}

void *static_allocate0(size_type n) {
  php_assert (!query_num);
  enter_critical_section();

  php_assert (n);

  void *result = calloc(1, n);
#ifdef DEBUG_MEMORY
  fprintf (stderr, "static allocate0 %d at %p\n", n, result);
#endif
  if (result == nullptr) {
    php_warning("Can't static_allocate0 %d bytes", (int)n);
    raise(SIGUSR2);
    leave_critical_section();
    return nullptr;
  }

  static_memory_used += n;
  leave_critical_section();
  return result;
}

void static_reallocate(void **p, size_type new_n, size_type *n) {
#ifdef DEBUG_MEMORY
  fprintf (stderr, "static reallocate %d at %p\n", *n, *p);
#endif

  enter_critical_section();
  static_memory_used -= *n;

  void *old_p = *p;
  *p = realloc(*p, new_n);
  if (*p == nullptr) {
    php_warning("Can't static_reallocate from %d to %d bytes", (int)*n, (int)new_n);
    raise(SIGUSR2);
    *p = old_p;
    leave_critical_section();
    return;
  }
  *n = new_n;

  static_memory_used += *n;
  leave_critical_section();
}

void static_deallocate(void **p, size_type *n) {
#ifdef DEBUG_MEMORY
  fprintf (stderr, "static deallocate %d at %p\n", *n, *p);
#endif

  enter_critical_section();
  static_memory_used -= *n;

  free(*p);
  *p = nullptr;
  *n = 0;

  leave_critical_section();
}

// guaranteed alignment of dl::allocate
const size_t MAX_ALIGNMENT = 8;

void *malloc_replace(size_t x) {
  php_assert (x <= MAX_ALLOC - MAX_ALIGNMENT);
  php_assert (sizeof(size_t) <= MAX_ALIGNMENT);
  size_t real_allocate = x + MAX_ALIGNMENT;
  void *p;
  if (use_script_allocator) {
    p = allocate(real_allocate);
  } else {
    p = static_allocate(real_allocate);
  }
  if (p == nullptr) {
    php_critical_error ("not enough memory to continue");
  }
  *(size_t *)p = real_allocate;
  return (void *)((char *)p + MAX_ALIGNMENT);
}

void free_replace(void *p) {
  if (p == nullptr) {
    return;
  }

  p = (void *)((char *)p - MAX_ALIGNMENT);
  if (use_script_allocator) {
    deallocate(p, *(size_t *)p);
  } else {
    size_type n = *(size_t *)p;
    static_deallocate(&p, &n);
  }
}

}

//replace global operators new and delete for linked C++ code
void *operator new(std::size_t n) {
  return dl::malloc_replace(n);
}

void *operator new(std::size_t n, const std::nothrow_t &) noexcept {
  return dl::malloc_replace(n);
}

void *operator new[](std::size_t n) {
  return dl::malloc_replace(n);
}

void *operator new[](std::size_t n, const std::nothrow_t &) noexcept {
  return dl::malloc_replace(n);
}

void operator delete(void *p) throw() {
  return dl::free_replace(p);
}

void operator delete(void *p, const std::nothrow_t &) noexcept {
  return dl::free_replace(p);
}

void operator delete[](void *p) throw() {
  return dl::free_replace(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept {
  return dl::free_replace(p);
}
