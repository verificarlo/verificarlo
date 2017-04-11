/* numdbg backend interface */
struct numdbg_backend_interface_t {
  void (*numdbg_add_float)(float, float, float*, void*);
  void (*numdbg_sub_float)(float, float, float*, void*);
  void (*numdbg_mul_float)(float, float, float*, void*);
  void (*numdbg_div_float)(float, float, float*, void*);

  void (*numdbg_add_double)(double, double, double*, void*);
  void (*numdbg_sub_double)(double, double, double*, void*);
  void (*numdbg_mul_double)(double, double, double*, void*);
  void (*numdbg_div_double)(double, double, double*, void*);
};
