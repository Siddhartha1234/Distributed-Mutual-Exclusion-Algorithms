using namespace std;

FILE *inp = fopen("inp-params.txt", "r");
FILE *out = fopen("output-log.txt", "w");

typedef struct message {
    int id;
    int sq;
}mes;

int blocklengths[2] = {1, 1};
MPI_Datatype types[2] = {MPI_INT, MPI_INT};
MPI_Datatype Message;
MPI_Aint offsets[2];

PathReversal::PathReversal(int id, int N, int terminator) : MutualExcl(id, N) {
    this->terminator = terminator;
    this->term = false;
}


