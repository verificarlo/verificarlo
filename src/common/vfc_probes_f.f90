module vfc_probes_f
    use, intrinsic :: iso_c_binding, only: C_PTR, C_DOUBLE, C_SIZE_T, C_INT, C_CHAR

    ! Structures

    type, bind(C) :: vfc_probe_node
        type(C_PTR) :: key
        real(kind=C_DOUBLE) :: val
    end type vfc_probe_node


    type, bind(C) :: vfc_hashmap_t
        integer(kind=C_SIZE_T) :: nbits
        integer(kind=C_SIZE_T) :: mask

        integer(kind=C_SIZE_T) :: capacity
        type(C_PTR) :: items
        integer(kind=C_SIZE_T) :: nitems
        integer(kind=C_SIZE_T) :: n_deleted_items
    end type vfc_hashmap_t


    type, bind(C) :: vfc_probes
        type(C_PTR) :: map
    end type vfc_probes


    ! Functions

    interface

        subroutine vfc_init_probes(probes) bind(C, name = "vfc_init_probes_f")
            import :: vfc_probes
            type(vfc_probes), intent(out) :: probes
        end subroutine vfc_init_probes

        function vfc_free_probes(probes) bind(C, name = "vfc_free_probes")
            use, intrinsic :: iso_c_binding, only: C_PTR, C_DOUBLE, C_SIZE_T, C_INT, C_CHAR
            import :: vfc_probes

            type(vfc_probes) :: probes
        end function vfc_free_probes

        integer(C_INT) function vfc_probe(probes, testName, varName, val) bind(C, name = "vfc_probe_f")
            use, intrinsic :: iso_c_binding, only: C_PTR, C_DOUBLE, C_SIZE_T, C_INT, C_CHAR
            import :: vfc_probes

            type(vfc_probes) :: probes
            character(kind=C_CHAR),dimension(*) :: testName
            character(kind=C_CHAR),dimension(*) :: varName
            real(kind=C_DOUBLE) :: val
        end function vfc_probe

        integer(C_SIZE_T) function vfc_num_probes(probes) bind(C, name = "vfc_num_probes")
            use, intrinsic :: iso_c_binding, only: C_PTR, C_DOUBLE, C_SIZE_T, C_INT, C_CHAR
            import :: vfc_probes

            type(vfc_probes) :: probes
        end function vfc_num_probes

        integer(C_INT) function vfc_dump_probes(probes) bind(C, name = "vfc_dump_probes")
            use, intrinsic :: iso_c_binding, only: C_PTR, C_DOUBLE, C_SIZE_T, C_INT, C_CHAR
            import :: vfc_probes

            type(vfc_probes) :: probes
        end function vfc_dump_probes

    end interface

end module vfc_probes_f
