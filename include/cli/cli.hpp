#ifndef EPIWORLD_CLI_HPP
#define EPIWORLD_CLI_HPP

/**
 * @file cli.hpp
 * @brief A small command-line option parser for examples and benchmarks.
 *
 * @details Options are `--key value` pairs bound to variables. Every value is
 * validated as a whole (`12abc` is an error, not 12) and every error exits
 * with a message. `--help` prints the options and their defaults. The header
 * has no dependency on epiworld.
 *
 * @code
 * int reps = 5;
 * std::vector< size_t > sizes = {10000};
 * std::vector< std::string > scenarios = {"A", "B"};
 *
 * cli::Parser args("What this program does");
 * args.add_int("--reps", reps, "Replicates per cell", 1)
 *     .add_size_list("--sizes", sizes, "Population sizes", 100)
 *     .add_list("--scenarios", scenarios, "Scenarios to run", {"A", "B", "C"})
 *     .parse(argc, argv);
 * @endcode
 *
 * A list option replaces its default the first time it is given. Values are
 * comma-separated.
 */

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace cli {

/** Prints @p msg to stderr and exits with status 1. */
[[noreturn]] inline void fail(const std::string & msg)
{
    std::fprintf(stderr, "%s\n", msg.c_str());
    std::exit(1);
}

/** Splits @p s at @p sep, dropping empty pieces. */
inline std::vector< std::string > split(const std::string & s, char sep = ',')
{
    std::vector< std::string > out;
    size_t start = 0u;
    while (start <= s.size())
    {
        size_t end = s.find(sep, start);
        if (end == std::string::npos)
            end = s.size();
        if (end > start)
            out.push_back(s.substr(start, end - start));
        start = end + 1u;
    }
    return out;
}

/** Parses the whole of @p val as an integer in [min, max]. */
inline long parse_long(
    const std::string & key, const std::string & val,
    long min = LONG_MIN, long max = LONG_MAX
)
{
    size_t used = 0u;
    long x = 0;
    try {
        x = std::stol(val, &used);
    } catch (const std::exception &) {
        used = 0u;
    }
    if (used == 0u || used != val.size())
        fail("Option " + key + " expects an integer, got '" + val + "'");
    if (x < min || x > max)
        fail(
            "Option " + key + " must be between " + std::to_string(min) +
            " and " + std::to_string(max) + ", got " + val
        );
    return x;
}

/** Parses the whole of @p val as a finite number in [min, max]. */
inline double parse_double(
    const std::string & key, const std::string & val,
    double min = -HUGE_VAL, double max = HUGE_VAL
)
{
    size_t used = 0u;
    double x = 0.0;
    try {
        x = std::stod(val, &used);
    } catch (const std::exception &) {
        used = 0u;
    }
    if (used == 0u || used != val.size() || !(x - x == 0.0))
        fail("Option " + key + " expects a number, got '" + val + "'");
    if (x < min || x > max)
        fail(
            "Option " + key + " must be between " + std::to_string(min) +
            " and " + std::to_string(max) + ", got " + val
        );
    return x;
}

/** Declares options, binds them to variables, and parses `argv`. */
class Parser
{
private:

    struct Option {
        std::string key;
        std::string help;
        std::string def; ///< The default, as text
        std::function< void(const std::string &) > set;
    };

    std::string description;
    std::vector< Option > options;

    static std::string join(const std::vector< std::string > & x)
    {
        std::string out;
        for (size_t i = 0u; i < x.size(); ++i)
            out += (i ? "," : "") + x[i];
        return out;
    }

    Parser & push(
        std::string key, std::string help, std::string def,
        std::function< void(const std::string &) > set
    )
    {
        options.push_back(
            {std::move(key), std::move(help), std::move(def), std::move(set)}
        );
        return *this;
    }

public:

    explicit Parser(std::string description_ = "") :
        description(std::move(description_)) {}

    /** An integer in [min, max]. */
    Parser & add_int(
        const std::string & key, int & target, const std::string & help,
        long min = INT_MIN, long max = INT_MAX
    )
    {
        return push(key, help, std::to_string(target),
            [&target, key, min, max](const std::string & v) {
                target = static_cast< int >(parse_long(key, v, min, max));
            });
    }

    /** A 64-bit integer in [min, max]. */
    Parser & add_long(
        const std::string & key, long & target, const std::string & help,
        long min = LONG_MIN, long max = LONG_MAX
    )
    {
        return push(key, help, std::to_string(target),
            [&target, key, min, max](const std::string & v) {
                target = parse_long(key, v, min, max);
            });
    }

    /** A number in [min, max]. */
    Parser & add_double(
        const std::string & key, double & target, const std::string & help,
        double min = -HUGE_VAL, double max = HUGE_VAL
    )
    {
        return push(key, help, std::to_string(target),
            [&target, key, min, max](const std::string & v) {
                target = parse_double(key, v, min, max);
            });
    }

    /** `on` or `off`. */
    Parser & add_onoff(
        const std::string & key, bool & target, const std::string & help
    )
    {
        return push(key, help, target ? "on" : "off",
            [&target, key](const std::string & v) {
                if (v != "on" && v != "off")
                    fail("Option " + key + " expects on or off, got '" + v + "'");
                target = (v == "on");
            });
    }

    /** A comma-separated list of non-negative integers, each at least @p min. */
    Parser & add_size_list(
        const std::string & key, std::vector< size_t > & target,
        const std::string & help, long min = 0
    )
    {
        std::string def;
        for (size_t i = 0u; i < target.size(); ++i)
            def += (i ? "," : "") + std::to_string(target[i]);

        // The default stays until the option is given
        return push(key, help, def,
            [&target, key, min](const std::string & v) {
                std::vector< size_t > out;
                for (auto & s : split(v))
                    out.push_back(static_cast< size_t >(
                        parse_long(key, s, min < 0 ? 0 : min)
                    ));
                if (out.empty())
                    fail("Option " + key + " needs at least one value");
                target = std::move(out);
            });
    }

    /**
     * A comma-separated list of words. If @p allowed is not empty, every word
     * has to be one of them.
     */
    Parser & add_list(
        const std::string & key, std::vector< std::string > & target,
        const std::string & help,
        std::vector< std::string > allowed = {}
    )
    {
        std::string h = help;
        if (!allowed.empty())
            h += " (" + join(allowed) + ")";

        return push(key, h, join(target),
            [&target, key, allowed](const std::string & v) {
                std::vector< std::string > out = split(v);
                if (out.empty())
                    fail("Option " + key + " needs at least one value");
                for (auto & w : out)
                {
                    bool ok = allowed.empty();
                    for (auto & a : allowed)
                        ok = ok || (a == w);
                    if (!ok)
                        fail("Option " + key + ": unknown value " + w);
                }
                target = std::move(out);
            });
    }

    /** A word, which has to be one of @p allowed if that is not empty. */
    Parser & add_choice(
        const std::string & key, std::string & target,
        const std::string & help,
        std::vector< std::string > allowed = {}
    )
    {
        std::string h = help;
        if (!allowed.empty())
            h += " (" + join(allowed) + ")";

        return push(key, h, target,
            [&target, key, allowed](const std::string & v) {
                bool ok = allowed.empty();
                for (auto & a : allowed)
                    ok = ok || (a == v);
                if (!ok)
                    fail("Option " + key + ": unknown value " + v);
                target = v;
            });
    }

    /** Prints the options and their defaults. */
    void print_help(const char * prog) const
    {
        std::printf("Usage: %s [options]\n", prog);
        if (!description.empty())
            std::printf("%s\n", description.c_str());
        std::printf("\nOptions (each takes a value, `--key value`):\n");
        for (auto & o : options)
            std::printf(
                "  %-14s %s [default: %s]\n",
                o.key.c_str(), o.help.c_str(), o.def.c_str()
            );
    }

    /** Parses `--key value` pairs. Exits with a message on any error. */
    void parse(int argc, char ** argv) const
    {
        for (int i = 1; i < argc; i += 2)
        {
            std::string key = argv[i];

            if (key == "--help" || key == "-h")
            {
                print_help(argv[0]);
                std::exit(0);
            }

            const Option * which = nullptr;
            for (auto & o : options)
                if (o.key == key)
                    which = &o;

            if (which == nullptr)
                fail("Unknown option " + key + " (see --help)");
            if (i + 1 >= argc)
                fail("Option " + key + " has no value");

            which->set(argv[i + 1]);
        }
    }
};

} // namespace cli

#endif
