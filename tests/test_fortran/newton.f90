!
! Numerical Mathematics and Computing, Fifth Edition
! Ward Cheney & David Kincaid
! Brooks/Cole Publ. Co.
! Copyright (c) 2003.  All rights reserved.
! For educational use with the Cheney-Kincaid textbook.
! Absolutely no warranty implied or expressed.
!
! Section 3.2
!
! File: newton.f90
!
! Sample Newton's method program using subroutine newton(f,fp,x,m)
!
program main
  integer, parameter :: dp = kind (1d0)
  real (kind = dp) :: x, true
  integer :: m

  interface
     function f(x)
       integer, parameter :: dp = kind(1d0)
       real (kind=dp), intent(in) :: x
     end function f
     function g(x)
       integer, parameter :: dp = kind (1d0)
       real (kind=dp), intent(in) :: x
     end function g
  end interface

  m = 6
  x  =  4.0_dp
  call newton(f,g,x,m)
end program main

subroutine newton(f,g,x,m)
  interface
     function f(x)
       integer, parameter :: dp = kind(1d0)
       real (kind=dp), intent(in) :: x
     end function f
     function g(x)
       integer, parameter :: dp = kind(1d0)
       real (kind =dp), intent(in) :: x
     end function g
  end interface
  integer, parameter :: dp = kind(1d0)
  real (kind=dp) :: x
  integer, intent(in) :: m

  print *, "n         x                f(x)"
  fx = f(x)
  print *, 0, x, fx
  do n = 1,m
     x = x - fx/g(x)
     fx = f(x)
     print "(i2, f20.16, f20.16)", n, x, fx
  end do
end subroutine newton

function f(x)
  integer, parameter :: dp = kind (1d0)
  real (kind = dp) , intent(in) :: x
  f = ((x - 2.0_dp)*x + 1.0_dp)*x - 3.0_dp
end function f

function g(x)
  integer, parameter  :: dp = kind (1d0)
  real (kind = dp), intent(in) :: x
  g = (3.0_dp*x - 4.0_dp)*x + 1.0_dp
end function g
