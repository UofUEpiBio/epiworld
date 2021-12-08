#include <string>
#include "../include/epiworld/epiworld.hpp"

int main() {

    epiworld::ReadGraph R;
    R.read_edgelist("edgelist.txt");

    R.print();

    return 0;
}