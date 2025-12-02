#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// numero de fumadores 

const int num_fumadores = 3 ;
Semaphore mostrador_vacio=1;        //Inicializamos el semáforo del mostrador a uno para indicar que esta vacio
Semaphore ingr_disp[num_fumadores]={0,0,0};
mutex mtx;


//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   while (true){
      int producto = producir_ingrediente();    // inicializamos el bucle llamando a la funcion que genera el producto y 
                                                //posteriormente pasamos a preguntar si hay espacio en el mostrador para colocar le producto
      sem_wait(mostrador_vacio);
      cout<<"Ingrediente puesto en el mostrador: "<<producto<<endl;

      sem_signal(ingr_disp[producto]);


   }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {

      sem_wait (ingr_disp[num_fumador]);
      mtx.lock();
      cout<<"Retirado del monstrador producto: "<<num_fumador<<endl;
      mtx.unlock();
      sem_signal(mostrador_vacio);
      fumar(num_fumador);

   }
}

//----------------------------------------------------------------------

int main()
{
   thread estanquero(funcion_hebra_estanquero);
   thread array_fumadores[num_fumadores];

   for (int i=0;i<num_fumadores; i++){
      array_fumadores[i]=thread(funcion_hebra_fumador,i);

   }
   
   for (int i=0; i<num_fumadores;i++){

      array_fumadores[i].join();
   }


   cout<<"Todo salió bien:"<<endl;
}
