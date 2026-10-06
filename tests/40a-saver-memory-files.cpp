#include "tests.hpp"

using namespace epiworld;

// Splits a line of a saved csv on spaces, outside of double quotes (which
// are dropped).
static std::vector< std::string > split_csv_line(const std::string & line)
{
    std::vector< std::string > tokens(1u);
    bool quoted = false;
    for (char c : line)
    {
        if (c == '"')
            quoted = !quoted;
        else if (c == ' ' && !quoted)
            tokens.emplace_back();
        else
            tokens.back() += c;
    }
    return tokens;
}

EPIWORLD_TEST_CASE("SaverMemory matches the saved files", "[saver-memory]") {

    epimodels::ModelSEIRCONN<> model("a virus", 500, 0.1, 3.0, 0.3, 4.0, 0.2);

    Tool<> tool("a tool");
    tool.set_susceptibility_reduction(.5);
    tool.set_distribution(distribute_tool_randomly<>(0.3, false));
    model.add_tool(tool);
    model.verbose_off();

    const auto & whats = run_output_names();
    const size_t nsims = 3u;

    // Same seed, same model: once to memory, once to files
    SaverMemory saver(whats);
    model.run_multiple(30, nsims, 123, saver, true, false, 1);

    auto fn = epi_temp_file("40a-saver-memory-files", "%03lu");
    model.run_multiple(
        30, nsims, 123,
        make_save_run<>(
            fn.full_path, true, true, true, true, true, true, true, true,
            true, true, true, true
        ),
        true, false, 1
    );

    auto results = saver.results();
    REQUIRE(results.size() == whats.size());

    for (const auto & what : whats)
    {

        const auto & table = results.at(what);
        REQUIRE(table.colnames.at(0u) == "sim_id");

        for (size_t sim = 0u; sim < nsims; ++sim)
        {

            char fname[1024u];
            snprintf(fname, sizeof(fname), (fn.full_path + "_" + what + ".csv").c_str(), sim);
            std::ifstream file(fname);
            REQUIRE(file.good());

            std::string line;
            std::getline(file, line);
            auto header = split_csv_line(line);

            #ifdef EPI_DEBUG
            header.erase(header.begin()); // The thread column
            #endif

            // Same columns as the file
            REQUIRE(header.size() + 1u == table.colnames.size());
            for (size_t j = 0u; j < header.size(); ++j)
                REQUIRE(header[j] == table.colnames[j + 1u]);

            // Same rows as the file, in the same order
            size_t row = 0u;
            for (; std::getline(file, line); ++row)
            {

                auto cells = split_csv_line(line);

                #ifdef EPI_DEBUG
                cells.erase(cells.begin());
                #endif

                // Moving to the next row of this simulation
                while (
                    row < table.nrow() &&
                    std::get< std::vector< int > >(table.columns[0u])[row] != static_cast<int>(sim)
                )
                    ++row;

                REQUIRE(row < table.nrow());
                for (size_t j = 0u; j < cells.size(); ++j)
                {
                    std::ostringstream cell;
                    std::visit(
                        [&](const auto & col) { cell << col[row]; },
                        table.columns[j + 1u]
                    );
                    REQUIRE(cells[j] == cell.str());
                }

            }

        }

    }

    // Something was actually compared
    REQUIRE(results.at("total_hist").nrow() > 0u);
    REQUIRE(results.at("transmission").nrow() > 0u);
    REQUIRE(results.at("tool_info").nrow() > 0u);

    REQUIRE_THROWS_AS(
        model.get_db().get_run_outputs({"not_an_output"}),
        std::invalid_argument
    );

}
