// Writes the diagram of each model (`<dir>/<id>.mmd`, the id in lower case)
// from its declarations (Model::draw_natural_history()). Run with
// `make diagrams`.
#include <fstream>
#include <iostream>
#include "../include/epiworld/epiworld.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

static void write(const Model & model, const std::string & fn)
{
    std::ofstream out(fn);
    if (!out)
        throw std::runtime_error("Cannot write " + fn);
    out << "%% Drawn from the model's declarations by `make diagrams`; do not edit.\n";
    out << model.draw_natural_history();
    std::cout << "Wrote " << fn << std::endl;
}

int main()
{

    ModelRegistry core, measles_models;
    epimodels::register_models(core);
    measles::register_models(measles_models);

    auto lower = [](std::string x) {
        for (auto & c : x)
            c = static_cast< char >(std::tolower(static_cast< unsigned char >(c)));
        return x;
    };

    for (const auto & info : core.list_models())
        write(*core.make_model(info.id, {}), "include/epiworld/models/" + lower(info.id) + ".mmd");

    for (const auto & info : measles_models.list_models())
        write(*measles_models.make_model(info.id, {}), "include/measles/" + lower(info.id) + ".mmd");

    // Built directly (it takes data); the diagram needs none
    std::vector< double > data(2u, 1.0);
    epimodels::ModelSIRLogit sirlogit(
        "Disease", 1u, data.data(), 2u, {0.0, 0.0, 0.0}, {0.0, 0.0},
        {0u, 1u}, {0u, 1u}, 0.1, 0.1, 0.0
    );
    write(sirlogit, "include/epiworld/models/sirlogit.mmd");

    return 0;

}
