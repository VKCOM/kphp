// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>
#include <stdbool.h>


#define __DEFINE_TREE(prefix, name, value_t, value_compare, y_t, y_compare, new_tree_node, free_tree_node)                                                     \
  __DECLARE_TREE(prefix, name, value_t, y_t)                                                                                                                   \
                                                                                                                                                               \
  tree_##name##_t *tree_lookup_##name(tree_##name##_t *T, value_t x) {                                                                                         \
    long long c;                                                                                                                                               \
    while (T && (c = value_compare(x, T->x))) {                                                                                                                \
      T = (c < 0) ? T->left : T->right;                                                                                                                        \
    }                                                                                                                                                          \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_lookup_p_##name(tree_##name##_t *T, value_t *x) {                                                                                      \
    long long c;                                                                                                                                               \
    while (T && (c = value_compare((*x), T->x))) {                                                                                                             \
      T = (c < 0) ? T->left : T->right;                                                                                                                        \
    }                                                                                                                                                          \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  value_t *tree_lookup_value_##name(tree_##name##_t *T, value_t x) {                                                                                           \
    long long c;                                                                                                                                               \
    while (T && (c = value_compare(x, T->x))) {                                                                                                                \
      T = (c < 0) ? T->left : T->right;                                                                                                                        \
    }                                                                                                                                                          \
    return T ? &T->x : 0;                                                                                                                                      \
  }                                                                                                                                                            \
                                                                                                                                                               \
  value_t *tree_lookup_value_p_##name(tree_##name##_t *T, value_t *x) {                                                                                        \
    long long c;                                                                                                                                               \
    while (T && (c = value_compare((*x), T->x))) {                                                                                                             \
      T = (c < 0) ? T->left : T->right;                                                                                                                        \
    }                                                                                                                                                          \
    return T ? &T->x : 0;                                                                                                                                      \
  }                                                                                                                                                            \
                                                                                                                                                               \
  void tree_split_##name(tree_##name##_t **L, tree_##name##_t **R, tree_##name##_t *T, value_t x) {                                                            \
    if (!T) {                                                                                                                                                  \
      *L = *R = 0;                                                                                                                                             \
      return;                                                                                                                                                  \
    }                                                                                                                                                          \
    long long c = value_compare(x, T->x);                                                                                                                      \
    if (c < 0) {                                                                                                                                               \
      *R = T;                                                                                                                                                  \
      tree_split_##name(L, &T->left, T->left, x);                                                                                                              \
      tree_relax_##name(*R);                                                                                                                                   \
    } else {                                                                                                                                                   \
      *L = T;                                                                                                                                                  \
      tree_split_##name(&T->right, R, T->right, x);                                                                                                            \
      tree_relax_##name(*L);                                                                                                                                   \
    }                                                                                                                                                          \
  }                                                                                                                                                            \
                                                                                                                                                               \
  void tree_split_p_##name(tree_##name##_t **L, tree_##name##_t **R, tree_##name##_t *T, value_t *x) {                                                         \
    if (!T) {                                                                                                                                                  \
      *L = *R = 0;                                                                                                                                             \
      return;                                                                                                                                                  \
    }                                                                                                                                                          \
    long long c = value_compare((*x), T->x);                                                                                                                   \
    if (c < 0) {                                                                                                                                               \
      *R = T;                                                                                                                                                  \
      tree_split_p_##name(L, &T->left, T->left, x);                                                                                                            \
      tree_relax_##name(*R);                                                                                                                                   \
    } else {                                                                                                                                                   \
      *L = T;                                                                                                                                                  \
      tree_split_p_##name(&T->right, R, T->right, x);                                                                                                          \
      tree_relax_##name(*L);                                                                                                                                   \
    }                                                                                                                                                          \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_insert_##name(tree_##name##_t *T, value_t x, y_t y) {                                                                                  \
    tree_##name##_t *P;                                                                                                                                        \
    if (!T) {                                                                                                                                                  \
      P = new_tree_node(x, y);                                                                                                                                 \
      tree_relax_##name(P);                                                                                                                                    \
      return P;                                                                                                                                                \
    }                                                                                                                                                          \
    long long c = y_compare(y, T->y);                                                                                                                          \
    if (c < 0) {                                                                                                                                               \
      c = value_compare(x, T->x);                                                                                                                              \
      assert(c);                                                                                                                                               \
      if (c < 0) {                                                                                                                                             \
        T->left = tree_insert_##name(T->left, x, y);                                                                                                           \
      } else {                                                                                                                                                 \
        T->right = tree_insert_##name(T->right, x, y);                                                                                                         \
      }                                                                                                                                                        \
      tree_relax_##name(T);                                                                                                                                    \
      return T;                                                                                                                                                \
    }                                                                                                                                                          \
    P = new_tree_node(x, y);                                                                                                                                   \
    tree_split_##name(&P->left, &P->right, T, x);                                                                                                              \
    tree_relax_##name(P);                                                                                                                                      \
    return P;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_insert_p_##name(tree_##name##_t *T, value_t *x, y_t y) {                                                                               \
    tree_##name##_t *P;                                                                                                                                        \
    if (!T) {                                                                                                                                                  \
      P = new_tree_node(*x, y);                                                                                                                                \
      tree_relax_##name(P);                                                                                                                                    \
      return P;                                                                                                                                                \
    }                                                                                                                                                          \
    long long c = y_compare(y, T->y);                                                                                                                          \
    if (c < 0) {                                                                                                                                               \
      c = value_compare((*x), T->x);                                                                                                                           \
      assert(c);                                                                                                                                               \
      if (c < 0) {                                                                                                                                             \
        T->left = tree_insert_p_##name(T->left, x, y);                                                                                                         \
      } else {                                                                                                                                                 \
        T->right = tree_insert_p_##name(T->right, x, y);                                                                                                       \
      }                                                                                                                                                        \
      tree_relax_##name(T);                                                                                                                                    \
      return T;                                                                                                                                                \
    }                                                                                                                                                          \
    P = new_tree_node(*x, y);                                                                                                                                  \
    tree_split_p_##name(&P->left, &P->right, T, x);                                                                                                            \
    tree_relax_##name(P);                                                                                                                                      \
    return P;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_merge_##name(tree_##name##_t *L, tree_##name##_t *R) {                                                                                 \
    if (!L) {                                                                                                                                                  \
      return R;                                                                                                                                                \
    }                                                                                                                                                          \
    if (!R) {                                                                                                                                                  \
      return L;                                                                                                                                                \
    }                                                                                                                                                          \
    if (y_compare(L->y, R->y) > 0) {                                                                                                                           \
      L->right = tree_merge_##name(L->right, R);                                                                                                               \
      tree_relax_##name(L);                                                                                                                                    \
      return L;                                                                                                                                                \
    } else {                                                                                                                                                   \
      R->left = tree_merge_##name(L, R->left);                                                                                                                 \
      tree_relax_##name(R);                                                                                                                                    \
      return R;                                                                                                                                                \
    }                                                                                                                                                          \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_delete_##name(tree_##name##_t *T, value_t x) {                                                                                         \
    assert(T);                                                                                                                                                 \
    long long c = value_compare(x, T->x);                                                                                                                      \
    if (!c) {                                                                                                                                                  \
      tree_##name##_t *N = tree_merge_##name(T->left, T->right);                                                                                               \
      free_tree_node(T);                                                                                                                                       \
      return N;                                                                                                                                                \
    } else if (c < 0) {                                                                                                                                        \
      T->left = tree_delete_##name(T->left, x);                                                                                                                \
    } else {                                                                                                                                                   \
      T->right = tree_delete_##name(T->right, x);                                                                                                              \
    }                                                                                                                                                          \
    tree_relax_##name(T);                                                                                                                                      \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_delete_p_##name(tree_##name##_t *T, value_t *x) {                                                                                      \
    assert(T);                                                                                                                                                 \
    long long c = value_compare((*x), T->x);                                                                                                                   \
    if (!c) {                                                                                                                                                  \
      tree_##name##_t *N = tree_merge_##name(T->left, T->right);                                                                                               \
      free_tree_node(T);                                                                                                                                       \
      return N;                                                                                                                                                \
    } else if (c < 0) {                                                                                                                                        \
      T->left = tree_delete_p_##name(T->left, x);                                                                                                              \
    } else {                                                                                                                                                   \
      T->right = tree_delete_p_##name(T->right, x);                                                                                                            \
    }                                                                                                                                                          \
    tree_relax_##name(T);                                                                                                                                      \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_get_min_##name(tree_##name##_t *T) {                                                                                                   \
    while (T && T->left) {                                                                                                                                     \
      T = T->left;                                                                                                                                             \
    }                                                                                                                                                          \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  tree_##name##_t *tree_get_max_##name(tree_##name##_t *T) {                                                                                                   \
    while (T && T->right) {                                                                                                                                    \
      T = T->right;                                                                                                                                            \
    }                                                                                                                                                          \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
  void tree_act_##name(tree_##name##_t *T, tree_##name##_act_t A) {                                                                                            \
    if (!T) {                                                                                                                                                  \
      return;                                                                                                                                                  \
    }                                                                                                                                                          \
    tree_act_##name(T->left, A);                                                                                                                               \
    A(T->x);                                                                                                                                                   \
    tree_act_##name(T->right, A);                                                                                                                              \
  }                                                                                                                                                            \
  void tree_act_ex_##name(tree_##name##_t *T, tree_##name##_act_ex_t A, void *extra) {                                                                         \
    if (!T) {                                                                                                                                                  \
      return;                                                                                                                                                  \
    }                                                                                                                                                          \
    tree_act_ex_##name(T->left, A, extra);                                                                                                                     \
    A(T->x, extra);                                                                                                                                            \
    tree_act_ex_##name(T->right, A, extra);                                                                                                                    \
  }                                                                                                                                                            \
  void tree_act_ex2_##name(tree_##name##_t *T, tree_##name##_act_ex2_t A, void *extra, void *extra2) {                                                         \
    if (!T) {                                                                                                                                                  \
      return;                                                                                                                                                  \
    }                                                                                                                                                          \
    tree_act_ex2_##name(T->left, A, extra, extra2);                                                                                                            \
    A(T->x, extra, extra2);                                                                                                                                    \
    tree_act_ex2_##name(T->right, A, extra, extra2);                                                                                                           \
  }                                                                                                                                                            \
  bool tree_act_stop_##name(tree_##name##_t *T, tree_##name##_act_stop_t A, void *extra) {                                                                     \
    if (!T) {                                                                                                                                                  \
      return true;                                                                                                                                             \
    }                                                                                                                                                          \
    if (!tree_act_stop_##name(T->left, A, extra)) {                                                                                                            \
      return false;                                                                                                                                            \
    }                                                                                                                                                          \
    if (!A(T->x, extra)) {                                                                                                                                     \
      return false;                                                                                                                                            \
    }                                                                                                                                                          \
    if (!tree_act_stop_##name(T->right, A, extra)) {                                                                                                           \
      return false;                                                                                                                                            \
    }                                                                                                                                                          \
    return true;                                                                                                                                               \
  }                                                                                                                                                            \
  tree_##name##_t *tree_clear_##name(tree_##name##_t *T) {                                                                                                     \
    if (!T) {                                                                                                                                                  \
      return 0;                                                                                                                                                \
    }                                                                                                                                                          \
    tree_clear_##name(T->left);                                                                                                                                \
    tree_clear_##name(T->right);                                                                                                                               \
    tree_free_##name(T);                                                                                                                                       \
    return 0;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  int tree_count_##name(tree_##name##_t *T) {                                                                                                                  \
    if (!T) {                                                                                                                                                  \
      return 0;                                                                                                                                                \
    }                                                                                                                                                          \
    return 1 + tree_count_##name(T->left) + tree_count_##name(T->right);                                                                                       \
  }

