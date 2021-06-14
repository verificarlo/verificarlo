program vfc_probes_test
  use iso_c_binding
  use vfc_probes_f
  implicit none

  type(vfc_probes) :: probes
  integer(C_INT) :: err
  real(kind=C_DOUBLE) :: var = 3.14

  probes = vfc_init_probes()

  err = vfc_probe(probes, "test", "var", var)
  print *, "Num probes :", vfc_num_probes(probes)

  err = vfc_dump_probes(probes)
end program vfc_probes_test
