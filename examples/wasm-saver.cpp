#include "../include/epiworld/epiworld.hpp"
#include <iostream>

int main() {
    epiworld::epimodels::ModelSIRCONN<> model(
        "virus", 100, 0.1, 4.0, 0.2, 0.1);
    model.verbose_off();
    epiworld::SaverMemory<> saver;
    model.run_multiple(10, 3, 42, saver, true, false);
    const auto history = saver.results().total_hist;
    std::cout << "sim_id,date,state,counts\n";
    for (size_t i = 0; i < history.size(); ++i)
        std::cout << history.sim_id[i] << ',' << history.date[i] << ','
                  << history.state[i] << ',' << history.counts[i] << '\n';
    return history.size() == 99 ? 0 : 1;
}
