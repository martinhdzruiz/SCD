#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"
using namespace std ;
using namespace scd ;

const int num_fumadores = 3 ;

class Estanco : public HoareMonitor{
    private:

        CondVar estanquero, fumadores[num_fumadores];
        bool mostrador_vacio;
        int ingrediente;

    public:
        Estanco();
        void obtenerIngrediente(const int i);
        void ponerIngrediente(const int i);
        void esperarRecogidaIngrediente();
} ;
//--------------------------------------------------------------------------

Estanco::Estanco( ) {
    mostrador_vacio = true;
    ingrediente = -1;                                   // Se inicializa a -1 porq los ingredientes son: 0, 1 y 2; tmb puede servir como variable
                                                        // para inidcar cierre el estanco
    estanquero = newCondVar();
    for(int i = 0; i < num_fumadores; i++){
        fumadores[i] = newCondVar();
    }
}

void Estanco::obtenerIngrediente(const int i){
    if(mostrador_vacio || ingrediente != i){
        fumadores[i].wait();
    }
    mostrador_vacio = true;
    ingrediente = -1;
    estanquero.signal();
}

void Estanco::ponerIngrediente(const int i){
    if(!mostrador_vacio){
        estanquero.wait();
    }
    ingrediente = i;
    mostrador_vacio = false;

    cout << "Puesto ingrediente " << ingrediente << " en el mostrador." <<"\n";
    fumadores[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente(){
    if(!mostrador_vacio){
        estanquero.wait();
    }
}

int producir_ingrediente(){
    // calcular milisegundos aleatorios de duración de la acción defumar
    chrono::milliseconds duracion_produ( aleatorio<10,100>() );
    // informa de que comienza a producir
    cout << "Estanquero : empieza a producir ingrediente( " <<duracion_produ.count() << " milisegundos)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for( duracion_produ );
    const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;
    // informa de que ha terminado de producir
    cout << "Estanquero : termina de producir ingrediente"<<num_ingrediente << endl;


    return num_ingrediente ;
}

void funcion_hebra_estanquero(MRef<Estanco> estanco){
    while( true ){
    
        int ingrediente = producir_ingrediente();
        estanco->ponerIngrediente(ingrediente);
        estanco->esperarRecogidaIngrediente();
    }
}
void fumar( int num_fumador ){
    // calcular milisegundos aleatorios de duración de la acción defumar
    chrono::milliseconds duracion_fumar( aleatorio<20,200>() );
    // informa de que comienza a fumar
    cout << "Fumador " << num_fumador << " :" << " empieza a fumar (" << duracion_fumar.count() << "milisegundos)" << endl;
    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for( duracion_fumar );
    // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador <<": termina de fumar, comienza espera de ingrediente." << endl;

}
void funcion_hebra_fumador(MRef<Estanco> estanco, int num_fumador ){
    while( true ) {
        estanco->obtenerIngrediente(num_fumador);
        cout << "Ingrediente " << num_fumador << " recogido del mostrador.";
        fumar(num_fumador);
    }
}

int main() {
    MRef<Estanco> estanco = Create<Estanco>() ;
    thread f[num_fumadores] , e(funcion_hebra_estanquero,estanco);
    for(int i = 0; i < num_fumadores; i++){
        f[i] = thread(funcion_hebra_fumador, estanco, i);
    }
//no hace falta porque son bucles infinitos, pero lo dejo reflejado por si fuera necesario en otro caso
    e.join();
    for(int i = 0; i < num_fumadores; i++){
        f[i].join();
    }
}
