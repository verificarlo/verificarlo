/* interflop backend interface */

/* float compare predicates, follows the same order than
 * LLVM's FCMPInstruction predicates */
enum FCMP_PREDICATE {
  FCMP_FALSE,
  FCMP_OEQ,
  FCMP_OGT,
  FCMP_OGE,
  FCMP_OLT,
  FCMP_OLE,
  FCMP_ONE,
  FCMP_ORD,
  FCMP_UNO,
  FCMP_UEQ,
  FCMP_UGT,
  FCMP_UGE,
  FCMP_ULT,
  FCMP_ULE,
  FCMP_UNE,
  FCMP_TRUE,
};

struct interflop_backend_interface_t {
  void (*interflop_add_float)(float, float, float *, void *);
  void (*interflop_sub_float)(float, float, float *, void *);
  void (*interflop_mul_float)(float, float, float *, void *);
  void (*interflop_div_float)(float, float, float *, void *);
  void (*interflop_cmp_float)(enum FCMP_PREDICATE, float, float, bool *,
                              void *);

  void (*interflop_add_double)(double, double, double *, void *);
  void (*interflop_sub_double)(double, double, double *, void *);
  void (*interflop_mul_double)(double, double, double *, void *);
  void (*interflop_div_double)(double, double, double *, void *);
  void (*interflop_cmp_double)(enum FCMP_PREDICATE, double, double, bool *,
                               void *);
};

/* interflop_init: called at initialization before using a backend.
 * It returns an interflop_backend_interface_t structure with callbacks
 * for each of the numerical instrument hooks.
 *
 * argv will be deallocated after the call. If the backends, wants to make it
 * persistent it should copy it.
 * */

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context);
