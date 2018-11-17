#include "TreeQuorum.h"

using namespace std;

FILE *inp = fopen("inp-params.txt", "r");
FILE *out = fopen("output-log.txt", "w");

typedef struct TQMessage {
    int id;
    int sq;
}mes;

int blocklengths[2] = {1, 1};
MPI_Datatype types[2] = {MPI_INT, MPI_INT};
MPI_Datatype Message;
MPI_Aint offsets[2];


TreeQuorum::TreeQuorum(int id, int N, int terminator) : id(id), N(N) {
    this->terminator = terminator;
    this->term = false;
}

void TreeQuorum::commitMessageDataType() {

}

