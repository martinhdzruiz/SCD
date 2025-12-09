.SUFFIXES:
.PHONY:    pc,pc2,pcm,fp,f,fi,fc,clean

compilador := mpicxx 
flagsc     := -std=c++11 -g -Wall

pc: prodcons_mpi_exe
	mpirun -oversubscribe -np  3 ./$<

pca: prodcons-alt_mpi_exe
	mpirun -oversubscribe  -np  3 ./$<

pc2: prodcons2_mpi_exe
	mpirun -oversubscribe  -np  10 ./$<

pcm: prodcons-mu_mpi_exe
	mpirun -oversubscribe  -np  10 ./$<

pcmf: prodcons-mu-falsificador_mpi_exe

	mpirun -oversubscribe  -np  16 ./$<

pcmv : prodcons-mu-veri_mpi_exe
	mpirun -oversubscribe  -np  11 ./$<

fp: filosofos-plantilla_mpi_exe
	mpirun -oversubscribe  -np 10 ./$<

fi: filosofos-interb_mpi_exe
	mpirun -oversubscribe  -np 10 ./$<

f: filosofos_mpi_exe
	mpirun -oversubscribe  -np 10 ./$<

fc: filosofos-cam_mpi_exe
	mpirun -oversubscribe  -np 11 ./$< 
	

%_mpi_exe: %.cpp
	$(compilador) $(flagsc) -o $@ $<

clean:
	rm -rf *_exe *.dSYM
