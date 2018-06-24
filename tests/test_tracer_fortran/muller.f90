program main
  integer, parameter :: dp = kind(1d0)
  real(kind=dp)    :: x, u_k, u_k1, t
  integer            :: i,iter = 30

  interface
     function muller1(x) result(res)
       integer, parameter :: dp = kind(1d0)
       real(kind=dp), intent(in) :: x
       real(kind=dp)             :: res
     end function muller1
     function muller2(u_k, u_k1) result(res)
       integer, parameter :: dp = kind(1d0)
       real(kind=dp), intent(in) :: u_k,u_k1
       real(kind=dp)             :: res
     end function muller2
  end interface
  
  x = 1.5100050721319

  do i=1,iter
     x = muller1(x)
  end do

  write(*,*) x

  u_k = -4.0
  u_k1 = 2.0
  do i=1,iter
     t = u_k
     u_k = muller2(u_k, u_k1)
     u_k1 = t
  end do
  
end program main

function muller1(x) result(res)
  implicit none
  integer, parameter        :: dp = kind (1d0)
  real(kind=dp), intent(in) :: x
  real(kind=dp)             :: res
  res = (3.0_dp*x**4.0 - 20.0_dp*x**3.0 + 35.0_dp*x**2.0 - 24.0_dp) &
       / (4.0_dp*x**3.0 - 30.0_dp*x**2.0 + 70.0_dp*x - 50.0_dp)
end function muller1

function muller2(u_k, u_k1) result(res)
  implicit none
  integer, parameter        :: dp = kind (1d0)
  real(kind=dp), intent(in) :: u_k, u_k1
  real(kind=dp)             :: res
  res = 111.0_dp - 1130.0_dp/u_k + 3000.0_dp/(u_k*u_k1)
end function muller2
