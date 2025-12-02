#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std;
using namespace scd;

//**********************************************************************
// Variables globales

const unsigned
   num_items = 80, // número de items
   tam_vec = 5,   // tamaño del buffer
   
   num_hebras_productoras=4,
   num_hebras_consumidoras=5;


unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato = 0,         // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
   bufer[tam_vec],

   producir_por_hebra=num_items/num_hebras_productoras,
   consumir_por_hebra=num_items/num_hebras_consumidoras,

   num_producidos=0,
   num_consumidos=0,
   recopiladora=2,

   Posicion = 0,
   primera_libre = 0,
   primera_ocupada = 0,
   items_producidos[num_hebras_productoras]={0};

Semaphore
   puede_producir=10,
   puede_consumir=0,
   turno_productora=1,
   turno_consumidora=1,
   turno_recopiladora=0,
   anotar_prod=1,
   anotar_cons=1;


//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------
unsigned producir_dato(int hebra)
{
   this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
   const unsigned dato_producido = items_producidos[hebra]+hebra*producir_por_hebra;
   items_producidos[hebra]++;
   cont_prod[dato_producido]++;
   cout << "producido: " << dato_producido << endl
   << flush;

      anotar_prod.sem_wait();
      num_producidos++;
      anotar_prod.sem_signal();
   return dato_producido;
}
//----------------------------------------------------------------------

void consumir_dato(unsigned dato, int hebra)
{
   assert(dato < num_items);
   cont_cons[dato]++;
   this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

   cout << "                  consumido: " << dato << endl;

   anotar_cons.sem_wait();
      num_consumidos++;
   anotar_cons.sem_signal();
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true;
   cout << "comprobando contadores ....";
   for (unsigned i = 0; i < num_items; i++)
   {
      if (cont_prod[i] != 1)
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl;
         ok = false;
      }
      if (cont_cons[i] != 1)
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl;
         ok = false;
      }
   }
   if (ok)
      cout << endl
         << flush << "solución (aparentemente) correcta." << endl
         << flush;
}

/*
//###################################################################//
//                         VERSION LIFO                              //
//###################################################################//
void funcion_hebra_productora(const int hebra)
{
   
   for (unsigned i = 0; i < producir_por_hebra; i++)
   {
      
      puede_producir.sem_wait();
      turno_productora.sem_wait();
      int dato = producir_dato(hebra);
      bufer[Posicion] = dato;
      Posicion++;
      turno_productora.sem_signal();
      puede_consumir.sem_signal();
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(const int hebra)
{
   for (unsigned i = 0; i < consumir_por_hebra; i++)
   {
      sem_wait(turno_consumidora);
      int dato;
      sem_wait( puede_consumir);
      Posicion--;
      dato = bufer[Posicion];
      
      puede_producir.sem_signal();
      
      turno_consumidora.sem_signal();
      consumir_dato(dato, hebra);
   }
}
//----------------------------------------------------------------------


*/

// ###################################################################//
//                          VERSION FIFO                              //
// ###################################################################//
void funcion_hebra_productora(const int hebra)
{
   for (unsigned i = 0; i < producir_por_hebra; i++)
   {
      int dato = producir_dato(hebra);
      
      puede_producir.sem_wait();
      turno_productora.sem_wait();

      bufer[primera_libre] = dato;
      primera_libre = (primera_libre + 1) % tam_vec;
      
      puede_consumir.sem_signal();

      if(num_producidos==10 || num_producidos==25) turno_recopiladora.sem_signal();

      turno_productora.sem_signal();


   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(const int hebra)
{
   for (unsigned i = 0; i < consumir_por_hebra; i++)
   {
      int dato;

      puede_consumir.sem_wait();
      turno_consumidora.sem_wait();
      dato = bufer[primera_ocupada];
      primera_ocupada = (primera_ocupada + 1) % tam_vec;
      puede_producir.sem_signal();
      turno_consumidora.sem_signal();
      
      consumir_dato(dato, hebra);
   }
}


void funcion_hebra_recopiladora(){

   while (recopiladora>0)
   { 
      turno_recopiladora.sem_wait();
      cout<<"\n\n SE HAN PRODUCIDOS: "<<num_producidos<< endl;
      cout<<" SE HAN CONSUMIDO: "<<num_consumidos<<endl<<endl;
      recopiladora--;
      
   }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
      << "Problema de los productores-consumidores (solución LIFO o FIFO ?)." << endl
      << "------------------------------------------------------------------" << endl
      << flush;

   thread hebras_productoras[num_hebras_productoras];
   thread hebras_consumidoras[num_hebras_consumidoras];
   thread hebra_recopiladora= thread(funcion_hebra_recopiladora);

   for (int i=0; i<num_hebras_productoras; i++){
      hebras_productoras[i]=thread( funcion_hebra_productora, i);
      //cout<<" Hebra produtora"<< i<< endl;
      
   }
   for( int i=0; i<num_hebras_consumidoras; i++){
      hebras_consumidoras[i]= thread(funcion_hebra_consumidora, i);
      //cout<<" Hebra consumidora"<< i<< endl;
   }

   for(int i=0; i<num_hebras_productoras; i++){
      hebras_productoras[i].join();
      
   }

   for(int i=0; i<num_hebras_consumidoras; i++){
      
      hebras_consumidoras[i].join();
      
   }

   test_contadores();
}
