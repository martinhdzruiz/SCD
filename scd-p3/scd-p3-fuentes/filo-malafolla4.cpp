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
    // filósofos (0,2,4,6,8), tenedores (1,3,5,7,9) y camarero (10)
    num_procesos = 2 * num_filosofos + 1,
    TAG_TOMAR = 0,
    TAG_DEJAR = 1,
    TAG_SENTARSE = 2,
    TAG_LEVANTARSE = 3,
    TAG_PREGUNTAR = 4,
    id_camarero = 2 * num_filosofos; // 10

// generador aleatorio
template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion(min, max);
    return distribucion(generador);
}

// ---------------------------------------------------------------------
// Filósofos "amables"
void funcion_filosofos_amables(int id)
{
    // los tenedores van de 0 a 2*num_filosofos-1 (0..9),
    // el camarero es 10 y no debe ser visto como tenedor
    int id_ten_izq = (id + 1) % (2 * num_filosofos);
    int id_ten_der = (id + 2 * num_filosofos - 1) % (2 * num_filosofos);

    bool hambriento = true; // empiezan hambrientos
    int info_malafolla;
    int peticion = 1;
    MPI_Status estado;

    while (true)
    {
        // solicitar sentarse
        cout << "FILOSOFO " << id / 2 << ": solicita sentarse." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_camarero, TAG_SENTARSE, MPI_COMM_WORLD);

        // preguntar al camarero si el malafollá está sentado
        MPI_Recv(&info_malafolla, 1, MPI_INT,
                 id_camarero, TAG_PREGUNTAR, MPI_COMM_WORLD, &estado);
        bool malafolla_sentado = (info_malafolla != 0);

        if (!malafolla_sentado || hambriento)
        {
            // come normalmente
            cout << "FILOSOFO " << id / 2 << ": decide comer." << endl
                 << flush;

            cout << "FILOSOFO " << id / 2 << ": pide TENEDOR IZQ -> "
                 << id_ten_izq / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT,
                      id_ten_izq, TAG_TOMAR, MPI_COMM_WORLD);

            cout << "FILOSOFO " << id / 2 << ": pide TENEDOR DER -> "
                 << id_ten_der / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT,
                      id_ten_der, TAG_TOMAR, MPI_COMM_WORLD);

            cout << "FILOSOFO " << id / 2 << ": COMIENDO." << endl
                 << flush;
            sleep_for(milliseconds(aleatorio<10, 100>()));

            cout << "FILOSOFO " << id / 2 << ": suelta TENEDOR IZQ -> "
                 << id_ten_izq / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT,
                      id_ten_izq, TAG_DEJAR, MPI_COMM_WORLD);

            cout << "FILOSOFO " << id / 2 << ": suelta TENEDOR DER -> "
                 << id_ten_der / 2 << endl
                 << flush;
            MPI_Ssend(&peticion, 1, MPI_INT,
                      id_ten_der, TAG_DEJAR, MPI_COMM_WORLD);

            // tras comer pasa a saciado
            hambriento = false;
        }
        else
        {
            // malafollá sentado y estaba saciado → se salta la comida
            cout << "FILOSOFO " << id / 2
                 << ": se SALTA la comida por el malafollá." << endl
                 << flush;
            // si se salta una comida → pasa a hambriento
            hambriento = true;
        }

        // levantarse
        cout << "FILOSOFO " << id / 2 << ": se levanta." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_camarero, TAG_LEVANTARSE, MPI_COMM_WORLD);

        // pensar
        cout << "FILOSOFO " << id / 2 << ": pensando." << endl
             << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));
    }
}

