#include "mpi.h"
#include <iostream>
#include <fstream>

#define COORDONATOR0 0
#define COORDONATOR1 1
#define COORDONATOR2 2

using namespace std;

//functie de printare topologii
void printTopology(int rank, int nr0, int C0[], int nr1, int C1[], int nr2, int C2[]) {
    cout << rank << " -> " << COORDONATOR0 << ":";
    for(int i = 0; i < nr0; i++) {
        cout << C0[i];
        if(i != nr0 - 1) {
            cout << ",";
        }
    }
    cout << " " << COORDONATOR1 << ":";
    for(int i = 0; i < nr1; i++) {
        cout << C1[i];
        if(i != nr1 - 1) {
            cout << ",";
        }
    }
    cout << " " << COORDONATOR2 << ":";
    for(int i = 0; i < nr2; i++) {
        cout << C2[i];
        if(i != nr2 - 1) {
            cout << ",";
        }
    }
    cout << endl;
}

int main (int argc, char *argv[])
{
    int  numtasks, rank, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Get_processor_name(hostname, &len);

    MPI_Status status;

    if(rank == COORDONATOR0) {
        //citim datele din fisier
        fstream my_file;
        string s = "cluster" + to_string(COORDONATOR0) + ".txt";
        my_file.open(s, ios::in);
        int numberOfWorkers;
        my_file >> numberOfWorkers;
        int C0[numberOfWorkers];
        for(int i = 0; i < numberOfWorkers; i++) {
                my_file >> C0[i];
        }
        my_file.close();

        //Coordonatorul 0 isi trimite datele lui 2
        cout << "M(" << COORDONATOR0 << "," << COORDONATOR2 << ")" << endl;
        MPI_Send(&numberOfWorkers, 1, MPI_INT, COORDONATOR2, COORDONATOR0, MPI_COMM_WORLD);
        MPI_Send(C0, numberOfWorkers, MPI_INT, COORDONATOR2, COORDONATOR0, MPI_COMM_WORLD);

        //Coordonatorul 0 primeste datele de la coordonatorul 2
        int nr2;
        MPI_Recv(&nr2, 1, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);
        int C2[nr2];
        MPI_Recv(C2, nr2, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status); 

        //Coordonatorul 0 primeste datele de la coordonatorul 1 prin 2
        int nr1;
        MPI_Recv(&nr1, 1, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);
        int C1[nr1];
        MPI_Recv(C1, nr1, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);
        
        printTopology(rank, numberOfWorkers, C0, nr1, C1, nr2, C2);

        //trimitem datele catre workeri
        for(int i = 0; i < numberOfWorkers; i++) {
            cout << "M(" << COORDONATOR0 << "," << C0[i] << ")" << endl;
            MPI_Send(&rank, 1, MPI_INT, C0[i], 0, MPI_COMM_WORLD);

            MPI_Send(&numberOfWorkers, 1, MPI_INT, C0[i], rank, MPI_COMM_WORLD);
            MPI_Send(C0, numberOfWorkers, MPI_INT, C0[i], rank, MPI_COMM_WORLD);

            MPI_Send(&nr1, 1, MPI_INT, C0[i], rank, MPI_COMM_WORLD);
            MPI_Send(C1, nr1, MPI_INT, C0[i], rank, MPI_COMM_WORLD);

            MPI_Send(&nr2, 1, MPI_INT, C0[i], rank, MPI_COMM_WORLD);
            MPI_Send(C2, nr2, MPI_INT, C0[i], rank, MPI_COMM_WORLD);
        }

        //retinem numarul de elem din vector
        int N = atoi(argv[1]);
        int v[N];
        for(int i = 0; i < N; i++) {
            v[i] = i;
        }

        //trimitem numarul de elem si vectorul la coord 2
        MPI_Send(&N, 1, MPI_INT, COORDONATOR2, 0, MPI_COMM_WORLD);
        MPI_Send(v, N, MPI_INT, COORDONATOR2, 0, MPI_COMM_WORLD);

        //trimitem numarul de elem si vect la workeri
        for(int i = 0 ; i < numberOfWorkers; i++) {
            MPI_Send(&N, 1, MPI_INT, C0[i], 0, MPI_COMM_WORLD);
            MPI_Send(v, N, MPI_INT, C0[i], 0, MPI_COMM_WORLD);
            int pos = N/(numberOfWorkers + nr1 + nr2) * i;
            MPI_Send(&pos, 1, MPI_INT, C0[i], 0, MPI_COMM_WORLD);
        }

        //primim vectorul modificat de la workeri
        for(int i = 0 ; i < numberOfWorkers; i++) {
            int a[N];
            MPI_Recv(a, N, MPI_INT, C0[i], 0, MPI_COMM_WORLD, &status);
            int pos = N/(numberOfWorkers + nr1 + nr2) * i;
            for(int j = pos; j < pos + N/(numberOfWorkers + nr1 + nr2); j++) {
                v[j] = a[j];
            }
        }

        //primim vectorul modificat de la ceilalti coordonatori
        int a[N];
        MPI_Recv(a, N, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);
        int pos = N/(numberOfWorkers + nr1 + nr2) * numberOfWorkers;
        for(int i = pos; i < pos + N/(numberOfWorkers + nr1 + nr2) * nr1; i++) {
                v[i] = a[i];
        }

        MPI_Recv(a, N, MPI_INT, COORDONATOR2, 0, MPI_COMM_WORLD, &status);
        pos = N/(numberOfWorkers + nr1 + nr2) * (numberOfWorkers + nr1);
        for(int i = pos; i < pos + N/(numberOfWorkers + nr1 + nr2) * nr2; i++) {
                v[i] = a[i];
        }

        //verificam daca mai ramasesera elem pe dinanfara
        if(pos == N/(nr2 + nr1 + numberOfWorkers) * (numberOfWorkers + nr1)) {
            for(int j = N/(nr2 + nr1 + numberOfWorkers) * (nr2 + nr1 + numberOfWorkers); j < N; j++) {
                v[j] = a[j];
            }
        }

        cout << "Rezultat: ";
        for(int i = 0; i < N; i++) {
            cout << v[i] << " ";
        }
        cout << endl;

    } else if(rank == COORDONATOR1) {
        //citim datele din fisier
        fstream my_file;
        string s = "cluster" + to_string(COORDONATOR1) + ".txt";
        my_file.open(s, ios::in);
        int numberOfWorkers;
        my_file >> numberOfWorkers;
        int C1[numberOfWorkers];
        for(int i = 0; i < numberOfWorkers; i++) {
                my_file >> C1[i];
        }
        my_file.close();

        //Coordonatorul 1 isi trimite datele lui 2

        cout << "M(" << COORDONATOR1 << "," << COORDONATOR2 << ")" << endl;
        MPI_Send(&numberOfWorkers, 1, MPI_INT, COORDONATOR2, COORDONATOR1, MPI_COMM_WORLD);
        MPI_Send(C1, numberOfWorkers, MPI_INT, COORDONATOR2, COORDONATOR1, MPI_COMM_WORLD);

        //Coordonatorul 1 primeste datele de la coordonatorul 2
        int nr2;
        MPI_Recv(&nr2, 1, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);
        int C2[nr2];
        MPI_Recv(C2, nr2, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);

        //Coordonatorul 1 primeste datele de la coordonatorul 0 prin 2
        int nr0;
        MPI_Recv(&nr0, 1, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);
        int C0[nr0];
        MPI_Recv(C0, nr0, MPI_INT, COORDONATOR2, COORDONATOR2, MPI_COMM_WORLD, &status);

        printTopology(rank, nr0, C0, numberOfWorkers, C1, nr2, C2);

        //trimitem datele catre workeri
        for(int i = 0; i < numberOfWorkers; i++) {
            cout << "M(" << COORDONATOR1 << "," << C1[i] << ")" << endl;
            MPI_Send(&rank, 1, MPI_INT, C1[i], 0, MPI_COMM_WORLD);

            MPI_Send(&nr0, 1, MPI_INT, C1[i], rank, MPI_COMM_WORLD);
            MPI_Send(C0, nr0, MPI_INT, C1[i], rank, MPI_COMM_WORLD);

            MPI_Send(&numberOfWorkers, 1, MPI_INT, C1[i], rank, MPI_COMM_WORLD);
            MPI_Send(C1, numberOfWorkers, MPI_INT, C1[i], rank, MPI_COMM_WORLD);

            MPI_Send(&nr2, 1, MPI_INT, C1[i], rank, MPI_COMM_WORLD);
            MPI_Send(C2, nr2, MPI_INT, C1[i], rank, MPI_COMM_WORLD);
        }

        //primim numarul de elem si vectorul de la coord 2
        int N;
        MPI_Recv(&N, 1, MPI_INT, COORDONATOR2, 0, MPI_COMM_WORLD, &status);
        int v[N];
        MPI_Recv(v, N, MPI_INT, COORDONATOR2, 0, MPI_COMM_WORLD, &status);

        //le trimitem mai departe la workeri
        for(int i = 0 ; i < numberOfWorkers; i++) {
            MPI_Send(&N, 1, MPI_INT, C1[i], 1, MPI_COMM_WORLD);
            MPI_Send(v, N, MPI_INT, C1[i], 1, MPI_COMM_WORLD);
            int pos = N/(numberOfWorkers + nr0 + nr2) * (nr0 + i);
            MPI_Send(&pos, 1, MPI_INT, C1[i], 1, MPI_COMM_WORLD);
        }

        //primim vect modificat de la workeri
        for(int i = 0 ; i < numberOfWorkers; i++) {
            int a[N];
            MPI_Recv(a, N, MPI_INT, C1[i], 0, MPI_COMM_WORLD, &status);
            int pos = N/(numberOfWorkers + nr0 + nr2) * (nr0 + i);
            for(int j = pos; j < pos + N/(numberOfWorkers + nr0 + nr2); j++) {
                v[j] = a[j];
            }
        }

        //trimitem rez inapoi la procesul 2
        MPI_Send(v, N, MPI_INT, COORDONATOR2, COORDONATOR1, MPI_COMM_WORLD);

    } else if(rank == COORDONATOR2) {
        //citim datele din fisier
        fstream my_file;
        string s = "cluster" + to_string(COORDONATOR2) + ".txt";
        my_file.open(s, ios::in);
        int numberOfWorkers;
        my_file >> numberOfWorkers;
        int C2[numberOfWorkers];
        for(int i = 0; i < numberOfWorkers; i++) {
                my_file >> C2[i];
        }
        my_file.close();

        //Coordonatorul 2 isi trimite datele celorlalti 2 coordonatori
        cout << "M(" << COORDONATOR2 << "," << COORDONATOR0 << ")" << endl;
        MPI_Send(&numberOfWorkers, 1, MPI_INT, COORDONATOR0, COORDONATOR2, MPI_COMM_WORLD);
        MPI_Send(C2, numberOfWorkers, MPI_INT, COORDONATOR0, COORDONATOR2, MPI_COMM_WORLD);
        cout << "M(" << COORDONATOR2 << "," << COORDONATOR1 << ")" << endl;
        MPI_Send(&numberOfWorkers, 1, MPI_INT, COORDONATOR1, COORDONATOR2, MPI_COMM_WORLD);
        MPI_Send(C2, numberOfWorkers, MPI_INT, COORDONATOR1, COORDONATOR2, MPI_COMM_WORLD);

        //Coordonatorul 2 primeste datele de la coordonatorul 0
        int nr0;
        MPI_Recv(&nr0, 1, MPI_INT, COORDONATOR0, COORDONATOR0, MPI_COMM_WORLD, &status);
        int C0[nr0];
        MPI_Recv(C0, nr0, MPI_INT, COORDONATOR0, COORDONATOR0, MPI_COMM_WORLD, &status);

        //Coordonatorul 2 primeste datele de la coordonatorul 1
        int nr1;
        MPI_Recv(&nr1, 1, MPI_INT, COORDONATOR1, COORDONATOR1, MPI_COMM_WORLD, &status);
        int C1[nr1];
        MPI_Recv(C1, nr1, MPI_INT, COORDONATOR1, COORDONATOR1, MPI_COMM_WORLD, &status);

        //Coordonatorul 2 trimite datele de la 0 la 1
        cout << "M(" << COORDONATOR2 << "," << COORDONATOR1 << ")" << endl;
        MPI_Send(&nr0, 1, MPI_INT, COORDONATOR1, COORDONATOR2, MPI_COMM_WORLD);
        MPI_Send(C0, nr0, MPI_INT, COORDONATOR1, COORDONATOR2, MPI_COMM_WORLD);

        //Coordonatorul 2 trimite datele de la 1 la 0
        cout << "M(" << COORDONATOR2 << "," << COORDONATOR0 << ")" << endl;
        MPI_Send(&nr1, 1, MPI_INT, COORDONATOR0, COORDONATOR2, MPI_COMM_WORLD);
        MPI_Send(C1, nr1, MPI_INT, COORDONATOR0, COORDONATOR2, MPI_COMM_WORLD);

        printTopology(rank, nr0, C0, nr1, C1, numberOfWorkers, C2);

        //trimitem datele catre workeri
        for(int i = 0; i < numberOfWorkers; i++) {
            cout << "M(" << COORDONATOR2 << "," << C2[i] << ")" << endl;
            MPI_Send(&rank, 1, MPI_INT, C2[i], 0, MPI_COMM_WORLD);

            MPI_Send(&nr0, 1, MPI_INT, C2[i], rank, MPI_COMM_WORLD);
            MPI_Send(C0, nr0, MPI_INT, C2[i], rank, MPI_COMM_WORLD);

            MPI_Send(&nr1, 1, MPI_INT, C2[i], rank, MPI_COMM_WORLD);
            MPI_Send(C1, nr1, MPI_INT, C2[i], rank, MPI_COMM_WORLD);

            MPI_Send(&numberOfWorkers, 1, MPI_INT, C2[i], rank, MPI_COMM_WORLD);
            MPI_Send(C2, numberOfWorkers, MPI_INT, C2[i], rank, MPI_COMM_WORLD);
        }

        //primim numarul de elem si vectorul
        int N;
        MPI_Recv(&N, 1, MPI_INT, COORDONATOR0, 0, MPI_COMM_WORLD, &status);
        int v[N];
        MPI_Recv(v, N, MPI_INT, COORDONATOR0, 0, MPI_COMM_WORLD, &status);

        //trimitem datele catre coord 1
        MPI_Send(&N, 1, MPI_INT, COORDONATOR1, 0, MPI_COMM_WORLD);
        MPI_Send(v, N, MPI_INT, COORDONATOR1, 0, MPI_COMM_WORLD);

        //trimitem datele de workeri
        for(int i = 0 ; i < numberOfWorkers; i++) {
            MPI_Send(&N, 1, MPI_INT, C2[i], 2, MPI_COMM_WORLD);
            MPI_Send(v, N, MPI_INT, C2[i], 2, MPI_COMM_WORLD);
            int pos = N/(numberOfWorkers + nr0 + nr1) * (nr0 + nr1 + i);
            MPI_Send(&pos, 1, MPI_INT, C2[i], 2, MPI_COMM_WORLD);
        }

        //le primim inapoi verificand daca au ramas elem nemodificate
        for(int i = 0 ; i < numberOfWorkers; i++) {
            int a[N];
            MPI_Recv(a, N, MPI_INT, C2[i], 0, MPI_COMM_WORLD, &status);
            int pos = N/(numberOfWorkers + nr0 + nr1) * (nr0 + nr1 + i);
            for(int j = pos; j < pos + N/(numberOfWorkers + nr0 + nr1); j++) {
                v[j] = a[j];
            }
            if(pos == N/(nr0 + nr1 + numberOfWorkers) * (nr0 + nr1 + numberOfWorkers - 1)) {
                for(int j = N/(nr0 + nr1 + numberOfWorkers) * (nr0 + nr1 + numberOfWorkers); j < N; j++) {
                    v[j] = a[j];
                }
            }
        }

        //primim si trimitem datele de la 1
        int a[N];
        MPI_Recv(a, N, MPI_INT, COORDONATOR1, COORDONATOR1, MPI_COMM_WORLD, &status);
        MPI_Send(a, N, MPI_INT, COORDONATOR0, COORDONATOR2, MPI_COMM_WORLD);

        //le trimitem inapoi la procesul 0
         MPI_Send(v, N, MPI_INT, 0, 0, MPI_COMM_WORLD);

    } else {

        //Trimitem rankul coordonatorului si topologiile catre workeri
        int rankCoord;
        MPI_Recv(&rankCoord, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        int nr0;
        MPI_Recv(&nr0, 1, MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);
        int C0[nr0];
        MPI_Recv(C0, nr0 ,MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);

        int nr1;
        MPI_Recv(&nr1, 1, MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);
        int C1[nr1];
        MPI_Recv(C1, nr1 ,MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);

        int nr2;
        MPI_Recv(&nr2, 1, MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);
        int C2[nr2];
        MPI_Recv(C2, nr2 ,MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);

        printTopology(rank, nr0, C0, nr1, C1, nr2, C2);

        //primim informatiile despre vect care trebuie modificat
        int N;
        MPI_Recv(&N, 1, MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);
        int v[N];
        MPI_Recv(v, N, MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);
        int number = N / (nr0 + nr1 + nr2);
        int pos;
        MPI_Recv(&pos, 1, MPI_INT, rankCoord, rankCoord, MPI_COMM_WORLD, &status);
        for(int i = pos; i < pos + number; i++) {
            v[i] *= 2;
        }

        if(pos == N/(nr0 + nr1 + nr2) * (nr0 + nr1 + nr2 - 1)) {
            for(int i = N/(nr0 + nr1 + nr2) * (nr0 + nr1 + nr2); i < N; i++) {
                v[i] *= 2;
            }
        }

        //trimitem inapoi la coordonatori
        MPI_Send(v, N, MPI_INT, rankCoord, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}
