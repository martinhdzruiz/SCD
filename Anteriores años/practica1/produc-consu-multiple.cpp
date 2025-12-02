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
    num_items = 40, // número de items
    tam_vec = 10, // tamaño del buffer
    np = 5, // número de productores
    nc = 5; // número de consumidores
unsigned
    cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
    cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
    siguiente_dato = 0, // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
    primera_libre = 0,
    primera_ocupada = 0,
    p = num_items/np, //número de items que produce cada productor
    c = num_items/nc, // número de items que consume cada consumidor
    items_producidos[np] = {0},
    buff[tam_vec];
Semaphore libres = tam_vec;
Semaphore ocupadas = 0;
Semaphore turnoc=1;
Semaphore turnop=1;
//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------
unsigned producir_dato(int ip){
    assert(ip < np);
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
    const unsigned dato_producido = items_producidos[ip] + ip*p;
    items_producidos[ip]++;
    cont_prod[dato_producido]++;
    cout << "producido: " << dato_producido << "   por la hebra: "<<ip<<endl << flush ;
    
    return dato_producido;
}
void consumir_dato(int ic, unsigned dato){
    assert(dato<num_items);
    assert(ic<nc);
    cont_cons[dato]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
     cout << "                  consumido: " << dato << endl ;


}

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items; i++ )
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

void funcion_hebra_productora(int ih){
    for(unsigned i=0;i<p;i++){
        int dato=producir_dato(ih);
        sem_wait(libres);
        sem_wait(turnop);
        buff[primera_libre]=dato;
        if(primera_libre==(tam_vec-1)){
         primera_libre=0;
      }else primera_libre++;
        sem_signal(ocupadas);
        sem_signal(turnop);
    }

}
void funcion_hebra_consumidora(int ih){
    for(unsigned i =0;i<c;i++){
        int dato;
        sem_wait(ocupadas);
        sem_wait(turnoc);
        dato=buff[primera_ocupada];
        
        if(primera_ocupada==tam_vec-1){
         primera_ocupada=0;
      }else primera_ocupada++;
        
        sem_signal(libres);
        sem_signal(turnoc);

        consumir_dato(ih,dato);
    }
}

    int main(){
    cout << "-----------------------------------------------------------------------------" << endl
        << "PRIMERA PARTE :Problema de los productores-consumidores (solución FIFO ?)." << endl
        << "-----------------------------------------------------------------------------" << endl
        << flush ;

        thread array_de_hebras_productoras[np];
        thread array_de_hebras_consumidoras[nc];

        for (int i=0;  i<np; i++
        ){

         array_de_hebras_productoras[i]=thread(funcion_hebra_productora, i);

         
        }

        for (int i=0; i<nc;i++){
         array_de_hebras_consumidoras[i]=thread(funcion_hebra_consumidora, i);
        }


        

        for (int i=0; i<np; i++){
         array_de_hebras_productoras[i].join();
          
        }


        for (int j=0; j<nc; j++){
         array_de_hebras_consumidoras[j].join();
          
        }
        

   

test_contadores();


}
