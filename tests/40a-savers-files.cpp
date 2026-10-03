#include "tests.hpp"
using namespace epiworld;

EPIWORLD_TEST_CASE("Memory savers match all legacy files", "[savers]") {
    epimodels::ModelSEIRCONN<> model("test virus", 100, 0.1, 5.0, 0.5, 3.0, 0.2);
    model.verbose_off();
    Tool<> tool("test tool", 0.5, true);
    model.add_tool(tool);
    model.add_globalevent([](Model<>* m) {
        for (auto& agent : m->get_agents())
            if (agent.get_virus()) { m->record_hospitalization(agent); break; }
    }, "record hospitalizations");
    SaveOptions options;
    options.total_hist = true;
    options.virus_info = true;
    options.virus_hist = true;
    options.tool_info = true;
    options.tool_hist = true;
    options.transmission = true;
    options.transition = true;
    options.reproductive = true;
    options.generation = true;
    options.active_cases = true;
    options.outbreak_size = true;
    options.hospitalizations = true;
    auto temp = epi_temp_file("40a-savers", "csv");
    SaverFiles<> files(temp.directory + "/new-%zu", options);
    SaverMemory<> memory(options);
    auto legacy = make_save_run<int>(temp.directory + "/old-%zu",
        true, true, true, true, true, true, true, true, true, true, true, true);
    model.run_multiple(10, 3, 123, [&](size_t id, Model<>* m) {
        legacy(id, m);
        m->write_data(
            temp.directory + "/direct-" + std::to_string(id) + "_virus_info.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_virus_hist.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_tool_info.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_tool_hist.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_total_hist.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_transmission.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_transition.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_reproductive.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_generation.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_active_cases.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_outbreak_size.csv",
            temp.directory + "/direct-" + std::to_string(id) + "_hospitalizations.csv"
        );
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_total_hist.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_total_hist.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_virus_info.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_virus_info.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_virus_hist.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_virus_hist.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_tool_info.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_tool_info.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_tool_hist.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_tool_hist.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_transmission.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_transmission.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_transition.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_transition.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_reproductive.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_reproductive.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_generation.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_generation.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_active_cases.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_active_cases.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_outbreak_size.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_outbreak_size.csv"));
        REQUIRE(file_reader(temp.directory + "/direct-" + std::to_string(id) + "_hospitalizations.csv") ==
            file_reader(temp.directory + "/old-" + std::to_string(id) + "_hospitalizations.csv"));
        auto out = memory.extract(id, *m);
        files.write(id, RunOutputs(out));
        {
            std::ostringstream expected;
            out.total_hist.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_total_hist.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_total_hist.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_total_hist.csv"));
        }
        {
            std::ostringstream expected;
            out.virus_info.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_virus_info.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_virus_info.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_virus_info.csv"));
        }
        {
            std::ostringstream expected;
            out.virus_hist.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_virus_hist.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_virus_hist.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_virus_hist.csv"));
        }
        {
            std::ostringstream expected;
            out.tool_info.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_tool_info.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_tool_info.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_tool_info.csv"));
        }
        {
            std::ostringstream expected;
            out.tool_hist.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_tool_hist.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_tool_hist.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_tool_hist.csv"));
        }
        {
            std::ostringstream expected;
            out.transmission.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_transmission.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_transmission.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_transmission.csv"));
        }
        {
            std::ostringstream expected;
            out.transition.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_transition.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_transition.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_transition.csv"));
        }
        {
            std::ostringstream expected;
            out.reproductive.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_reproductive.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_reproductive.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_reproductive.csv"));
        }
        {
            std::ostringstream expected;
            out.generation.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_generation.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_generation.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_generation.csv"));
        }
        {
            std::ostringstream expected;
            out.active_cases.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_active_cases.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_active_cases.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_active_cases.csv"));
        }
        {
            std::ostringstream expected;
            out.outbreak_size.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_outbreak_size.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_outbreak_size.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_outbreak_size.csv"));
        }
        {
            std::ostringstream expected;
            out.hospitalizations.write(expected);
            std::ifstream input(temp.directory + "/old-" + std::to_string(id) + "_hospitalizations.csv");
            std::string actual((std::istreambuf_iterator<char>(input)), {});
            REQUIRE(expected.str() == actual);
            REQUIRE(file_reader(temp.directory + "/new-" + std::to_string(id) + "_hospitalizations.csv") ==
                file_reader(temp.directory + "/old-" + std::to_string(id) + "_hospitalizations.csv"));
        }
        memory.write(id, std::move(out));
    }, true, false, 1);
    auto result = memory.results();
    REQUIRE(result.total_hist.size() == 3 * 11 * 4);
    REQUIRE(result.tool_info.size() == 3);
    REQUIRE(result.hospitalizations.size() > 0);
    REQUIRE(result.total_hist.sim_id.front() == 0);
    REQUIRE(result.total_hist.sim_id.back() == 2);
    // The new overload produces the same result, and begin clears old runs.
    model.run_multiple(10, 3, 123, memory, true, false);
    REQUIRE(memory.results().total_hist.counts == result.total_hist.counts);
}
