#pragma once
#include "PHP/common.h"

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"
#include "compiler/types.h"
#include "compiler/location.h"

namespace tinf {
  class Edge;
  class Node;
  class TypeInferer;

  struct RestrictionBase {
    virtual ~RestrictionBase() {}
    virtual const char *get_description() = 0;
    virtual int check_impl() = 0;
    virtual void check() = 0;
  };

  class Node : public Lockable {
    private:
      vector <Edge *> next_;
      vector <Edge *> rev_next_;
      volatile int recalc_state_;
      volatile int holder_id_;
    public:
      const TypeData *type_;
      volatile int recalc_cnt_;
      int isset_flags;
      int isset_was;

      enum {empty_st, own_st, own_recalc_st};

      Node();

      int get_recalc_cnt();
      int get_holder_id();

      void add_edge (Edge *edge);
      void add_rev_edge(Edge *edge);

      inline Range <vector <Edge *>::iterator> next_range() {
        return all (next_);
      }
      inline Range <vector <Edge *>::iterator> rev_next_range() {
        return all (rev_next_);
      }

      bool try_start_recalc();
      void start_recalc();
      bool try_finish_recalc();

      const TypeData *get_type() const {
        return type_;
      }
      void set_type (const TypeData *type) {
        type_ = type;
      }
      virtual void recalc (TypeInferer *inferer) = 0;
      virtual string get_description() = 0;
  };

  class VarNode : public Node {
    public:
      VarPtr var_;
      int param_i;
      FunctionPtr function_;
      VarNode (VarPtr var = VarPtr()) :
        var_ (var),
        param_i (-2) {
        }
      void copy_type_from (const TypeData *from) {
        type_ = from;
        recalc_cnt_ = 1;
      }
      void recalc (TypeInferer *inferer);
      VarPtr get_var() {
        return var_;
      }
      string get_description();
  };
  class ExprNode : public Node {
    private:
      VertexPtr expr_;
    public:
      ExprNode (VertexPtr expr = VertexPtr()) :
        expr_ (expr) {
      }
      void recalc (TypeInferer *inferer);
      VertexPtr get_expr() {
        return expr_;
      }
      string get_description();
      Location const & get_location();
  };
  class TypeNode : public Node {
    public:
      TypeNode (const TypeData *type) {
        set_type (type);
      }
      void recalc (TypeInferer *inferer __attribute__((unused))) {
      }
      string get_description();
  };

  class Edge {
    public :
      Node *from;
      Node *to;

      const MultiKey *from_at;
  };

  typedef queue <Node *> NodeQueue;
  class TypeInferer {
    private:
      int do_run_queue();
      TLS <vector <RestrictionBase *> > restrictions;
      bool finish_flag;
    public:
      TypeInferer();
      TLS <NodeQueue> Q;
      void add_edge (Edge *edge);

      void recalc_node (Node *node);
      bool add_node (Node *node);

      void add_restriction (RestrictionBase *restriction);
      void check_restrictions();

      int run_queue (NodeQueue *q);
      vector <Task *> get_tasks();

      void run_node (Node *node);
      const TypeData *get_type (Node *node);

      void finish();
      bool is_finished();
  };

  void register_inferer (TypeInferer *inferer);
  TypeInferer *get_inferer();
  const TypeData *get_type (VertexPtr vertex);
  const TypeData *get_type (VarPtr var);
  const TypeData *get_type (FunctionPtr function, int id);
}

