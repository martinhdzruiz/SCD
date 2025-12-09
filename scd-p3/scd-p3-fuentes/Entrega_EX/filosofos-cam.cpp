// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI

// -----------------------------------------------------------------------------
// Fecha de creación: 30 Noviembre 2025
//
// Compilación: mpicxx -std=c++11 -o filo_interb_exe filosofos-interb.cpp
// Ejcución: mpirun -oversubscribe -np 10 ./filo_interb_exe
// -----------------------------------------------------------------------------
#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>
using namespace std;
using namespace std::this_thread;
using namespace std::chrono;
const int
    num_filosofos = 5,                // número de filósofos
    num_filo_ten = 2 * num_filosofos, // número de filósofos y tenedores
    num_procesos = num_filo_ten + 1,      // número de procesos total (por ahora solo hay filo y ten)
    camarero = num_procesos - 1,      // id del camarero,
    TAG_Sentarse=8,
    TAG_Levantarse=9;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------
template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}
// ---------------------------------------------------------------------
void funcion_filosofos(int id)
{
    int id_ten_izq = (id + 1) % num_filo_ten,                // id. tenedor izq.
        id_ten_der = (id + num_filo_ten - 1) % num_filo_ten; // id. tenedor der.
    int peticion = 93, liberacion = 26;
    while (true)
    {
        
        MPI_Ssend(&peticion, 1, MPI_INT, camarero, TAG_Sentarse, MPI_COMM_WORLD); // Solicita permiso al camarero para sentarse
        
        cout << endl<< "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
        // ... solicitar tenedor izquierdo (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
        cout << endl<< "Filósofo " << id << " solicita ten. der." << id_ten_der << endl;
        // ... solicitar tenedor derecho (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

        cout << endl<< "Filósofo " << id << " comienza a comer" << endl;

        sleep_for(milliseconds(aleatorio<10, 100>()));

        cout << endl<< "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl;
        
        // ... soltar el tenedor izquierdo (completar)
        MPI_Ssend(&liberacion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
        cout << endl<< "Filósofo " << id << " suelta ten. der. " << id_ten_der << endl;

        // ... soltar el tenedor derecho (completar)
        MPI_Ssend(&liberacion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

        cout << endl<< "Filosofo " << id << " comienza a pensar" << endl;
        MPI_Send(&peticion, 1, MPI_INT, camarero, TAG_Levantarse, MPI_COMM_WORLD); // Notifica al camarero que se levanta

        sleep_for(milliseconds(aleatorio<10, 100>()));

        
    }
}

void funcion_camarero(int id){

    int sentados=0;
    int valor, id_filosofo, etiqueta, id_etiqueta;
    MPI_Status estado;

    while(true){

        (sentados< num_filosofos -1) ? etiqueta=MPI_ANY_TAG : etiqueta=TAG_Levantarse;

        //Recibir peticion con la etiqueta adecuada
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta, MPI_COMM_WORLD, &estado);
        cout << endl<< "Camarero recibe peticion de filo. " << estado.MPI_SOURCE << " con etiqueta " << estado.MPI_TAG << endl;
        //Guardar id del filosofo y la etiqueta
        id_filosofo= estado.MPI_SOURCE;
        id_etiqueta= estado.MPI_TAG;

        switch (id_etiqueta)
        {
        case TAG_Sentarse:
            sentados++;
            cout << endl<< "Camarero permite sentarse a filo. " << id_filosofo << " . Total sentados: " << sentados << endl;
            break;
        
        case TAG_Levantarse:
            sentados--;
            cout << endl<< "Camarero permite levantarse a filo. " << id_filosofo << " . Total sentados: " << sentados << endl;
            break;
        }
    }

}
// ---------------------------------------------------------------------
void funcion_tenedores(int id)
{
    int valor, id_filosofo; // valor recibido, identificador del filósofo
    MPI_Status estado;      // metadatos de las dos recepciones
    while (true)
    {
        // ...... recibir petición de cualquier filósofo (completar)
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
        // ...... guardar en 'id_filosofo' el id. del emisor (completar)
        id_filosofo = estado.MPI_SOURCE;
        cout << endl << "Ten. " << id << " ha sido cogido por filo. " << id_filosofo << endl;
        // ...... recibir liberación de filósofo 'id_filosofo' (completar)
        MPI_Recv(&valor, 1, MPI_INT, id_filosofo, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
        cout << endl<< "Ten. " << id << " ha sido liberado por filo. " << id_filosofo << endl;
    }
}
// ---------------------------------------------------------------------
int main(int argc, char **argv)
{
    int id_propio, num_procesos_actual;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);
    cout << "Num procesos actual: " << num_procesos_actual << endl;
    cout << "Num procesos esperado: " << num_procesos << endl;
    if (num_procesos == num_procesos_actual)
    {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio == camarero){
            funcion_camarero(id_propio);
        }
        
        if (id_propio % 2 == 0)           // si es par
            funcion_filosofos(id_propio); // es un filósofo
        else                              // si es impar
            funcion_tenedores(id_propio); // es un tenedor
    
    }
    else
    {
        if (id_propio == 0) // solo el primero escribe error, indep. del rol
        {
            cout << "el número de procesos esperados es: " << num_procesos << endl
                 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
                 << "(programa abortado)" << endl;
        }
    }
    MPI_Finalize();
    return 0;
}