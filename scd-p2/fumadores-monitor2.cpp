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
mutex mx;

class Estanco : public HoareMonitor{
   private:
      int Mostrador;
      CondVar estanquero, fumador_espera[num_fumadores];


   public:
      Estanco();
      void poner_ingrediente(const int ingrediente);
      void esperar_recogida();
      void obtener_ingrediente(const int num_fumador);

};

Estanco::Estanco(){
   Mostrador=-1;
   estanquero= newCondVar();

   for(int i=0; i<num_fumadores; i++){
      fumador_espera[i]=newCondVar();
   }

}


void Estanco::poner_ingrediente(const int ingrediente){
   
   Mostrador=ingrediente;
   fumador_espera[ingrediente].signal();

}

void Estanco::esperar_recogida(){
   if(Mostrador!=-1)
      estanquero.wait();
}

void Estanco::obtener_ingrediente(const int num_fumador){

   if(Mostrador==-1 || num_fumador!=Mostrador){
      fumador_espera[num_fumador].wait();
      
   }else{
      Mostrador=-1;
      estanquero.signal();
      
   }

}

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   mx.lock();
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   mx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;
   mx.lock();
   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
   mx.unlock();
   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
   while (true){
   int ingrediente=producir_ingrediente();
   monitor->poner_ingrediente(ingrediente);
   mx.lock();
   cout<<"Se ha puesto en el mopstrador el ingrediente: "<<ingrediente<<endl;
   mx.unlock();
   monitor->esperar_recogida();
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
   mx.lock();
   cout << "Fumador " << num_fumador << "  :"
         << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
   mx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );
   // informa de que ha terminado de fumar
   mx.lock();
   cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
   mx.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador, MRef<Estanco> monitor )
{
   while( true )
   {
      monitor->obtener_ingrediente(num_fumador);
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   MRef<Estanco> monitor=Create<Estanco>();
   thread hebra_fumadores[num_fumadores];
   thread Estanquero= thread(funcion_hebra_estanquero, monitor);
   for(int i=0; i<num_fumadores; i++){
      hebra_fumadores[i]=thread(funcion_hebra_fumador,i, monitor);


   }

   for (int i=0; i<num_fumadores; i++){
      hebra_fumadores[i].join();
   }
   Estanquero.join();
}
