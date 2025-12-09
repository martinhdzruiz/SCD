// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int
    num_productores   = 5,
    num_consumidores  = 4,

    id_prim_consumidor = 0,
    id_ult_consumidor  = id_prim_consumidor + num_consumidores - 1, // 3

    id_prim_productor  = id_ult_consumidor + 1,                     // 4
    id_ult_productor   = id_prim_productor + num_productores - 1,   // 8

    id_buffer          = id_ult_productor + 1,                      // 9
    id_verificador     = id_buffer + 1,                             // 10

    num_procesos_esperado = id_verificador + 1,                     // 11

    num_items  = 12 * num_productores,
    tam_vector = 5;

// Etiquetas de los mensajes
const int
    etiq_consumidor = 88,
    etiq_productor = 99,
    etiq_verificador = 77,
    etiq_cualquiera = MPI_ANY_TAG;

// Reparto de items entre productores y consumidores
const int
    items_productor = num_items / num_productores,
    items_consumidor = num_items / num_consumidores;

int contador_producidos = 0;
int num_total_añadidos=0;

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
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int etiqueta)
{
    int dato = etiqueta * items_productor + contador_producidos;
    contador_producidos++;
    sleep_for(milliseconds(aleatorio<10, 100>()));
    // cout << "Productor ha producido valor " << contador_producidos << endl << flush;
    return dato;
}
// ---------------------------------------------------------------------

void funcion_productor(int etiqueta)
{

    int inicio = etiqueta * items_productor;
    int fin = inicio + items_productor;
    for (int i = inicio; i < fin; i++)
    {
        // producir valor
        int valor_prod = producir(etiqueta);
        // enviar valor
        cout << "Productor va a enviar valor " << valor_prod << endl
             << flush;

        MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, etiq_productor, MPI_COMM_WORLD);

        // cout << "Id BUFFER: " << id_buffer << " Etiqueta " << etiqueta << endl << flush ;
    }
}
// ---------------------------------------------------------------------

