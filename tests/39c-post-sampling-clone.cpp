#include "tests.hpp"

using namespace epiworld;

// Copies of a model keep the callback, but not the scratch: cloning and
// run_multiple() give each replicate its own batches, and the results do not
// depend on the number of threads.

namespace {

std::vector< size_t > deliveries(int nthreads, size_t nsims)
{

    epimodels::ModelSEIR<> model("Virus", 0.02, 0.5, 3.0, 0.2);
    model.agents_smallworld(1000, 6, false, 0.1);
    model.verbose_off();

    // One slot per replicate: replicates never share a slot
    std::vector< size_t > per_sim(nsims, 0u);
    auto * slots = &per_sim;

    model.set_post_sampling(
        [slots](Agent<> *, const SampledContactsView & c, Model<> * m) -> void {
            (*slots)[m->get_sim_id()] += c.size();
        }
    );

    model.run_multiple(30, nsims, 331, nullptr, true, false, nthreads);

    return per_sim;

}

}

EPIWORLD_TEST_CASE("Post-sampling callback - clones and run_multiple", "[post-sampling-clone]") {

    // A clone keeps the callback; removing it there leaves the original alone
    epimodels::ModelSEIR<> model("Virus", 0.02, 0.5, 3.0, 0.2);
    model.agents_smallworld(500, 6, false, 0.1);
    model.verbose_off();

    size_t original = 0u, copied = 0u;
    model.set_post_sampling(
        [&original](Agent<> *, const SampledContactsView & c, Model<> *) -> void {
            original += c.size();
        }
    );

    auto clone = std::make_unique< epimodels::ModelSEIR<> >(model);
    REQUIRE(clone->has_post_sampling());

    clone->set_post_sampling(
        [&copied](Agent<> *, const SampledContactsView & c, Model<> *) -> void {
            copied += c.size();
        }
    );

    model.run(20, 77);
    clone->run(20, 77);

    // Same model, same seed, each with its own callback and scratch
    REQUIRE(original > 0u);
    REQUIRE(original == copied);

    clone->clear_post_sampling();
    REQUIRE_FALSE(clone->has_post_sampling());
    REQUIRE(model.has_post_sampling());

    // Replicates: every one delivers, and they differ from each other
    const size_t nsims = 8u;
    auto one = deliveries(1, nsims);
    auto two = deliveries(2, nsims);

    for (auto n : one)
        REQUIRE(n > 0u);

    REQUIRE(std::adjacent_find(one.begin(), one.end(), std::not_equal_to<size_t>()) != one.end());

    // Same seed, same replicates, whatever the number of threads
    REQUIRE(one == two);

}
