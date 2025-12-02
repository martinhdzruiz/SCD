#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ,   // tamaño del buffer

   hebras_productoras=4,
   hebras_consumidoras=4,
   producir_por_hebra=num_items/hebras_productoras,
   consumir_por_hebra=num_items/hebras_consumidoras;
   

unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ,  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
   datos_hebra[hebras_productoras]={0};
   


class ProdCons : public HoareMonitor{

   private:

   int buffer[tam_vec]={0};
   int primera_libre, primera_ocupada, posicion;
   CondVar escribir, leer;

   public:

      ProdCons();
      void escribir_dato_LIFO(const int dato);
      int leer_dato_LIFO();
      void puedo_leer();
};

ProdCons::ProdCons(){
   escribir=newCondVar();
   leer=newCondVar();
   primera_libre, primera_ocupada, posicion =0;
}

void ProdCons::escribir_dato_LIFO(const int dato){

   if(posicion<tam_vec){
      buffer[posicion]=dato;
      posicion++;
      leer.signal();
   }else{
      escribir.wait();
   }

}
void ProdCons::puedo_leer(){
   if(posicion<=0) leer.wait();
}

int ProdCons::leer_dato_LIFO(){
   int devolver;

      devolver=buffer[--posicion];
      escribir.signal();
  
   return devolver;
}
//**********************************************************************
// funciones comunes a las dos soluciones  lifo)
//----------------------------------------------------------------------

unsigned producir_dato(const int hebra)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = datos_hebra[hebra] + hebra*producir_por_hebra;
   datos_hebra[hebra]++;
   cont_prod[dato_producido] ++ ;
   cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora( const int hebra ,MRef<ProdCons> monitor )
{
   for( unsigned i = 0 ; i < producir_por_hebra ; i++ )
   {
      int dato = producir_dato(hebra) ;
      monitor->escribir_dato_LIFO(dato);
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( const int hebra, MRef<ProdCons>monitor )
{
   for( unsigned i = 0 ; i < consumir_por_hebra ; i++ )
   {
      int dato ;
      monitor->puedo_leer();
      dato=monitor->leer_dato_LIFO();
      consumir_dato( dato ) ;
   }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
      << "Problema de los productores-consumidores  LIFO ." << endl
      << "------------------------------------------------------------------" << endl
      << flush ;
   MRef<ProdCons> monitor= Create<ProdCons>();
   thread productoras[hebras_productoras];
   thread consumidoras[hebras_consumidoras];

   for (int i=0; i<hebras_productoras; i++){
      productoras[i]=thread( funcion_hebra_productora, i, monitor);
      //cout<<" Hebra produtora"<< i<< endl;
      
   }
   for( int i=0; i<hebras_consumidoras; i++){
      consumidoras[i]= thread(funcion_hebra_consumidora, i, monitor);
      //cout<<" Hebra consumidora"<< i<< endl;
   }

   for(int i=0; i<hebras_productoras; i++){
      productoras[i].join();
      
   }

   for(int i=0; i<hebras_consumidoras; i++){
      
      consumidoras[i].join();
      
   }

   test_contadores();
}