void consumir(int valor_cons)
{
    // espera bloqueada
    sleep_for(milliseconds(aleatorio<110, 200>()));
    cout << "Consumidor ha consumido valor " << valor_cons << endl
         << flush;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int etiqueta)
{
    int peticion,
        valor_rec = 1;
    MPI_Status estado;

    int inicio = etiqueta * items_consumidor;
    int fin = inicio + items_consumidor;

    for (int i = inicio; i < fin; i++)
    {
        MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
        MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_cualquiera, MPI_COMM_WORLD, &estado); // Recibe de buffer, la etiqueta esa es correcta?
        // cout << "Consumidor ha recibido valor " << valor_rec  << "Por " << etiqueta << endl << flush ;
        consumir(valor_rec);
    }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
    int buffer[tam_vector],      // buffer con celdas ocupadas y vacías
        valor,                   // valor recibido o enviado
        primera_libre = 0,       // índice de primera celda libre
        primera_ocupada = 0,     // índice de primera celda ocupada
        num_celdas_ocupadas = 0, // número de celdas ocupadas
        num_extraidas=0,
        etiq_emisor_aceptable;   // identificador de emisor aceptable

    bool bloqueo=false;
        

    MPI_Status estado;           // metadatos del mensaje recibido

    while(num_extraidas < num_items){
    
        // 1. determinar si puede enviar solo prod., solo cons, o todos
        if(!bloqueo){
            if (num_celdas_ocupadas == 0)               // si buffer vacío
                etiq_emisor_aceptable = etiq_productor; // $~~~$ solo prod.

            else if (num_celdas_ocupadas == tam_vector)  // si buffer lleno
                etiq_emisor_aceptable = etiq_consumidor; // $~~~$ solo cons.

            else                                         // si no vacío ni lleno
                etiq_emisor_aceptable = etiq_cualquiera; // $~~~$ cualquiera
        }else{
            etiq_emisor_aceptable = etiq_productor;
            bloqueo=false;
        }
        
        if(num_total_añadidos==40){
            etiq_emisor_aceptable = etiq_consumidor;
            while (num_celdas_ocupadas>0){
                 // 2. recibir un mensaje del emisor o emisores aceptables según su etiqueta
                MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado);
                valor = buffer[primera_ocupada];
                primera_ocupada = (primera_ocupada + 1) % tam_vector;
                num_celdas_ocupadas--;
                cout << "Buffer va a enviar valor " << valor << endl;
                MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD); // Poruqe estado.MPI_SOURCE? y porque etiqueta 0?
                num_extraidas++;
            }
            etiq_emisor_aceptable = etiq_productor;
        }

        

        // 2. recibir un mensaje del emisor o emisores aceptables según su etiqueta
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado);

        // 3. procesar el mensaje recibido
        //cout<<"MPI_SOuRCE: "<<estado.MPI_SOURCE<<" ETIQUETA: "<<estado.MPI_TAG<<endl;
        
        switch (estado.MPI_TAG) // leer emisor del mensaje en metadatos
        {
        case etiq_productor: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor;
            primera_libre = (primera_libre + 1) % tam_vector;
            num_celdas_ocupadas++;
            num_total_añadidos++;
            if(num_total_añadidos==20 || num_total_añadidos==50){
                cout<<"\n \n -------------Buffer: Enviando al verificador que hay "<<num_celdas_ocupadas<<" elementos en el buffer---------------- \n"<<endl;
                //MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_verificador, MPI_COMM_WORLD, &estado);                     //pedir el numero de elementos al buffer
                MPI_Ssend(&num_celdas_ocupadas, 1, MPI_INT, id_verificador, etiq_verificador,MPI_COMM_WORLD);
                
            }
            cout << "Buffer ha recibido valor " << valor << endl;
            cout<<"----------- AÑADIDOS POR EL MOMENTO:" <<num_total_añadidos<<"-------------"<<endl;
            break;

        case etiq_consumidor: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada + 1) % tam_vector;
            num_celdas_ocupadas--;
            num_extraidas++;
            if(num_total_añadidos<30 && estado.MPI_SOURCE==2){
                bloqueo=true;
            }
            cout << "Buffer va a enviar valor " << valor << endl;
            MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD); // Poruqe estado.MPI_SOURCE? y porque etiqueta 0?
            break;
        }
    }
}

void funcion_verificador(int id_verificador){
    int num_iteraciones=2;
    int num_celdas_ocupadas;
    MPI_Status estado;

    for(int i=0; i<num_iteraciones; i++){
        //MPI_Send(&num_celdas_ocupadas,1,MPI_INT,id_buffer,etiq_verificador,MPI_COMM_WORLD);                     //pedir el numero de elementos al buffer

        MPI_Recv(&num_celdas_ocupadas,1,MPI_INT,MPI_ANY_SOURCE,etiq_verificador,MPI_COMM_WORLD,&estado);
        cout<<"\n \n -------------Verificador: En la iteracion "<<i<<" hay "<<num_celdas_ocupadas<<" elementos en el buffer---------------- \n"<<endl;
        
    }
}
// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int id_propio, num_procesos_actual;

    // inicializar MPI, leer identif. de proceso y número de procesos
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos_esperado == num_procesos_actual)
{
    if (id_propio >= id_prim_productor && id_propio <= id_ult_productor)
        funcion_productor(id_propio - id_prim_productor); // índice lógico 0..4
    else if (id_propio == id_buffer)
        funcion_buffer();
    else if (id_propio == id_verificador)
        funcion_verificador(id_propio);
    else                         // 0..3
        funcion_consumidor(id_propio); // etiqueta 0..3
}
    else
    {
        if (id_propio == 0) // solo el primero escribe error, indep. del rol
        {
            cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
                 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
                 << "(programa abortado)" << endl;
        }
    }

    // al terminar el proceso, finalizar MPI
    MPI_Finalize();
    return 0;
}
