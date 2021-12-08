#include <string>
#include "../include/epiworld/epiworld.hpp"

int main() {

    epiworld::AdjList L;
    L.read_edgelist("edgelist.txt");

    L.print();

    return 0;
}