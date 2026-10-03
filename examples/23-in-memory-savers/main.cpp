#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

// Collects repeated simulations in memory instead of writing files. This is
// also what WebAssembly CI runs (.github/workflows/wasm.yml).
int main()
{

    epimodels::ModelSIRCONN<> model("virus", 100, 0.1, 4.0, 0.2, 0.1);
    model.verbose_off();

    SaverMemory<> saver;
    model.run_multiple(10, 3, 42, saver, true, false);

    const auto history = saver.results().total_hist;
    printf("sim_id date state counts\n");
    for (size_t i = 0u; i < history.size(); ++i)
        printf(
            "%i %i \"%s\" %i\n",
            history.sim_id[i], history.date[i], history.state[i].c_str(),
            history.counts[i]
        );

    // 3 simulations x 11 days x 3 states
    return history.size() == 99u ? 0 : 1;

}
