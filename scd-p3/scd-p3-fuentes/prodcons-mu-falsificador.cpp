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
    // id_productor          = 0 ,
    // id_buffer             = 1 ,
    // id_consumidor         = 2 ,
    num_productores = 4,
    num_consumidores = 8,
    num_falsificadores = 2,
    num_procesos_esperado = num_consumidores + num_productores + num_falsificadores + 2,
    id_buffer = num_productores,
    id_buffer_falsificador = num_productores + num_consumidores+1,

    num_items = 80,
    PRODUCTO_FALSIFICADO = 44,
    num_falsificadores_items = 80,
    tam_vector = 3;

// Etiquetas de los mensajes
const int
    etiq_consumidor = 88,
    etiq_productor = 99,
    etiq_falsificador = 77,
    etiq_cualquiera = MPI_ANY_TAG;

// Reparto de items entre productores y consumidores
const int
    items_productor = num_items / num_productores,
    items_consumidor = num_items / num_consumidores;

int contador_producidos = 0;

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
    

    if (etiqueta == 0)
    {

        int inicio = etiqueta * items_consumidor;
        int fin = inicio + items_consumidor + num_falsificadores_items/num_falsificadores;
        bool turno = false;

        for (int i = inicio; i < fin; i++)
        {
            if (turno)
            {

                MPI_Ssend(&peticion, 1, MPI_INT, id_buffer_falsificador, etiq_consumidor, MPI_COMM_WORLD);

                MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer_falsificador, etiq_cualquiera, MPI_COMM_WORLD, &estado); // Recibe de buffer, la etiqueta esa es correcta?
                cout << "\n \n Consumidor ha recibido VALOR FALSIFICADO " << valor_rec  << "  Por "<< etiqueta << endl << flush ;
                consumir(valor_rec);

                turno = false;
            }
            else
            {

                MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
                MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_cualquiera, MPI_COMM_WORLD, &estado); // Recibe de buffer, la etiqueta esa es correcta?
                cout << "Consumidor ha recibido valor " << valor_rec  << "   Por "<< etiqueta << endl << flush ;
                turno = true;
                consumir(valor_rec);
            }
        }
    }
    else
    {

        int inicio =etiqueta * items_consumidor;
        int fin = inicio + items_consumidor;

        for (int i = inicio; i < fin; i++)
        {
            MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);

            MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_cualquiera, MPI_COMM_WORLD, &estado); // Recibe de buffer, la etiqueta esa es correcta?
            cout << "Consumidor ha recibido valor " << valor_rec  << "Por " << etiqueta << endl << flush ;
            consumir(valor_rec);
        }
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
        etiq_emisor_aceptable;   // identificador de emisor aceptable

    MPI_Status estado; // metadatos del mensaje recibido

    for (unsigned int i = 0; i < num_items * 2; i++)
    {
        // 1. determinar si puede enviar solo prod., solo cons, o todos

        if (num_celdas_ocupadas == 0)               // si buffer vacío
            etiq_emisor_aceptable = etiq_productor; // $~~~$ solo prod.

        else if (num_celdas_ocupadas == tam_vector)  // si buffer lleno
            etiq_emisor_aceptable = etiq_consumidor; // $~~~$ solo cons.

        else                                         // si no vacío ni lleno
            etiq_emisor_aceptable = etiq_cualquiera; // $~~~$ cualquiera

        // 2. recibir un mensaje del emisor o emisores aceptables según su etiqueta
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado);

        // 3. procesar el mensaje recibido

        switch (estado.MPI_TAG) // leer emisor del mensaje en metadatos
        {
        case etiq_productor: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor;
            primera_libre = (primera_libre + 1) % tam_vector;
            num_celdas_ocupadas++;
            cout << "Buffer ha recibido valor " << valor << endl;
            break;

        case etiq_consumidor: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada + 1) % tam_vector;
            num_celdas_ocupadas--;
            cout << "Buffer va a enviar valor " << valor << endl;
            MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_consumidor, MPI_COMM_WORLD); // Poruqe estado.MPI_SOURCE? y porque etiqueta 0?
            break;
        }
    }
}

void funcion_buffer_falsificador()
{

    int buffer[tam_vector],      // buffer con celdas ocupadas y vacías
        valor,                   // valor recibido o enviado
        primera_libre = 0,       // índice de primera celda libre
        primera_ocupada = 0,     // índice de primera celda ocupada
        num_celdas_ocupadas = 0, // número de celdas ocupadas
        etiq_emisor_aceptable;   // identificador de emisor aceptable

    MPI_Status estado; // metadatos del mensaje recibido

    for (unsigned int i = 0; i < num_items * 2; i++)
    {
        // 1. determinar si puede enviar solo prod., solo cons, o todos

        if (num_celdas_ocupadas == 0)                  // si buffer vacío
            etiq_emisor_aceptable = etiq_falsificador; // $~~~$ solo prod.

        else if (num_celdas_ocupadas == tam_vector)  // si buffer lleno
            etiq_emisor_aceptable = etiq_consumidor; // $~~~$ solo cons.

        else                                         // si no vacío ni lleno
            etiq_emisor_aceptable = etiq_cualquiera; // $~~~$ cualquiera

        // 2. recibir un mensaje del emisor o emisores aceptables según su etiqueta
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado); // debria ser id_buffer_falsificador

        // 3. procesar el mensaje recibido

        switch (estado.MPI_TAG) // leer emisor del mensaje en metadatos
        {
        case etiq_falsificador: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor;
            primera_libre = (primera_libre + 1) % tam_vector;
            num_celdas_ocupadas++;
            cout << "Buffer FALSIFICADOR  ha recibido valor " << valor << endl;
            break;

        case etiq_consumidor: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada + 1) % tam_vector;
            num_celdas_ocupadas--;
            cout << "Buffer FALSIFICADOR va a enviar valor " << valor << endl;
            MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_consumidor, MPI_COMM_WORLD); // Poruqe estado.MPI_SOURCE? y porque etiqueta 0?
            break;
        }
    }
}

void funcion_falsificador(int etiqueta)
{
    for (int i = 0; i < num_falsificadores_items/num_falsificadores; i++)
    {
        int valor_falsificado = PRODUCTO_FALSIFICADO;
        cout << "Falsificador va a enviar valor " << valor_falsificado << endl<< flush;

        MPI_Ssend(&valor_falsificado, 1, MPI_INT, id_buffer_falsificador, etiq_falsificador, MPI_COMM_WORLD);
        sleep_for(milliseconds(aleatorio<10, 100>()));
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
        // ejecutar la operación apropiada a 'id_propio'
        if (id_propio >= 0 && id_propio < num_productores)

            funcion_productor(id_propio);

        else if (id_propio == id_buffer)
            funcion_buffer();
        else if (id_propio > id_buffer && id_propio < num_productores + num_consumidores + 1)

            funcion_consumidor(id_propio - num_productores - 1);
        else if (id_propio == id_buffer_falsificador)
            funcion_buffer_falsificador();
        else

            funcion_falsificador(id_propio - num_productores - num_consumidores - 1);
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
