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
   
   while (Mostrador != -1) estanquero.wait();
   
      Mostrador=ingrediente;

}

void Estanco::esperar_recogida(){
   if(Mostrador!=-1)
      fumador_espera[Mostrador].signal();
}

void Estanco::obtener_ingrediente(const int num_fumador){

   if(Mostrador==-1 || num_fumador!=Mostrador){
      fumador_espera[num_fumador].wait();
      
   }else{
   
      estanquero.signal();
      Mostrador=-1;
   }

}



class Suministradora : public HoareMonitor{
   private:
      static const int  tam_vec=5;
      int buffer[tam_vec]={-1};                    // -1 simbolo de que no hay objeto
      int primera_libre, primera_ocupada, contador;
      CondVar puedo_añadir, puedo_leer;

   public:
   Suministradora();
   void añadirIngrediente(const int ingrediente);
   int Leeringrediente();
};
Suministradora::Suministradora(){

   primera_libre=primera_ocupada=contador=0;
   puedo_añadir=newCondVar();
   puedo_leer=newCondVar();

}

void Suministradora::añadirIngrediente(const int ingrediente){

   if(contador>=tam_vec){
      cout<<"Linea 96"<<endl;
      puedo_añadir.wait();
      if(!puedo_leer.empty())puedo_leer.signal();
   }
      cout<<"Se añade el ingrediente "<<ingrediente<<" al buffer"<<endl<<endl;
      buffer[primera_libre]=ingrediente;
      primera_libre++;
      primera_libre=primera_libre%tam_vec;
      contador++;
      puedo_leer.signal();
      
      
   
   
}

int Suministradora::Leeringrediente(){
   if((contador<=0)|| (contador==0 && primera_ocupada==0)){
      puedo_leer.wait();
      if(!puedo_añadir.empty()) puedo_añadir.signal();
   }
   
      int ingrediente = buffer[primera_ocupada];
      primera_ocupada++;
      primera_ocupada=primera_ocupada%tam_vec;
      contador--;
      puedo_añadir.signal();
      return ingrediente;
   
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

void funcion_hebra_estanquero( MRef<Estanco> monitor, MRef<Suministradora> suministro )
{
   while (true){
   int ingrediente=suministro->Leeringrediente();
   cout<<"Ingrediente para darselo al estanquero: "<<ingrediente<<endl;
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

void funcion_hebra_suministradora(MRef<Suministradora> monitor){
   while (true)
   {
      const int ingrediente=producir_ingrediente();
      monitor->añadirIngrediente(ingrediente);
   }
   

}

//----------------------------------------------------------------------

int main()
{
   MRef<Estanco> monitor=Create<Estanco>();
   MRef<Suministradora> suministro=Create<Suministradora>();
   thread Heb_suministradora= thread(funcion_hebra_suministradora, suministro);
   thread hebra_fumadores[num_fumadores];
   thread Estanquero= thread(funcion_hebra_estanquero, monitor, suministro);
   for(int i=0; i<num_fumadores; i++){
      hebra_fumadores[i]=thread(funcion_hebra_fumador,i, monitor);
   }

   for (int i=0; i<num_fumadores; i++){
      hebra_fumadores[i].join();
   }
   Estanquero.join();
}
