#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_lectores=5;
const int num_escritores=20;

class Lec_Esc : public HoareMonitor{

    private:
        int n_lec;                      // Número de lectores leyendo
        bool escrib;                    // Si hay algún escritor escribiendo
        CondVar lectura,                // No hay  escritura. Escribiendo, lectura posible
        escritura;                      // No hay lect. ni escrit. escritura posible

    public:
        Lec_Esc();
        void ini_lectura();
        void fin_lectura();
        void ini_escritura();
        void fin_escritura();

};

Lec_Esc::Lec_Esc(){
    n_lec=0;
    escrib=false;
    lectura=newCondVar();
    escritura=newCondVar();

}

void Lec_Esc::ini_lectura(){

    if(escrib) lectura.wait();

    n_lec=n_lec++;
    lectura.signal();
}
void Lec_Esc::fin_lectura(){
    n_lec= n_lec--;
    if (n_lec==0) escritura.signal();
}

void Lec_Esc::ini_escritura(){

    if (n_lec>0 || escrib){
        escritura.wait();
    }
    escrib=true;
}

void Lec_Esc::fin_escritura(){
    escrib=false;
    if(!lectura.empty()){
        lectura.signal();
    }else escritura.signal();
}

void leer(const int lec){
    cout<< " Lector " << lec<< " ha comemenzado a leer"<<endl;
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

}
void escribir(const int lec){
    cout<< " Escritor " << lec<< " ha comemenzado a escribir"<<endl;
    this_thread::sleep_for( chrono::milliseconds( aleatorio<60,200>() ));

}

void Lector(MRef<Lec_Esc>monitor, const int lec){
    while (true){
        monitor->ini_escritura();
        leer(lec);
        monitor->fin_lectura();
        cout<< " Lector " << lec<< " ha terminado de leer"<<endl;
        this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
    }

}

void Escritor(MRef<Lec_Esc> monitor, const int esc){
    while (true)
    {
        monitor->ini_escritura();
        escribir(esc);
        monitor->fin_escritura();
        cout<< " Escritor " << esc<< " ha terminado de escribir"<<endl;
        this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
    }
    
}

int main(){


    MRef<Lec_Esc> LyE = Create<Lec_Esc>();
    thread Lectores[num_lectores];
    thread Escritores[num_escritores];

    for(int i=0; i<num_escritores; i++){
        Escritores[i]=thread(Escritor, LyE, i);
    }

    for(int i=0; i<num_lectores; i++){
        Lectores[i]=thread(Lector, LyE, i);
    }

    for(int i=0; i<num_lectores; i++){
        Lectores[i].join();
    }

    for(int i=0; i<num_escritores; i++){
        Escritores[i].join();
    }

}