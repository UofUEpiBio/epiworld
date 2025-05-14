#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {
    Model<> model;
    model.agents_smallworld(100, 2);

    Virus<> v("covid", 10, false);

    model.add_virus(v);

    Queue<> q;
    q.initialize(&model, nullptr);

    return 0;
}