
!!
!! Test program for the geometry shared spaces.
!!
!! To run the program do:
!! ./app npapp npx npy npz spx spy spz timestems
!! The arguments mean:
!!     npapp - number of processors in the application
!!     np{x,y,z} - number of processors in the * direction (data decomposition)
!!     sp{x,y,z} - size of the data block in the * direction
!!     timestep  - number of iterations

program test_put
  use couple_comm
  implicit none

  include 'mpif.h'

  integer :: err
  ! MPI variables
  integer :: group_comm
  integer :: mpirank, mpisize
  integer :: ierr

  call MPI_Init (ierr)
  call MPI_Comm_dup (MPI_COMM_WORLD, group_comm, ierr)
  call MPI_Comm_rank (MPI_COMM_WORLD, mpirank, ierr)
  call MPI_Comm_size (group_comm, mpisize , ierr)


  call system('sleep 2')

!! This is an example on how to parse command line arguments in Fortran
!   if (iargc() == 0) then
!      print *, "Please provide command line arguments."
!      stop
!   endif
!   call getarg(1, arg)

  call parse_args()

  call dspaces_init(npapp,1, err)
  call dspaces_rank(rank)
  call dspaces_peers(nproc)
  call timer_init()
  call timer_start()
  call allocate_3d()

  do ts=1,timesteps
     call generate_3d()
     call couple_write_3d()
  enddo

!! call test_3d(n, loop)
!! call test_2d(128, 1001)
!! call test_2d_block_block ! (n, loop)

  call dspaces_barrier
  call dspaces_finalize
  call MPI_Finalize(ierr)

end program

subroutine matrix_inc(a, n)
  real (kind=8) :: a(n,n)

  do i=1,n
     do j = 1,n
        a(i,j) = a(i,j) + 10.0
     enddo
  enddo

end subroutine


subroutine test_3d(n, loop)
  real (kind=8), dimension(:,:,:), allocatable :: m3d
  integer :: i, j, k
  real (kind=8) :: f
  real (kind = 8) :: tm_tab(10), tm_start, tm_end
  integer :: rank, peers

  call dspaces_rank(rank)
  call dspaces_peers(peers)

  !! Data decomposition: (block, *, *)
  allocate(m3d(n/peers,n,n))

  f = 1.0 + rank * n * n/peers * n
  do i=1,n/peers
     do j=1,n
        do k=1,n
           m3d(i,j,k) = f 
           f = f + 1.0
        enddo
     enddo
  enddo

  j=(rank * n/peers)+1
  k=((rank+1) * n/peers)

  do i=1,loop
!     call sleep(5)
     call dspaces_lock_on_write

!     call timer_read(tm_start)

     !! Syntax is: var_name, version, size_elem, {bounding box}, data values.
     !! call dspaces_put("m3d", i-1, 8, 0, j-1, 0, n-1, k-1, n-1, m3d(j:k,1:n,1:n))
     call dspaces_put("m3d", i-1, 8, 0, j-1, 0, n-1, k-1, n-1, m3d)

!     call timer_read(tm_end)
     tm_tab(1) = tm_end - tm_start

     call dspaces_put_sync
!     call timer_read(tm_end)
     tm_tab(2) = tm_end - tm_start

     print *, "Ok, I put ", rank+1, ", part of the array."
     call dspaces_unlock_on_write

     !! I have 2 timers to report
!     call timer_log(tm_tab, 2)
  enddo

  deallocate(m3d)
end subroutine


subroutine test_2d(n, loop)
  integer :: dens(n)
  real (kind=8) :: a(n,n)
  integer :: i, j, k
  real (kind=8) :: f = 1.0
  real (kind = 8) :: tm_tab(10), tm_start, tm_end
  integer :: rank, peers

  call dspaces_rank(rank)
  call dspaces_peers(peer)

  do i=1,n
     do j=1,n
        a(i,j) = f
        f = f + 1.0
     enddo
     dens(i)=i+10
  enddo

!! Compute the distribution based on processor rank. Divide the matrix
!! based on rows: rank0 : [1 ..n/2), rank1: [n/2, n]
!! Distribution: (*, block)

 j=(rank * n/2)+1
 k=((rank+1) * n/2)
 do i=1,1001
!    call sleep(5)
    call dspaces_lock_on_write

    call dspaces_put("psi", rank, 4, rank, rank, rank, rank, rank, rank, i)
    call dspaces_put("dens", i-1, 4, j-1, 0, 0, k-1, 0, 0, dens(j:k))

    if (rank == 0) then
       call dspaces_put("m3d", i-1, 4, 0, 0, 0, 7, 7, 7, m3d)
    endif

!    call timer_read(tm_start)
    !! Syntax is: var_name, version, size_elem, {bounding box}, data values.
    call dspaces_put("flux_curent", i-1, 8, 0, j-1, 0, n-1, k-1, 0, a(j:k, 1:n))
!    call timer_read(tm_end)
    tm_tab(1) = tm_end - tm_start

    call dspaces_put_sync
!    call timer_read(tm_end)
    tm_tab(2) = tm_end - tm_start

    print *, "Ok, I put ", rank+1, ", part of the array."
    call dspaces_unlock_on_write

    !! I have 2 timers to report
!    call timer_log(tm_tab, 2)

    !! call matrix_inc(a, n)
 enddo
end subroutine


subroutine couple_write_2d()
  use couple_comm
  implicit none

!  call sleep(5)
  call dspaces_lock_on_write

  call dspaces_put("m2d", ts, 8, &
       off_x, off_y, 0, &
       off_x+spx-1, off_y+spy-1, 0, &
       m2d)
!  call dspaces_put_sync

  call dspaces_unlock_on_write
end subroutine ! couple_write_2d


subroutine couple_write_3d()
  use couple_comm
  implicit none

  real (kind=8) :: tm_tab(2), tm_start, tm_end
  integer :: M = -3, N = 5
  integer :: err
  m3d(spy,spx,spz) = -m3d(spy,spx,spz)
!  call sleep(5)
!  print *, "before write lock."
  call dspaces_lock_on_write("common_lock")
!  print *, "after write lock."
!   call dspaces_put("M", 0, 4, &
!        0, 0, 0, &
!        0, 0, 0, &
!        M)

!   call dspaces_put("N", 0, 4, &
!        0, 0, 0, &
!        0, 0, 0, &
!        N)

  !! Version number is forced to 0 (saves some memory space).
!  call timer_read(tm_start)
  call dspaces_put("/m3d_with_path_and_some_more_chars/value", ts-ts, 8, &
       off_x, off_y, off_z, &
       off_x+spx-1, off_y+spy-1, off_z+spz-1, &
       m3d, err)

!  call timer_read(tm_end)
  tm_tab(1) = tm_end - tm_start

  call dspaces_put_sync(err)!problem
!  call timer_read(tm_end)
  tm_tab(2) = tm_end - tm_start
 
!   call sleep(5);

!  print *, "before unlock."

  call dspaces_unlock_on_write("common_lock")
!  print *, "af unlock."
!  call timer_log(tm_tab, 2)

  print *, 'min value written to the space is: ', m3d(1,1,1)
end subroutine !couple_write_3d
