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
const int num_ingredientes=3;
mutex mx;

class Estanco : public HoareMonitor{
   private:
      int Mostrador;
      bool cuenta;
      int contadora_cigarros0;
     // int producidos;
      int recogidos_mostrador;
      int fumado_por_0, fumado_por_3;
      CondVar estanquero, fumador_espera[num_fumadores], heb_contadora;


   public:
      Estanco();
      void poner_ingrediente(const int ingrediente);
      void esperar_recogida();
      void obtener_ingrediente(const int num_fumador);
      void cuenta_cigarros();
      void cuenta_cigarro_0();

};

Estanco::Estanco(){
   Mostrador=-1;
   cuenta=false;
   contadora_cigarros0=0;
   fumado_por_0=0;
   fumado_por_3=0;
  // producidos=0;
   recogidos_mostrador=0;
   estanquero= newCondVar();
   heb_contadora=newCondVar();

   for(int i=0; i<num_fumadores; i++){
      fumador_espera[i]=newCondVar();
   }

}

void Estanco::cuenta_cigarro_0(){
   contadora_cigarros0++;
}

void Estanco::poner_ingrediente(const int ingrediente){
   
   while (Mostrador != -1) estanquero.wait();

   Mostrador=ingrediente;  
   
   if (ingrediente==0) cuenta_cigarro_0();

   if(ingrediente==0 && contadora_cigarros0%15==0 && contadora_cigarros0!=0) {
         cuenta=true;
         heb_contadora.signal();
   }
    //producidos++;  

}


void Estanco::esperar_recogida(){

   if(Mostrador!=-1){

      if(Mostrador==0){

         int aleatorio= rand()%2==0? 0:3;
         (aleatorio==0)? fumado_por_0++:fumado_por_3++;
         cout<<"LE TOCA FUMAR AL FUMADOR: "<<aleatorio<<endl;
         fumador_espera[aleatorio].signal();

      }else{
         fumador_espera[Mostrador].signal();
      }
   }
   
}

void Estanco::obtener_ingrediente(const int num_fumador){

   if(Mostrador!=-1 ){
      if((num_fumador==0 || num_fumador==3) && Mostrador!=0)
         fumador_espera[num_fumador].wait();

      if((num_fumador!=0 && num_fumador!=3) && num_fumador!=Mostrador)
         fumador_espera[num_fumador].wait();
   
      if (Mostrador==0){
         recogidos_mostrador++;
         cout<<"\n\n Se han recogido del mostrador el ingrendiente '0': "<<recogidos_mostrador << " veces"<<endl<<endl;
      }
      
   }
   Mostrador=-1;
   estanquero.signal();

}

void Estanco::cuenta_cigarros(){

   if (!cuenta){
      heb_contadora.wait();
      //estanquero.signal();

   }else{
      
      cout<<"\n\n EL FUMADOR 0 SE HA FUMADO: "<<fumado_por_0<<endl
      <<" EL FUMADOR 3 SE HA FUMADO: "<<fumado_por_3<<endl
      //<<" SE HAN PRODUCIDO: "<<producidos<<endl
      <<" SE HA CONTADO EL PRODUCTO '0' : "<<contadora_cigarros0<<endl<<endl;
      cuenta=false;
      //estanquero.signal();
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
   // cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   mx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_ingredientes-1>() ;
   mx.lock();
   // informa de que ha terminado de producir
   //cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
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
  // cout<<"Se ha puesto en el mostrador el ingrediente: "<<ingrediente<<endl;
   mx.unlock();
   monitor->esperar_recogida();
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<5,20>() );

   // informa de que comienza a fumar
   mx.lock();
   //cout << "Fumador " << num_fumador << "  :"<< " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
   mx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );
   // informa de que ha terminado de fumar
   mx.lock();
   //cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
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
void funcion_hebra_contadora(MRef<Estanco> monitor){
   while(true){
      monitor->cuenta_cigarros();
   }

}

//----------------------------------------------------------------------

int main()
{
   MRef<Estanco> monitor=Create<Estanco>();
   thread hebra_fumadores[num_fumadores];
   thread Estanquero= thread(funcion_hebra_estanquero, monitor);
   thread Contadora= thread(funcion_hebra_contadora, monitor);

   for(int i=0; i<num_fumadores; i++){

      hebra_fumadores[i]=thread(funcion_hebra_fumador,i, monitor);


   }

   for (int i=0; i<num_fumadores; i++){
      hebra_fumadores[i].join();
   }
   Estanquero.join();
}
