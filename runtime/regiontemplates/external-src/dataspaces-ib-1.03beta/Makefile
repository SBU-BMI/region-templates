SHELL   = /bin/sh
CC=mpicc -fPIC -fpermissive
CPP=mpic++ -fPIC
CFLAGS=-O3 -c
FTN=mpif90
LDFLAGS=-lrdmacm



SRC1 = bbox.c \
       ss_data.c \
       timer.c \
       dart_rpc_ib.c \
       ds_base_ib.c \
       dc_base_ib.c

SRC2 =  dc_gspace.c \
        ds_gspace.c

SRC3 =  dataspaces.c

SRC4 =  test_get.f90 \
	test_put.f90 \
	couple_mod.f90

SRC5 =	test_dsspace.c \

SRC4 =  test_get_mpi.f90 \
        test_put_mpi.f90 \
        couple_mod.f90

OBJ1 =	$(SRC1:.c=.o)
OBJ2 =	$(SRC2:.c=.o)
OBJ3 =	$(SRC3:.c=.o)
OBJ4 =	$(SRC4:.f90=.o)
OBJ5 =	$(SRC5:.c=.o)
OBJ6 =  $(SRC6:.f90=.o)
OBJ7 = $(SRC1:.c=.o) $(SRC2:.c=.o) $(SRC3:.c=.o)

OUT1 =	./libdart.a
OUT2 =	./libdscommon.a
OUT3 =  ./libdspaces.a
OUT4 =	./test_get \
	./test_put
OUT5 = ./dataspaces_server

OUT7 = ./libdspacesAll.so

couple_mod.o: couple_mod.f90
	$(FTN) -g -c $<
test_put.o: test_put.f90
	gfortran $(FLAGS) -g -c $<
test_get.o: test_get.f90
	gfortran $(FLAGS) -g -c $<

test_put_mpi.o: test_put_mpi.f90
	$(FTN) $(FLAGS) -g -c $<
test_get_mpi.o: test_get_mpi.f90
	$(FTN) $(FLAGS) -g -c $<


test_get: test_get.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3)
	gfortran -o $@ test_get.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)

test_put: test_put.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3)
	gfortran -o $@ test_put.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)

test_get_mpi: test_get_mpi.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3)
	$(FTN) -o $@ test_get_mpi.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)

test_put_mpi: test_put_mpi.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3)
	$(FTN) -o $@ test_put_mpi.o couple_mod.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)

# adding compilation rule for generation of the C clients

util.o: util.cpp
	$(CPP) -c util.cpp

#test_put_mpi_c: lib util.o
#	$(CPP) -o $@ test_put_mpi_c.c util.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)

test_put_mpi_c_2D: lib util.o
	$(CPP) -o $@ test_put_mpi_c_2D.c util.o -L. -ldspacesAll $(LDFLAGS)
#	$(CPP) -o $@ test_put_mpi_c_2D.c util.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)
#
#test_get_mpi_c_2D: lib util.o
#	$(CPP) -o $@ test_get_mpi_c_2D.c util.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)
#

test_dsspace: test_dsspace.o $(OBJ1) $(OBJ2) $(OBJ3)
	$(CC) -o $@ test_dsspace.o $(OBJ1) $(OBJ2) $(OBJ3) $(LDFLAGS)

test: couple_mod.o test_get_mpi test_put_mpi test_dsspace test_put_mpi_c_2D
	mv test_dsspace dataspaces_server
	mv test_get_mpi test_get
	mv test_put_mpi test_put


default: all

lib: $(OUT1) $(OUT2) $(OUT3) $(OUT7)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
.f90.o:
	$(FTN) -g -c $< -o $@
.cpp.o:
	$(CPP) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OUT1): $(OBJ1)
	ar rcs $(OUT1) $(OBJ1)

$(OUT2): $(OBJ2)
	ar rcs $(OUT2) $(OBJ2)

$(OUT3): $(OBJ3)
	ar rcs $(OUT3) $(OBJ3)

$(OUT7): $(OBJ7)
	$(CC) -fPIC -shared -o $(OUT7) $(OBJ7) $(LDFLAGS)
#	ar rcs $(OUT7) $(OBJ7)
clean:
	rm -f $(OUT1) $(OUT2) $(OUT3) $(OUT4) $(OUT5) $(OUT7)\
        $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) couple_mod.o conf srv.lck

    
