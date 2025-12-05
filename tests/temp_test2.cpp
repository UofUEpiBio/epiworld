#define EPI_DEBUG
#include "tests.hpp"
using namespace epiworld;

int main() {
    // Create a model with a tool
    epimodels::ModelSEIRCONN<> model(
        "test virus", 1000, 0.01, 10.0, 0.5, 4.0, 7.0
    );
    model.verbose_off();
    model.run(5, 123);
    
    model.write_data("", "virus_hist_compare.csv", "", "tool_hist_compare.csv", "", "", "", "", "");
    
    std::cout << "=== VIRUS HIST ===" << std::endl;
    system("head -10 virus_hist_compare.csv");
    
    std::cout << "\n=== TOOL HIST ===" << std::endl;
    system("head -10 tool_hist_compare.csv");
    
    return 0;
}