#define __DEFINE_TREE_STDNOZ_ALLOC_PREFIX(prefix, name, value_t, value_compare, y_t, y_compare)                                                                \
  __DECLARE_TREE_TYPE(name, value_t, y_t)                                                                                                                      \
  prefix tree_##name##_t *tree_alloc_##name(value_t x, y_t);                                                                                                   \
  prefix void tree_free_##name(tree_##name##_t *T);                                                                                                            \
  __DEFINE_TREE(prefix, name, value_t, value_compare, y_t, y_compare, tree_alloc_##name, tree_free_##name)                                                     \
  tree_##name##_t *tree_alloc_##name(value_t x, y_t y) {                                                                                                       \
    tree_##name##_t *T = reinterpret_cast<tree_##name##_t *>(malloc(sizeof(tree_##name##_t)));                                                                                                      \
    assert(T);                                                                                                                                                 \
    T->x = x;                                                                                                                                                  \
    T->y = y;                                                                                                                                                  \
    T->left = T->right = 0;                                                                                                                                    \
    return T;                                                                                                                                                  \
  }                                                                                                                                                            \
  void tree_free_##name(tree_##name##_t *T) {                                                                                                                  \
    free(T);                                                                                                                                                   \
  }                                                                                                                                                            \
  void tree_relax_##name(tree_##name##_t *T __attribute__((unused))) {}                                                                                        \
  void tree_check_##name(tree_##name##_t *T) {                                                                                                                 \
    if (!T) {                                                                                                                                                  \
      return;                                                                                                                                                  \
    }                                                                                                                                                          \
    if (T->left) {                                                                                                                                             \
      assert(value_compare(T->left->x, T->x) < 0);                                                                                                             \
      tree_check_##name(T->left);                                                                                                                              \
    }                                                                                                                                                          \
    if (T->right) {                                                                                                                                            \
      assert(value_compare(T->right->x, T->x) > 0);                                                                                                            \
      tree_check_##name(T->right);                                                                                                                             \
    }                                                                                                                                                          \
  }

