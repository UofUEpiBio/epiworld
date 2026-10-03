#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Savers match the database files", "[savers]") {

    // A tool and a hospitalization every day, so every output has rows
    epimodels::ModelSEIRCONN<> model("test virus", 100, 0.1, 5.0, 0.5, 3.0, 0.2);
    model.verbose_off();
    Tool<> tool("test tool", 0.5, true);
    model.add_tool(tool);
    model.add_globalevent([](Model<> * m) {
        for (auto & agent : m->get_agents())
            if (agent.get_virus())
            {
                m->record_hospitalization(agent);
                break;
            }
    }, "record hospitalizations");

    SaveOptions all;
    RunOutputs::for_each_table([&](const char *, auto option, auto) {
        all.*option = true;
    });

    // The file format is the contract downstream readers (e.g., epiworldR)
    // rely on, so headers are spelled out here.
    #ifdef EPI_DEBUG
    const std::string thread = "thread ";
    #else
    const std::string thread = "";
    #endif
    const std::map< std::string, std::string > headers = {
        {"total_hist", "date nviruses state counts"},
        {"virus_info", "virus_id virus virus_sequence date_recorded parent"},
        {"virus_hist", "date virus_id virus state n"},
        {"tool_info", "id tool_name tool_sequence date_recorded"},
        {"tool_hist", "date id state n"},
        {"transmission", "date virus_id virus source_exposure_date source target"},
        {"transition", "date from to counts"},
        {"reproductive", "virus_id virus source source_exposure_date rt"},
        {"generation", "virus source source_exposure_date gentime"},
        {"active_cases", "date virus_id virus active_cases"},
        {"outbreak_size", "date virus_id virus outbreak_size"},
        {"hospitalizations", "date virus_id tool_id count weight"}
    };

    const auto read = [](const std::string & fn) {
        std::ifstream file(fn);
        return std::string(std::istreambuf_iterator< char >(file), {});
    };

    const std::string dir = epi_temp_file("40a-savers").directory + "/";
    auto legacy = make_save_run<int>(
        dir + "legacy-%zu",
        true, true, true, true, true, true, true, true, true, true, true, true
    );
    SaverFiles<> files(dir + "files-%zu", all);
    SaverMemory<> memory(all);
    memory.begin(3);

    // Every way of writing a simulation gives the same files
    model.run_multiple(10, 3, 123, [&](size_t sim_id, Model<> * m) {

        const std::string id = std::to_string(sim_id);
        const auto fn = [&](const char * name) {
            return dir + "db-" + id + "_" + name + ".csv";
        };

        m->write_data(
            fn("virus_info"), fn("virus_hist"), fn("tool_info"),
            fn("tool_hist"), fn("total_hist"), fn("transmission"),
            fn("transition"), fn("reproductive"), fn("generation"),
            fn("active_cases"), fn("outbreak_size"), fn("hospitalizations")
        );
        legacy(sim_id, m);
        auto outputs = files.extract(sim_id, *m);
        files.write(sim_id, RunOutputs(outputs));

        RunOutputs::for_each_table([&](const char * name, auto, auto table) {

            const std::string expected = read(fn(name));
            const std::string suffix = id + "_" + name + ".csv";
            REQUIRE(read(dir + "legacy-" + suffix) == expected);
            REQUIRE(read(dir + "files-" + suffix) == expected);

            std::ostringstream written;
            (outputs.*table).write(written);
            REQUIRE(written.str() == expected);

            REQUIRE(expected.substr(0, expected.find('\n')) == thread + headers.at(name));
            REQUIRE((outputs.*table).size() > 0u);
            REQUIRE((outputs.*table).sim_id == std::vector< int >((outputs.*table).size(), static_cast<int>(sim_id)));

        });

        memory.write(sim_id, std::move(outputs));

    }, true, false, 1);

    // Results are concatenated in simulation order
    const auto results = memory.results();
    REQUIRE(results.total_hist.size() == 3u * 11u * 4u);
    REQUIRE(results.total_hist.sim_id.front() == 0);
    REQUIRE(results.total_hist.sim_id.back() == 2);

    // The saver overload gives the same results; begin() clears the old ones
    model.run_multiple(10, 3, 123, memory, true, false);
    REQUIRE(memory.results() == results);
    REQUIRE(memory.take_results() == results);
    REQUIRE(memory.results().total_hist.size() == 0u);

}
