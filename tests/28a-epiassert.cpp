#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("EpiAssert", "[epiassert]") {

    // -----------------------------------------------------------------
    //  check_bounds – scalar
    // -----------------------------------------------------------------
    REQUIRE_NOTHROW(EpiAssert::check_bounds(5, 0, 10, "x", "test"));
    REQUIRE_NOTHROW(EpiAssert::check_bounds(0, 0, 10, "x", "test"));
    REQUIRE_NOTHROW(EpiAssert::check_bounds(10, 0, 10, "x", "test"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_bounds(-1, 0, 10, "x", "test"),
        std::range_error
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_bounds(11, 0, 10, "x", "test"),
        std::range_error
    );

    // check_bounds – floating point scalar
    REQUIRE_NOTHROW(EpiAssert::check_bounds(0.5, 0.0, 1.0, "p"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_bounds(1.5, 0.0, 1.0, "p"),
        std::range_error
    );

    // check_bounds – vector
    std::vector<double> good_vec = {0.1, 0.5, 0.9};
    std::vector<double> bad_vec  = {0.1, 1.5, 0.9};
    REQUIRE_NOTHROW(
        EpiAssert::check_bounds(good_vec, 0.0, 1.0, "probs", "test")
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_bounds(bad_vec, 0.0, 1.0, "probs", "test"),
        std::range_error
    );

    // Empty container should pass (nothing to violate)
    std::vector<int> empty_vec;
    REQUIRE_NOTHROW(EpiAssert::check_bounds(empty_vec, 0, 100, "v"));

    // check_bounds – reversed bounds should throw std::invalid_argument
    REQUIRE_THROWS_AS(
        EpiAssert::check_bounds(5, 10, 0, "x", "test"),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_bounds(good_vec, 1.0, 0.0, "probs", "test"),
        std::invalid_argument
    );

    // -----------------------------------------------------------------
    //  check_non_negative
    // -----------------------------------------------------------------
    REQUIRE_NOTHROW(EpiAssert::check_non_negative(0, "n"));
    REQUIRE_NOTHROW(EpiAssert::check_non_negative(42, "n"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_non_negative(-1, "n"),
        std::range_error
    );

    // check_non_negative – vector
    std::vector<double> nn_good = {0.0, 1.0, 100.0};
    std::vector<double> nn_bad  = {0.0, -0.01, 100.0};
    REQUIRE_NOTHROW(EpiAssert::check_non_negative(nn_good, "counts"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_non_negative(nn_bad, "counts"),
        std::range_error
    );

    // -----------------------------------------------------------------
    //  check_probability
    // -----------------------------------------------------------------
    REQUIRE_NOTHROW(EpiAssert::check_probability(0.0, "p"));
    REQUIRE_NOTHROW(EpiAssert::check_probability(1.0, "p"));
    REQUIRE_NOTHROW(EpiAssert::check_probability(0.5, "p"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_probability(-0.01, "p"),
        std::range_error
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_probability(1.01, "p"),
        std::range_error
    );

    // check_probability – vector
    std::vector<double> prob_good = {0.0, 0.5, 1.0};
    std::vector<double> prob_bad  = {0.0, 0.5, 1.1};
    REQUIRE_NOTHROW(EpiAssert::check_probability(prob_good, "probs"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_probability(prob_bad, "probs"),
        std::range_error
    );

    // -----------------------------------------------------------------
    //  check_sum
    // -----------------------------------------------------------------
    std::vector<double> sum_good = {0.3, 0.3, 0.4};
    std::vector<double> sum_bad  = {0.3, 0.3, 0.5};
    REQUIRE_NOTHROW(
        EpiAssert::check_sum(sum_good, 1.0, "probs", "test")
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_sum(sum_bad, 1.0, "probs", "test"),
        std::invalid_argument
    );

    // check_sum with negative tolerance should throw std::invalid_argument
    REQUIRE_THROWS_AS(
        EpiAssert::check_sum(sum_good, 1.0, "probs", "test", -0.01),
        std::invalid_argument
    );

    // check_sum with custom tolerance
    std::vector<double> approx_sum = {0.34, 0.33, 0.34};
    REQUIRE_NOTHROW(
        EpiAssert::check_sum(approx_sum, 1.0, "p", "test", 0.02)
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_sum(approx_sum, 1.0, "p", "test", 0.001),
        std::invalid_argument
    );

    // -----------------------------------------------------------------
    //  check_size
    // -----------------------------------------------------------------
    std::vector<int> sized = {1, 2, 3};
    REQUIRE_NOTHROW(EpiAssert::check_size(sized, 3u, "v"));
    REQUIRE_THROWS_AS(
        EpiAssert::check_size(sized, 2u, "v"),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check_size(sized, 4u, "v"),
        std::invalid_argument
    );

    // -----------------------------------------------------------------
    //  check – custom predicate
    // -----------------------------------------------------------------

    // Lambda: value must be even
    REQUIRE_NOTHROW(
        EpiAssert::check(
            4, [](int v) { return v % 2 == 0; },
            "must be even"
        )
    );
    REQUIRE_THROWS_AS(
        EpiAssert::check(
            3, [](int v) { return v % 2 == 0; },
            "must be even"
        ),
        std::invalid_argument
    );

    // Lambda on a vector: custom aggregate check
    std::vector<double> custom_vec = {0.2, 0.3, 0.5};
    REQUIRE_NOTHROW(
        EpiAssert::check(
            custom_vec,
            [](const std::vector<double> & v) {
                double s = 0.0;
                for (auto x : v) s += x;
                return std::abs(s - 1.0) < 0.01;
            },
            "probabilities must sum to 1"
        )
    );

    // -----------------------------------------------------------------
    //  Verify error messages contain useful info
    // -----------------------------------------------------------------
    try {
        EpiAssert::check_bounds(
            -5, 0, 100, "pop_size", "ModelSIR::ModelSIR"
        );
        REQUIRE(false); // should not reach here
    } catch (const std::range_error & e) {
        std::string msg = e.what();
        CHECK(msg.find("pop_size") != std::string::npos);
        CHECK(msg.find("ModelSIR::ModelSIR") != std::string::npos);
        CHECK(msg.find("-5") != std::string::npos);
    }

    // Verify without caller
    try {
        EpiAssert::check_probability(2.0, "rate");
        REQUIRE(false);
    } catch (const std::range_error & e) {
        std::string msg = e.what();
        CHECK(msg.find("rate") != std::string::npos);
        CHECK(msg.find("2") != std::string::npos);
    }

}