// ---------------------------------------------------------------------
// Filósofo malafollá (ID 8, filósofo nº 4)
void funcion_filosofo_malafolla(int id)
{
    int id_ten_izq = (id + 1) % (2 * num_filosofos);
    int id_ten_der = (id + 2 * num_filosofos - 1) % (2 * num_filosofos);
    int peticion = 1;

    while (true)
    {
        cout << "MALAFOLLA " << id / 2 << ": solicita sentarse." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_camarero, TAG_SENTARSE, MPI_COMM_WORLD);

        cout << "MALAFOLLA " << id / 2 << ": pide TENEDOR IZQ -> "
             << id_ten_izq / 2 << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_ten_izq, TAG_TOMAR, MPI_COMM_WORLD);

        cout << "MALAFOLLA " << id / 2 << ": pide TENEDOR DER -> "
             << id_ten_der / 2 << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_ten_der, TAG_TOMAR, MPI_COMM_WORLD);

        cout << "MALAFOLLA " << id / 2 << ": COMIENDO." << endl
             << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));

        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_ten_izq, TAG_DEJAR, MPI_COMM_WORLD);
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_ten_der, TAG_DEJAR, MPI_COMM_WORLD);

        cout << "MALAFOLLA " << id / 2 << ": se levanta." << endl
             << flush;
        MPI_Ssend(&peticion, 1, MPI_INT,
                  id_camarero, TAG_LEVANTARSE, MPI_COMM_WORLD);

        cout << "MALAFOLLA " << id / 2 << ": pensando." << endl
             << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));
    }
}

// ---------------------------------------------------------------------
// Tenedores
void funcion_tenedores(int id)
{
    int valor, id_filosofo;
    MPI_Status estado;

    while (true)
    {
        MPI_Recv(&valor, 1, MPI_INT,
                 MPI_ANY_SOURCE, TAG_TOMAR, MPI_COMM_WORLD, &estado);
        id_filosofo = estado.MPI_SOURCE;
        cout << "TENEDOR " << id / 2 << ": cogido por FILOSOFO "
             << id_filosofo / 2 << endl
             << flush;

        MPI_Recv(&valor, 1, MPI_INT,
                 id_filosofo, TAG_DEJAR, MPI_COMM_WORLD, &estado);
        cout << "TENEDOR " << id / 2 << ": liberado por FILOSOFO "
             << id_filosofo / 2 << endl
             << flush;
    }
}

// ---------------------------------------------------------------------
// Camarero
void funcion_camarero()
{
    MPI_Status estado;
    int peticion;
    int num_sentados = 0;
    int info_malafolla = 0; // 0 = no sentado, 1 = malafollá sentado

    while (true)
    {
        int tag;
        if (num_sentados < num_filosofos - 1)
            tag = MPI_ANY_TAG; // puedo aceptar SENTARSE o LEVANTARSE
        else
            tag = TAG_LEVANTARSE; // ya hay 4 sentados, solo espero LEVANTARSE

        MPI_Recv(&peticion, 1, MPI_INT,
                 MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &estado);

        switch (estado.MPI_TAG)
        {
        case TAG_SENTARSE:
            num_sentados++;
            if (estado.MPI_SOURCE != 8) // filósofo amable
            {
                // responde indicándole si el malafollá está sentado o no
                MPI_Ssend(&info_malafolla, 1, MPI_INT,
                          estado.MPI_SOURCE, TAG_PREGUNTAR, MPI_COMM_WORLD);
            }
            else
            {
                // se ha sentado el malafollá
                info_malafolla = 1;
            }
            break;

        case TAG_LEVANTARSE:
            num_sentados--;
            if (estado.MPI_SOURCE == 8)
                info_malafolla = 0; // el malafollá ya no está sentado
            break;
        }
    }
}

// ---------------------------------------------------------------------
int main(int argc, char **argv)
{
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos_actual == num_procesos)
    {
        if (id_propio == id_camarero) // 10
            funcion_camarero();
        else if (id_propio == 8) // filósofo nº 4 malafollá
            funcion_filosofo_malafolla(id_propio);
        else if (id_propio % 2 == 0) // resto de filósofos (0,2,4,6)
            funcion_filosofos_amables(id_propio);
        else // tenedores (1,3,5,7,9)
            funcion_tenedores(id_propio);
    }
    else if (id_propio == 0)
    {
        cout << "Se esperaban " << num_procesos
             << " procesos y hay " << num_procesos_actual
             << ". Programa abortado." << endl;
    }

    MPI_Finalize();
    return 0;
}
