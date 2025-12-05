#define EPI_DEBUG
#include "tests.hpp"
using namespace epiworld;

int main() {
    epimodels::ModelSIR<> model("test virus", 10.0, 0.9, 0.3);
    model.seed(1231);
    model.agents_smallworld(100, 4, false, 0.01);
    model.verbose_off();
    model.run(5, 123);
    
    model.write_data("", "virus_hist_fixed2.csv", "", "", "", "", "", "", "");
    
    std::cout << "=== FIXED VIRUS HIST (with quotes on both virus and state) ===" << std::endl;
    system("head -15 virus_hist_fixed2.csv");
    
    return 0;
}
