#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    Model<> model;
    model.agents_smallworld(100, 2);

    Virus<> v("covid");
    
    model.add_virus_n(v, 10);

    Queue<> q;
    q.initialize(&model, nullptr);

    return 0;

}