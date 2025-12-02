.SUFFIXES:
.PHONY: pc, fu, pcpla, t, mon_fu
.PRECIOUS: %_exe

compilador:=g++

## prod-cons hecho, 1 prod. y 1 cons.
pc: prodcons_exe
	./$<

## prod-cons hecho, múltiples productores y consumidores
pcm: prodcons-multi_exe
	./$<

## prod-cons hecho, múltiples productores y consumidores
## (reparto dinámico de carga de trabajo con tipos atómicos)
pcmd: prodcons-multi-dynamic_exe
	./$<

## prod-cons hecho, 1 prod y 1 cons, FIFO, 
## (con hebra impresora)
pci: prodcons-impresora_exe
	./$<

## fumadores hecho
fu: fumadores_exe
	./$< 

## esto solo se usa para probar que la plantilla compila
## (da error al ejecutar)
pcpla: prodcons-plantilla_exe 
	./$< 

## esto solo se usa para probar que la plantilla compila
## (al ejecutarse no hace nada)
fupla: fumadores-plantilla_exe 
	./$< 


##monitores
mon_fu: fumadores-monitor_exe
	./$<
mon_fu2: fumadores-monitor2_exe
	./$<
mon-cuenta_cigarros: fumadores-monitor_cuenta_cigarros_exe
	./$<
mon-cuenta_cigarros_copy: fumadores-monitor_cuenta_cigarros_copy_exe
	./$<
mon-cuenta_cigarros_fer: fumadores_cuenta_cigarros_fer_exe
	./$<

mon-suministradora: fumadores-monitor_heb_suministradora_exe
	./$<


mon-pcL: prodcons-monitorLIFO_exe
	./$<
mon-pcF: prodcons-monitorFIFO_exe
	./$<

mon-lec-revisadora: lec-escr-revisadora_exe
	./$<
mon-lec: lec-escr_exe
	./$<

mon-lec_parejas: lec-escr_parejas_exe
	./$<

mon-lec-misma-prio: lec-escr-misma-prio_exe
	./$<

## solo para tests
t: test-semaforos_exe
	./$< | more

## regla para compilar (.cpp --> _exe)
%_exe: %.cpp scd.cpp scd.h
		$(compilador) -std=c++11 -pthread -o $@ $< scd.cpp

clean:
	rm -f *_exe
