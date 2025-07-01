#include "tests/tests.hpp"

using namespace epiworld;

int main() {
    
    printf("Starting test...\n");
    
    // Creating a basic SEIR model with mixing
    printf("Creating model...\n");
    epimodels::ModelSEIRMixing<> model(
        "Test model", 100, 0.01, 40.0, 1.0, 0.5, 1.0,
        {
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
        }
    );

    printf("Model created successfully!\n");
    return 0;
}
