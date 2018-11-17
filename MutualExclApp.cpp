#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

char *getTime(time_t input) {
    struct tm *timeinfo;
    timeinfo = localtime(&input);
    static char output[10];
    sprintf(output, "%.2d:%.2d:%.2d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return output;
}

template<typename Out>
void split(string &s, char delim, Out result) {
    stringstream ss(s);
    string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

vector <string> split(string &s, char delim) {
    vector <string> elems;
    split(s, delim, back_inserter(elems));
    return elems;
}

class MutualExclApp {
    int N;
    int id;


};