#define DEFINE_TREE_STDNOZ_ALLOC(name, value_t, value_compare, y_t, y_compare)                                                                                 \
  __DEFINE_TREE_STDNOZ_ALLOC_PREFIX(static, name, value_t, value_compare, y_t, y_compare)

#define __DECLARE_TREE_TYPE(name, value_t, y_t)                                                                                                                \
  struct tree_##name {                                                                                                                                         \
    struct tree_##name *left, *right;                                                                                                                          \
    value_t x;                                                                                                                                                 \
    y_t y;                                                                                                                                                     \
  };                                                                                                                                                           \
  typedef struct tree_##name tree_##name##_t;                                                                                                                  \
  typedef void (*tree_##name##_act_t)(value_t x);                                                                                                              \
  typedef void (*tree_##name##_act_ex_t)(value_t x, void *extra);                                                                                              \
  typedef void (*tree_##name##_act_ex2_t)(value_t x, void *extra, void *extra2);                                                                               \
  typedef bool (*tree_##name##_act_stop_t)(value_t x, void *extra);

#define __DECLARE_TREE_TYPE_CNT(name, value_t, y_t)                                                                                                            \
  struct tree_##name {                                                                                                                                         \
    struct tree_##name *left, *right;                                                                                                                          \
    value_t x;                                                                                                                                                 \
    y_t y;                                                                                                                                                     \
    int count;                                                                                                                                                 \
  };                                                                                                                                                           \
  typedef struct tree_##name tree_##name##_t;                                                                                                                  \
  typedef void (*tree_##name##_act_t)(value_t x);                                                                                                              \
  typedef void (*tree_##name##_act_ex_t)(value_t x, void *extra);                                                                                              \
  typedef void (*tree_##name##_act_ex2_t)(value_t x, void *extra, void *extra2);                                                                               \
  typedef bool (*tree_##name##_act_stop_t)(value_t x, void *extra);

#define __DECLARE_TREE(prefix, name, value_t, y_t)                                                                                                             \
  prefix tree_##name##_t *tree_lookup_##name(tree_##name##_t *T, value_t x) __attribute__((unused));                                                           \
  prefix tree_##name##_t *tree_lookup_p_##name(tree_##name##_t *T, value_t *x) __attribute__((unused));                                                        \
  prefix value_t *tree_lookup_value_##name(tree_##name##_t *T, value_t x) __attribute__((unused));                                                             \
  prefix value_t *tree_lookup_value_p_##name(tree_##name##_t *T, value_t *x) __attribute__((unused));                                                          \
  prefix void tree_split_##name(tree_##name##_t **L, tree_##name##_t **R, tree_##name##_t *T, value_t x) __attribute__((unused));                              \
  prefix void tree_split_p_##name(tree_##name##_t **L, tree_##name##_t **R, tree_##name##_t *T, value_t *x) __attribute__((unused));                           \
  prefix tree_##name##_t *tree_insert_##name(tree_##name##_t *T, value_t x, y_t y) __attribute__((warn_unused_result, unused));                                \
  prefix tree_##name##_t *tree_insert_p_##name(tree_##name##_t *T, value_t *x, y_t y) __attribute__((warn_unused_result, unused));                             \
  prefix tree_##name##_t *tree_merge_##name(tree_##name##_t *L, tree_##name##_t *R) __attribute__((unused));                                                   \
  prefix tree_##name##_t *tree_delete_##name(tree_##name##_t *T, value_t x) __attribute__((warn_unused_result, unused));                                       \
  prefix tree_##name##_t *tree_delete_p_##name(tree_##name##_t *T, value_t *x) __attribute__((warn_unused_result, unused));                                    \
  prefix tree_##name##_t *tree_get_min_##name(tree_##name##_t *T) __attribute__((unused));                                                                     \
  prefix tree_##name##_t *tree_get_max_##name(tree_##name##_t *T) __attribute__((unused));                                                                     \
  prefix void tree_act_##name(tree_##name##_t *T, tree_##name##_act_t A) __attribute__((unused));                                                              \
  prefix void tree_act_ex_##name(tree_##name##_t *T, tree_##name##_act_ex_t A, void *extra) __attribute__((unused));                                           \
  prefix void tree_act_ex2_##name(tree_##name##_t *T, tree_##name##_act_ex2_t A, void *extra, void *extra2) __attribute__((unused));                           \
  prefix bool tree_act_stop_##name(tree_##name##_t *T, tree_##name##_act_stop_t A, void *extra) __attribute__((unused));                                       \
  prefix tree_##name##_t *tree_clear_##name(tree_##name##_t *T) __attribute__((unused));                                                                       \
  prefix void tree_check_##name(tree_##name##_t *T) __attribute__((unused));                                                                                   \
  prefix int tree_count_##name(tree_##name##_t *T) __attribute__((unused));                                                                                    \
  prefix void tree_relax_##name(tree_##name##_t *T) __attribute__((unused));

struct tree_any_ptr {
  struct tree_any_ptr *left, *right;
  void *x;
  int y;
};

static inline void tree_act_any(struct tree_any_ptr *T, void (*f)(void *)) {
  if (!T) {
    return;
  }
  tree_act_any(T->left, f);
  f(T->x);
  tree_act_any(T->right, f);
}

static inline void tree_act_any_ex(struct tree_any_ptr *T, void (*f)(void *, void *), void *extra) {
  if (!T) {
    return;
  }
  tree_act_any_ex(T->left, f, extra);
  f(T->x, extra);
  tree_act_any_ex(T->right, f, extra);
}

static inline void tree_act_any_ex2(struct tree_any_ptr *T, void (*f)(void *, void *, void *), void *extra, void *extra2) {
  if (!T) {
    return;
  }
  tree_act_any_ex2(T->left, f, extra, extra2);
  f(T->x, extra, extra2);
  tree_act_any_ex2(T->right, f, extra, extra2);
}

#define DEFINE_HASH(prefix, name, value_t, value_compare, value_hash)                                                                                          \
  prefix hash_elem_##name##_t *hash_lookup_##name(hash_table_##name##_t *T, value_t x) __attribute__((unused));                                                \
  prefix void hash_insert_##name(hash_table_##name##_t *T, value_t x) __attribute__((unused));                                                                 \
  prefix int hash_delete_##name(hash_table_##name##_t *T, value_t x) __attribute__((unused));                                                                  \
  prefix void hash_clear_##name(hash_table_##name##_t *T) __attribute__((unused));                                                                             \
  prefix void hash_clear_act_##name(hash_table_##name##_t *T, void (*act)(value_t)) __attribute__((unused));                                                   \
  prefix hash_elem_##name##_t *hash_lookup_##name(hash_table_##name##_t *T, value_t x) {                                                                       \
    long long hash = value_hash(x);                                                                                                                            \
    if (hash < 0) {                                                                                                                                            \
      hash = -hash;                                                                                                                                            \
    }                                                                                                                                                          \
    if (hash < 0) {                                                                                                                                            \
      hash = 0;                                                                                                                                                \
    }                                                                                                                                                          \
    if (T->mask) {                                                                                                                                             \
      hash = hash & T->mask;                                                                                                                                   \
    } else {                                                                                                                                                   \
      hash %= (T->size);                                                                                                                                       \
    }                                                                                                                                                          \
    if (!T->E[hash]) {                                                                                                                                         \
      return 0;                                                                                                                                                \
    }                                                                                                                                                          \
    hash_elem_##name##_t *E = T->E[hash];                                                                                                                      \
    do {                                                                                                                                                       \
      if (!value_compare(E->x, x)) {                                                                                                                           \
        return E;                                                                                                                                              \
      }                                                                                                                                                        \
      E = E->next;                                                                                                                                             \
    } while (E != T->E[hash]);                                                                                                                                 \
    return 0;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  prefix void hash_insert_##name(hash_table_##name##_t *T, value_t x) {                                                                                        \
    long long hash = value_hash(x);                                                                                                                            \
    if (hash < 0) {                                                                                                                                            \
      hash = -hash;                                                                                                                                            \
    }                                                                                                                                                          \
    if (hash < 0) {                                                                                                                                            \
      hash = 0;                                                                                                                                                \
    }                                                                                                                                                          \
    if (T->mask) {                                                                                                                                             \
      hash = hash & T->mask;                                                                                                                                   \
    } else {                                                                                                                                                   \
      hash %= (T->size);                                                                                                                                       \
    }                                                                                                                                                          \
    hash_elem_##name##_t *E = hash_alloc_##name(x);                                                                                                            \
    if (T->E[hash]) {                                                                                                                                          \
      E->next = T->E[hash];                                                                                                                                    \
      E->prev = T->E[hash]->prev;                                                                                                                              \
      E->next->prev = E;                                                                                                                                       \
      E->prev->next = E;                                                                                                                                       \
    } else {                                                                                                                                                   \
      T->E[hash] = E;                                                                                                                                          \
      E->next = E;                                                                                                                                             \
      E->prev = E;                                                                                                                                             \
    }                                                                                                                                                          \
  }                                                                                                                                                            \
                                                                                                                                                               \
  prefix int hash_delete_##name(hash_table_##name##_t *T, value_t x) {                                                                                         \
    long long hash = value_hash(x);                                                                                                                            \
    if (hash < 0) {                                                                                                                                            \
      hash = -hash;                                                                                                                                            \
    }                                                                                                                                                          \
    if (hash < 0) {                                                                                                                                            \
      hash = 0;                                                                                                                                                \
    }                                                                                                                                                          \
    if (T->mask) {                                                                                                                                             \
      hash = hash & T->mask;                                                                                                                                   \
    } else {                                                                                                                                                   \
      hash %= (T->size);                                                                                                                                       \
    }                                                                                                                                                          \
    if (!T->E[hash]) {                                                                                                                                         \
      return 0;                                                                                                                                                \
    }                                                                                                                                                          \
    hash_elem_##name##_t *E = T->E[hash];                                                                                                                      \
    int ok = 0;                                                                                                                                                \
    do {                                                                                                                                                       \
      if (!value_compare(E->x, x)) {                                                                                                                           \
        ok = 1;                                                                                                                                                \
        break;                                                                                                                                                 \
      }                                                                                                                                                        \
      E = E->next;                                                                                                                                             \
    } while (E != T->E[hash]);                                                                                                                                 \
    if (!ok) {                                                                                                                                                 \
      return 0;                                                                                                                                                \
    }                                                                                                                                                          \
    E->next->prev = E->prev;                                                                                                                                   \
    E->prev->next = E->next;                                                                                                                                   \
    if (T->E[hash] != E) {                                                                                                                                     \
      hash_free_##name(E);                                                                                                                                     \
    } else if (E->next == E) {                                                                                                                                 \
      T->E[hash] = 0;                                                                                                                                          \
      hash_free_##name(E);                                                                                                                                     \
    } else {                                                                                                                                                   \
      T->E[hash] = E->next;                                                                                                                                    \
      hash_free_##name(E);                                                                                                                                     \
    }                                                                                                                                                          \
    return 1;                                                                                                                                                  \
  }                                                                                                                                                            \
                                                                                                                                                               \
  prefix void hash_clear_##name(hash_table_##name##_t *T) {                                                                                                    \
    int i;                                                                                                                                                     \
    for (i = 0; i < T->size; i++) {                                                                                                                            \
      if (T->E[i]) {                                                                                                                                           \
        hash_elem_##name##_t *cur = T->E[i];                                                                                                                   \
        hash_elem_##name##_t *first = cur;                                                                                                                     \
        do {                                                                                                                                                   \
          hash_elem_##name##_t *next = cur->next;                                                                                                              \
          hash_free_##name(cur);                                                                                                                               \
          cur = next;                                                                                                                                          \
        } while (cur != first);                                                                                                                                \
        T->E[i] = 0;                                                                                                                                           \
      }                                                                                                                                                        \
    }                                                                                                                                                          \
  }                                                                                                                                                            \
                                                                                                                                                               \
  prefix void hash_clear_act_##name(hash_table_##name##_t *T, void (*act)(value_t)) {                                                                          \
    int i;                                                                                                                                                     \
    for (i = 0; i < T->size; i++) {                                                                                                                            \
      if (T->E[i]) {                                                                                                                                           \
        hash_elem_##name##_t *cur = T->E[i];                                                                                                                   \
        hash_elem_##name##_t *first = cur;                                                                                                                     \
        do {                                                                                                                                                   \
          hash_elem_##name##_t *next = cur->next;                                                                                                              \
          act(cur->x);                                                                                                                                         \
          hash_free_##name(cur);                                                                                                                               \
          cur = next;                                                                                                                                          \
        } while (cur != first);                                                                                                                                \
        T->E[i] = 0;                                                                                                                                           \
      }                                                                                                                                                        \
    }                                                                                                                                                          \
  }


#define DEFINE_HASH_STDNOZ_ALLOC_PREFIX(prefix, name, value_t, value_compare, value_hash)                                                                      \
  DECLARE_HASH_TYPE(name, value_t)                                                                                                                             \
  prefix hash_elem_##name##_t *hash_alloc_##name(value_t x);                                                                                                   \
  prefix void hash_free_##name(hash_elem_##name##_t *T);                                                                                                       \
  DEFINE_HASH(prefix, name, value_t, value_compare, value_hash);                                                                                               \
  hash_elem_##name##_t *hash_alloc_##name(value_t x) {                                                                                                         \
    hash_elem_##name##_t *E = (hash_elem_##name##_t *)malloc(sizeof(*E));                                                                                      \
    E->x = x;                                                                                                                                                  \
    return E;                                                                                                                                                  \
  }                                                                                                                                                            \
  void hash_free_##name(hash_elem_##name##_t *E) {                                                                                                             \
    free(E);                                                                                                                                                   \
  }

#define DEFINE_HASH_STDNOZ_ALLOC(name, value_t, value_compare, value_hash) DEFINE_HASH_STDNOZ_ALLOC_PREFIX(static, name, value_t, value_compare, value_hash)

#define DECLARE_HASH_TYPE(name, value_t)                                                                                                                       \
  struct hash_elem_##name {                                                                                                                                    \
    struct hash_elem_##name *next, *prev;                                                                                                                      \
    value_t x;                                                                                                                                                 \
  };                                                                                                                                                           \
  struct hash_table_##name {                                                                                                                                   \
    struct hash_elem_##name **E;                                                                                                                               \
    int size;                                                                                                                                                  \
    int mask;                                                                                                                                                  \
  };                                                                                                                                                           \
  typedef struct hash_elem_##name hash_elem_##name##_t;                                                                                                        \
  typedef struct hash_table_##name hash_table_##name##_t;

#define std_int_compare(a, b) ((a) - (b))
#define std_ll_ptr_compare(a, b) ((*(long long *)(a)) - (*(long long *)(b)))
