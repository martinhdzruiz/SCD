#include <mpi.h>
#include <thread>
#include <random>
#include <chrono>
#include <iostream>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int
    num_filosofos = 5,
    num_procesos = 2 * num_filosofos + 1, // filósofos + tenedores + camarero
    TAG_TOMAR = 0,
    TAG_DEJAR = 1,
    TAG_SENTARSE = 2,
    TAG_LEVANTARSE = 3,
    TAG_PREGUNTAR = 4,
    id_camarero = 10;

// ------------------------------------------------------------
// Generador aleatorio
template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion(min, max);
    return distribucion(generador);
}

// ------------------------------------------------------------
// Filósofos amables
void funcion_filosofos_amables(int id)
{
    int id_ten_izq = (id + 1) % num_procesos;
    int id_ten_der = (id + num_procesos - 1) % num_procesos;

    bool esta_hambriento = true;
    int esta_malafolla;
    bool malafolla;
    int peticion;
    MPI_Status estado;

    while (true)
    {
        // Sentarse
        cout << "FILOSOFO " << id / 2 << ": solicita sentarse." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, TAG_SENTARSE, MPI_COMM_WORLD);

        // Pregunta si está el malafollá
        MPI_Recv(&esta_malafolla, 1, MPI_INT, id_camarero,
                 TAG_PREGUNTAR, MPI_COMM_WORLD, &estado);

        malafolla = (esta_malafolla != 0);

        if (!malafolla || esta_hambriento)
        {
            cout << "FILOSOFO " << id / 2 << ": decide comer." << endl
                 << flush;

            cout << "FILOSOFO " << id / 2 << ": solicita TENEDOR IZQUIERDO -> "
                 << id_ten_izq / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, TAG_TOMAR, MPI_COMM_WORLD);

            cout << "FILOSOFO " << id / 2 << ": solicita TENEDOR DERECHO -> "
                 << id_ten_der / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, TAG_TOMAR, MPI_COMM_WORLD);

            cout << "FILOSOFO " << id / 2 << ": comienza a comer." << endl
                 << flush;
            sleep_for(milliseconds(aleatorio<10, 100>()));

            cout << "FILOSOFO " << id / 2 << ": suelta TENEDOR IZQUIERDO -> "
                 << id_ten_izq / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, TAG_DEJAR, MPI_COMM_WORLD);

            cout << "FILOSOFO " << id / 2 << ": suelta TENEDOR DERECHO -> "
                 << id_ten_der / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, TAG_DEJAR, MPI_COMM_WORLD);

            esta_hambriento = false;
        }
        else
        {
            cout << "FILOSOFO " << id / 2
                 << ": decide no comer. Sigue hambriento." << endl
                 << flush;
            esta_hambriento = true;
        }

        cout << "FILOSOFO " << id / 2 << ": se levanta." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, TAG_LEVANTARSE, MPI_COMM_WORLD);

        cout << "FILOSOFO " << id / 2 << ": comienza a pensar." << endl
             << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));
    }
}

// ------------------------------------------------------------
// Filósofo malafolla
void funcion_filosofo_malafolla(int id)
{
    int id_ten_izq = (id + 1) % num_procesos;
    int id_ten_der = (id + num_procesos - 1) % num_procesos;
    int peticion;

    while (true)
    {
        cout << "FILOSOFO " << id / 2 << ": solicita sentarse." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, TAG_SENTARSE, MPI_COMM_WORLD);

        cout << "FILOSOFO " << id / 2 << ": solicita TENEDOR IZQUIERDO -> "
             << id_ten_izq / 2 << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, TAG_TOMAR, MPI_COMM_WORLD);

        cout << "FILOSOFO " << id / 2 << ": solicita TENEDOR DERECHO -> "
             << id_ten_der / 2 << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, TAG_TOMAR, MPI_COMM_WORLD);

        cout << "FILOSOFO " << id / 2 << ": comienza a comer." << endl
             << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));

        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, TAG_DEJAR, MPI_COMM_WORLD);
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, TAG_DEJAR, MPI_COMM_WORLD);

        cout << "FILOSOFO " << id / 2 << ": se levanta." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, TAG_LEVANTARSE, MPI_COMM_WORLD);

        cout << "FILOSOFO " << id / 2 << ": comienza a pensar." << endl
             << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));
    }
}

// ------------------------------------------------------------
// Tenedores
void funcion_tenedores(int id)
{
    int valor, id_filosofo;
    MPI_Status estado;

    while (true)
    {
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, TAG_TOMAR,
                 MPI_COMM_WORLD, &estado);

        id_filosofo = estado.MPI_SOURCE;
        cout << "TENEDOR " << id / 2
             << ": cogido por FILOSOFO " << id_filosofo / 2 << endl
             << flush;

        MPI_Recv(&valor, 1, MPI_INT, id_filosofo, TAG_DEJAR,
                 MPI_COMM_WORLD, &estado);

        cout << "TENEDOR " << id / 2
             << ": liberado por FILOSOFO " << id_filosofo / 2 << endl
             << flush;
    }
}

// ------------------------------------------------------------
// Camarero
void funcion_camarero()
{
    MPI_Status estado;
    int peticion, num_sentados = 0, tag;
    int esta_malafolla = 0;

    while (true)
    {
        if (num_sentados < num_filosofos - 1)
            tag = MPI_ANY_TAG;
        else
            tag = TAG_LEVANTARSE;

        MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE,
                 tag, MPI_COMM_WORLD, &estado);

        switch (estado.MPI_TAG)
        {
        case TAG_SENTARSE:
            num_sentados++;
            if (estado.MPI_SOURCE != 8)
            {
                MPI_Send(&esta_malafolla, 1, MPI_INT,
                         estado.MPI_SOURCE, TAG_PREGUNTAR, MPI_COMM_WORLD);
            }
            else
                esta_malafolla = 1;
            break;

        case TAG_LEVANTARSE:
            num_sentados--;
            if (estado.MPI_SOURCE == 8)
                esta_malafolla = 0;
            break;
        }
    }
}

// ------------------------------------------------------------
// MAIN
int main(int argc, char **argv)
{
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos == num_procesos_actual)
    {
        if (id_propio == 10)
            funcion_camarero();
        else if (id_propio == 8)
            funcion_filosofo_malafolla(id_propio);
        else if (id_propio % 2 == 0)
            funcion_filosofos_amables(id_propio);
        else
            funcion_tenedores(id_propio);
    }
    else if (id_propio == 0)
    {
        cout << "Número incorrecto de procesos." << endl;
    }

    MPI_Finalize();
    return 0;
}
