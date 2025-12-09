// imagina al haber acabado, primero acaben los consumidores, lugeo los productores y luego el buffer

#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <mpi.h>
#include <vector>
#include <algorithm>
using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int longitud = 1,
          // Número de productores
    np = 4,
          nc = 5,
          // Número de consumidores
    num_procesos_esperado = np + nc + 2, // Total de procesos
    num_items = 40,
          // Número total de ítems (múltiplo de np y nc)
    tam_vector = 10,
          // Tamaño del buffer
    id_buffer = np,
          // ID del buffer
    etiq_prod = 0,
          // Etiqueta para productores
    etiq_cons = 1,
          // Etiqueta para consumidores
    id_verificador = 10;
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

// --------------------------------------------------

void funcion_productor(int orden)
{
    for (int i = 0; i < num_items / np; i++)
    {
        int valor_prod = producir(orden, i);
        MPI_Ssend(&valor_prod, longitud, MPI_INT,
                  id_buffer, etiq_prod, MPI_COMM_WORLD);
        cout << "Productor " << orden << " envía " << valor_prod << endl;
    }
}

// --------------------------------------------------

void consumir(int valor_cons, int orden)
{
    sleep_for(milliseconds(aleatorio<110, 200>()));
    cout << "Consumidor " << orden << " ha consumido " << valor_cons << endl;
}

// --------------------------------------------------

void funcion_consumidor(int orden)
{
    int peticion = 1, valor_rec;
    MPI_Status estado;

    for (int i = 0; i < num_items / nc; i++)
    {
        MPI_Ssend(&peticion, longitud, MPI_INT,
                  id_buffer, etiq_cons, MPI_COMM_WORLD);

        MPI_Recv(&valor_rec, longitud, MPI_INT,
                 id_buffer, etiq_cons, MPI_COMM_WORLD, &estado);

        consumir(valor_rec, orden);
    }
}

// --------------------------------------------------

void funcion_verificadora()
{
    int recibido;
    MPI_Status estado;
    vector<int> v;

    for (int i = 0; i < num_items; i++)
    {
        MPI_Recv(&recibido, 1, MPI_INT,
                 id_buffer, etiq_cons, MPI_COMM_WORLD, &estado);
        v.push_back(recibido);
    }

    sort(v.begin(), v.end());

    bool ok = true;
    for (int i = 0; i < num_items; i++)
        if (v[i] != i)
            ok = false;

    if (ok)
        cout << "\nVERIFICACIÓN CORRECTA\n";
    else
        cout << "\nVERIFICACIÓN INCORRECTA\n";
}

// --------------------------------------------------

void funcion_buffer()
{
    int buffer[tam_vector];
    int primera_libre = 0;
    int primera_ocupada = 0;
    int num_celdas_ocupadas = 0;

    int valor;
    int num_total_extraidos = 0;

    MPI_Status estado;

    while (num_total_extraidos < num_items)
    {
        int etiq_aceptable;

        if (num_celdas_ocupadas == 0)
            etiq_aceptable = etiq_prod;
        else if (num_celdas_ocupadas == tam_vector)
            etiq_aceptable = etiq_cons;
        else
            etiq_aceptable = MPI_ANY_TAG;

        MPI_Recv(&valor, 1, MPI_INT,
                 MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);

        switch (estado.MPI_TAG)
        {
        case etiq_prod:
            buffer[primera_libre] = valor;
            primera_libre = (primera_libre + 1) % tam_vector;
            num_celdas_ocupadas++;
            cout << "Buffer recibe " << valor << endl;
            break;

        case etiq_cons:
        {
            int dato = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada + 1) % tam_vector;
            num_celdas_ocupadas--;
            num_total_extraidos++;

            cout << "Buffer envía " << dato << endl;

            MPI_Ssend(&dato, 1, MPI_INT,
                      estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD);

            MPI_Send(&dato, 1, MPI_INT,
                     id_verificador, etiq_cons, MPI_COMM_WORLD);
            break;
        }
        }
    }

    cout << "\nBUFFER TERMINA\n";
}

// --------------------------------------------------

int main(int argc, char *argv[])
{
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos_actual == num_procesos_esperado)
    {
        if (id_propio >= 0 && id_propio < np)
            funcion_productor(id_propio);

        else if (id_propio == id_buffer)
            funcion_buffer();

        else if (id_propio > id_buffer && id_propio < id_verificador)
            funcion_consumidor(id_propio - (np + 1));

        else
            funcion_verificadora();
    }
    else if (id_propio == 0)
    {
        cout << "Se esperaban " << num_procesos_esperado
             << " procesos y hay " << num_procesos_actual << endl;
    }

    MPI_Finalize();
    return 0;
}