#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

#include "MutualExclApp.h"

class TreeQuorum  {
    bool has_token;
    bool requesting_CS;
    bool term;
    int father;
    int terminator;
public:
    thread worker_thread;
    thread receiver_thread;
    TreeQuorum(int id, int N, int terminator);
    void commitMessageDataType();
};