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
   num_items = 40,   // número de items
    tam_vec   = 10 ;   // tamaño del buffer


unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//                            MODIFCICACIONES                                         //
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


// VARIABLES
const unsigned num_hebras_productoras=5;  //numerpo de hebras prodductoras
const unsigned num_hebras_consumidoras=5; //número de hebras consumidoras
int primera_libre=0;
int primera_ocupada=0;
int rp=(num_items/num_hebras_productoras)-1;
int rc=num_items/num_hebras_consumidoras;
bool esta_el_dato=false;


//BUFFER DE ALMACENAMIENTO

int vec_buffer[tam_vec];
int array_matriz[num_hebras_productoras][tam_vec]={0};

// SEMÁFOROS
Semaphore pos_libres=tam_vec;
Semaphore pos_ocupadas=0;

// CERROJOS
mutex mtx;





//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------




unsigned producir_dato(int hebra)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
    unsigned dato_producido = siguiente_dato + (rp*hebra);
    siguiente_dato==rp? siguiente_dato=0:siguiente_dato++ ;
    
    if (cont_prod[dato_producido]==0){
        cont_prod[dato_producido] ++ ;
        cout << "producido: " << dato_producido << endl << flush ;
        esta_el_dato=false;
        
        
    }else{
        esta_el_dato=true;
        
    }

    return dato_producido ;
            
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )

{
  // cout<<endl<<"                   DATO QUE SE VA A CONSUMIR:"<<dato<<"                        "<<endl;
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


//**********************************************************************//
// ------------------FUNCIONES DE LAS HEBRAS----------------------------//
//**********************************************************************//

void  funcion_hebra_productora (int hebra )
{
   for( unsigned i = 0 ; i < num_items; i++ )
   {
   
      sem_wait(pos_libres);
      

//--------SECCIÓN CRÍTICA-------------------//
    
      mtx.lock();
       
      int dato = producir_dato(hebra) ;
       
       if (esta_el_dato==false)
       {
           vec_buffer[primera_libre]=dato;
           if(primera_libre==(tam_vec-1))
           {
               primera_libre=0;
           }
           else primera_libre++;
           
         sem_signal(pos_ocupadas);
           
       }
       else sem_signal(pos_libres);
    
      mtx.unlock();
//--------SECCIÓN CRÍTICA-------------------//

      
   }
}
//----------------------------------------------------------------------

void funcion_hebra_consumidora(  int hebra)
{
   for( unsigned i = 0 ; i < num_items; i++ )
   {
       
      int dato ;
      sem_wait(pos_ocupadas);
      mtx.lock();
      dato=vec_buffer[primera_ocupada];

      if(primera_ocupada==tam_vec-1)
      {
         primera_ocupada=0;
      }
      else primera_ocupada++;

      consumir_dato( dato ) ;
    
      mtx.unlock();
      
   sem_signal(pos_libres);
       cout<<"salgo de la secion crítica y aumento pos_libres"<<endl;
      // completar ......
      
    }
}
//----------------------------------------------------------------------

int main()
{
    cout << "-----------------------------------------------------------------------------" << endl
        << "PRIMERA PARTE :Problema de los productores-consumidores (solución FIFO ?)." << endl
        << "-----------------------------------------------------------------------------" << endl
        << flush ;

        thread array_de_hebras_productoras[num_hebras_productoras];
        thread array_de_hebras_consumidoras[num_hebras_consumidoras];

        for (int i=0;  i<num_hebras_consumidoras; i++
        ){

         array_de_hebras_productoras[i]=thread(funcion_hebra_productora, i);

         array_de_hebras_consumidoras[i]=thread(funcion_hebra_consumidora, i);
        }

        

        for (int i=0,  j=0; i<num_hebras_productoras|| j<num_hebras_consumidoras; i++, j++){
         array_de_hebras_productoras[i].join();
          array_de_hebras_consumidoras[j].join();
        }

   

test_contadores();


}
