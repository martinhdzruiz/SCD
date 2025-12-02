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

const int num_fumadores = 4 ;
int MOSTRADOR=-1;             //Inicilizado vacio
int total_fumado=0;
Semaphore poner_ingredinete=1;
Semaphore despertar_fumador[3]={0,0,0};
Semaphore avisar_estanquero=0;

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
   while (true) {
   
   int inngrediente= producir_ingrediente();
   poner_ingredinete.sem_wait();
   MOSTRADOR=inngrediente;
   despertar_fumador[inngrediente].sem_signal();
   cout<< "Estanquero  : pone el ingrediente "<<MOSTRADOR<< " en el mostardor"<<endl;
   
   }


}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<10,20>() );

   // informa de que comienza a fumar

   cout << "Fumador " << num_fumador << "  :"
         << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

   cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
   total_fumado++;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   int fuma;
   while( true )
   {
      despertar_fumador[num_fumador].sem_wait();
      fuma=MOSTRADOR;
      MOSTRADOR =-1;

      if(total_fumado%2==0){
         poner_ingredinete.sem_signal();
         fumar(fuma);
      }else{
         fumar(fuma);
         poner_ingredinete.sem_signal();
      }
      cout<<"Total fumado: "<<total_fumado<<endl;
      
   }
}

//----------------------------------------------------------------------

int main()
{
   thread Estanquero= thread (funcion_hebra_estanquero);
   thread fumadores[num_fumadores];

   for (int i=0; i<num_fumadores; i++){
      fumadores[i]=thread (funcion_hebra_fumador,i);
   }

   for(int i=0; i<num_fumadores; i++){
      fumadores[i].join();
   }

   Estanquero.join();


}
