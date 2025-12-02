// faltan los metodos del monitor con sus colas condicion
//  -----------------------------------------------------------------------------
//
//  Sistemas concurrentes y Distribuidos.
//  Seminario 2. Introducción a los monitores en C++11.
//
//  Archivo: prodcons1_su.cpp
//
//  Ejemplo de un monitor en C++11 con semántica SU, para el problema
//  del productor/consumidor, con productor y consumidor únicos.
//  Opcion LIFO
//
//  Historial:
//  Creado el 30 Sept de 2022. (adaptado de prodcons2_su.cpp)
//  20 oct 22 --> paso este archivo de FIFO a LIFO, para que se corresponda con lo que dicen las transparencias
//  -----------------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

const int
    num_fumadores = 4, // número de fumadores
    num_ingredientes = 3;

constexpr int
    min_ms = 5,  // tiempo minimo de espera en sleep_for
    max_ms = 20; // tiempo máximo de espera en sleep_for

int cuenta_cigarro[num_fumadores] = {0};
int cuenta_ingr_0 = 0;
mutex
    mtx; // mutex de escritura en pantalla

int producir_ingrediente()
{

   this_thread::sleep_for(chrono::milliseconds(aleatorio<10, 1000>()));

   const int num_ingrediente = aleatorio<0, num_ingredientes - 1>();
   cout << "Estanquero : empieza a producir ingrediente (" << num_ingrediente << ")" << endl;
   if (num_ingrediente == 0)
      cuenta_ingr_0++;
   return num_ingrediente;
}

void fumar(int num_fumador)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)

   // informa de que comienza a fumar
   mtx.lock();
   cout << "Fumador " << num_fumador << "  :"
        << " empieza a fumar" << endl;
   mtx.unlock();

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for(chrono::milliseconds(aleatorio<min_ms, max_ms>()));

   // informa de que ha terminado de fumar
   cuenta_cigarro[num_fumador]++;
   cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}
//----------------------------------------------------------------------

// clase para monitor buffer, version FIFO, semántica SC, multiples prod/cons

class Estanco : public HoareMonitor
{
private:
   int mostrador;
   CondVar                       // colas condicion:
       mostr_vacio,              //  cola donde espera el consumidor (n>0)
       ingr_disp[num_fumadores], //  cola donde espera el productor  (n<num_celdas_total)
       contadora;
   bool cuenta;

public:       // constructor y métodos públicos
   Estanco(); // constructor
   void ponerIngrediente(int ingre);
   void esperarRecogidaIngrediente();
   void obtenerIngrediente(int num_fumador);
   void cuenta_cigarros();
};
// -----------------------------------------------------------------------------

Estanco::Estanco()
{
   mostr_vacio = newCondVar();
   for (int i = 0; i < num_fumadores; i++)
   {
      ingr_disp[i] = newCondVar();
   }
   mostrador = -1; // indica que esta vacio
   cuenta = false;
   contadora = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

void Estanco::ponerIngrediente(int ingre)
{
   // esperar bloqueado hasta que 0 < primera_libre
   if (cuenta_ingr_0 == 15)
   {
      cuenta = true;
      contadora.signal();
      cuenta_ingr_0 = 0;
   }

   // hacer la operación de lectura, actualizando estado del monitor
   mostrador = ingre;
   cout << "Puesto ingr: " << ingre << endl;
   if (ingre == 0)
   {
      int aleator = aleatorio<0, 1>();
      if (aleator == 0)
      {
         ingr_disp[0].signal();
      }
      else
      {
         ingr_disp[3].signal();
      }
   }
   else
   {
      // señalar al productor que hay un hueco libre, por si está esperando
      ingr_disp[ingre].signal();
   }
}
// -----------------------------------------------------------------------------
void Estanco::obtenerIngrediente(int num_fumador)
{
   if (mostrador == -1)
   {
      if (num_fumador == 3)
      {
         cout << "Esperando a que este el ingrediente: 0" << endl;
      }
      else
      {
         cout << "Esperando a que este el ingrediente: " << num_fumador << endl;
      }
      ingr_disp[num_fumador].wait();
   }
   else if (mostrador == 0 && num_fumador == 3)
   {
      cout << "Ingrediente encontrado" << endl;
   }
   else if (mostrador != num_fumador)
   {
      cout << "Esperando a que este el ingrediente: " << num_fumador << endl;
      ingr_disp[num_fumador].wait();
   }
   mostrador = -1;
   mostr_vacio.signal();
}

// -----------------------------------------------------------------------------
void Estanco::esperarRecogidaIngrediente()
{
   // esperar bloqueado hasta que primera_libre < num_celdas_total
   if (mostrador != -1)
   {
      cout << "Esperando a que este vacio el mostrador" << endl;
      mostr_vacio.wait();
   }
}
void Estanco::cuenta_cigarros()
{

   if (!cuenta)
   {
      // cout<<"Esperando a que este vacio el mostrador"<<endl;
      contadora.wait();
   }
   cout << endl
        << endl
        << "El fumador 0 ha fumado: " << cuenta_cigarro[0] << endl
        << "El fumador 3 ha fumado: " << cuenta_cigarro[3] << endl;
   cuenta = false;
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
   int valor;
   while (true)
   {
      valor = producir_ingrediente();
      monitor->ponerIngrediente(valor);
      monitor->esperarRecogidaIngrediente();
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador)
{
   while (true)
   {
      monitor->obtenerIngrediente(num_fumador);
      if (num_fumador == 3)
      {

         cout << "Retirado ingr: 0" << endl;
      }
      else
         cout << "Retirado ingr: " << num_fumador << endl;
      fumar(num_fumador);
   }
}
void funcion_hebra_contadora(MRef<Estanco> monitor)
{
   while (true)
   {
      monitor->cuenta_cigarros();
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema de los fumadores. " << endl
        << "--------------------------------------------------------------------" << endl
        << flush;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Estanco> monitor = Create<Estanco>();

   // crear y lanzar las hebras

   thread hebra_fumadores[num_fumadores], hebra_estanquero(funcion_hebra_estanquero, monitor), hebra_contadora(funcion_hebra_contadora, monitor);
   for (int i = 0; i < num_fumadores; i++)
   {
      hebra_fumadores[i] = thread(funcion_hebra_fumador, monitor, i);
   }

   for (int i = 0; i < num_fumadores; i++)
   {
      hebra_fumadores[i].join();
   }
   hebra_estanquero.join();
   hebra_contadora.join();
}
