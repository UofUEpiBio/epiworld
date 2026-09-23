#include "tests.hpp"

using namespace epiworld;

// Whole-model behaviour of the transmission modes.
//
// 1. Pushing, pulling, and the automatic choice give the same epidemics (in
//    distribution) for the built-in SEIR and for a custom SEIRH whose
//    susceptibles ignore exposed, hospitalized, and recovered neighbors.
// 2. Queuing stays a pure optimisation in every mode: on and off give
//    identical runs, even with ties added and removed mid-run.
// 3. Only the default susceptible samplers are pushed: a custom function
//    always pulls -- and gives exactly the pulled run.
// 4. In a dense, high-prevalence epidemic, "auto" uses both steps.

namespace {

struct Moments {
    double sum = 0.0, sum2 = 0.0;
    int n = 0;
    void add(double x) { sum += x; sum2 += x * x; ++n; }
    double mean() const { return sum / n; }
    double var() const { double m = mean(); return (sum2 - n * m * m) / (n - 1); }
};

struct Outcomes {
    Moments final_size, peak, incidence_10;
};

// Final size, peak prevalence (all but the first and last state), and
// infections in the first ten days, over many replicates.
Outcomes simulate(Model<> & model, int ndays, int nsims)
{

    Outcomes out;
    auto saver = [&out](size_t, Model<> * m) -> void {

        const size_t ns = m->get_n_states();

        std::vector< int > today;
        m->get_db().get_today_total(nullptr, &today);
        out.final_size.add(static_cast< double >(m->size() - today[0u]));

        std::vector< int > counts;
        m->get_db().get_hist_total(nullptr, nullptr, &counts);
        double peak = 0.0;
        for (size_t k = 0u; k < counts.size(); k += ns)
        {
            double prevalence = 0.0;
            for (size_t s = 1u; s + 1u < ns; ++s)
                prevalence += counts[k + s];
            peak = std::max(peak, prevalence);
        }
        out.peak.add(peak);

        std::vector< int > date, source, target, virus, sexp;
        m->get_db().get_transmissions(date, source, target, virus, sexp);
        double inc = 0.0;
        for (auto d : date)
            if ((d > 0) && (d <= 10))
                inc += 1.0;
        out.incidence_10.add(inc);

    };

    model.verbose_off();
    model.run_multiple(ndays, nsims, 2024, saver, true, false, 1);
    return out;

}

void check_same(const Moments & a, const Moments & b, const std::string & what)
{
    double z = (a.mean() - b.mean()) / std::sqrt(a.var() / a.n + b.var() / b.n);
    INFO(what << ": " << a.mean() << " vs " << b.mean() << " (z = " << z << ")");
    CHECK(std::abs(z) <= 5.0);
}

// A custom SEIRH: exposed, hospitalized, and recovered agents keep the virus
// but do not transmit (the epiworld-benchmark model).
void build_seirh(Model<> & model)
{
    model.add_param(0.06, "Transmission rate");
    model.add_param(0.25, "Incubation rate");
    model.add_param(0.02, "Hospitalization rate");
    model.add_param(0.15, "Recovery rate");
    model.add_param(0.15, "Hospital recovery rate");

    model.add_state("Susceptible", sampler::make_update_susceptible<>({1u, 3u, 4u}));
    model.add_state("Exposed", new_state_update_transition<>({"Incubation rate"}, {2u}));
    model.add_state("Infected", new_state_update_transition<>(
        {"Hospitalization rate", "Recovery rate"}, {3u, 4u}
    ));
    model.add_state("Hospitalized", new_state_update_transition<>(
        {"Hospital recovery rate"}, {4u}
    ));
    model.add_state("Recovered");

    Virus<> v("pathogen", 0.02, true);
    v.set_state(1, 4, 4);
    v.set_prob_infecting("Transmission rate");
    model.add_virus(v);
}

// Adds and removes a fixed set of ties on a schedule (no random numbers)
void rewire_some_ties(Model<> * m)
{
    int day = m->today();
    if ((day % 10) == 5)
        for (size_t i = 0u; i < 40u; ++i)
            m->add_edge(i, i + 700u);
    else if ((day % 10) == 8)
        for (size_t i = 0u; i < 40u; ++i)
            m->rm_edge(i, i + 700u);
}

struct Run {
    std::vector< int > counts, date, source, target;
    bool operator==(const Run & o) const {
        return (counts == o.counts) && (date == o.date) &&
            (source == o.source) && (target == o.target);
    }
};

Run record(Model<> & m)
{
    Run r;
    m.get_db().get_hist_total(nullptr, nullptr, &r.counts);
    std::vector< int > virus, sexp;
    m.get_db().get_transmissions(r.date, r.source, r.target, virus, sexp);
    return r;
}

} // namespace

EPIWORLD_TEST_CASE("Transmission - modes agree on whole models", "[transmission]") {

    const int nsims = 300;

    // 1. Same epidemics in every mode ------------------------------------------
    {
        std::map< std::string, Outcomes > seir, seirh;
        for (auto mode : {"pull", "push", "auto"})
        {
            epimodels::ModelSEIR<> m_seir("flu", 0.01, 0.04, 4.0, 1.0 / 7.0);
            m_seir.seed(11);
            m_seir.agents_smallworld(1500, 8, false, 0.1);
            m_seir.set_transmission_mode(mode);
            seir[mode] = simulate(m_seir, 60, nsims);

            Model<> m_seirh;
            build_seirh(m_seirh);
            m_seirh.seed(11);
            m_seirh.agents_smallworld(1500, 8, false, 0.1);
            m_seirh.set_transmission_mode(mode);
            seirh[mode] = simulate(m_seirh, 60, nsims);
        }

        for (auto mode : {"push", "auto"})
        {
            std::string m(mode);
            check_same(seir[m].final_size, seir["pull"].final_size, "SEIR final size, " + m);
            check_same(seir[m].peak, seir["pull"].peak, "SEIR peak, " + m);
            check_same(seir[m].incidence_10, seir["pull"].incidence_10, "SEIR day-10 incidence, " + m);
            check_same(seirh[m].final_size, seirh["pull"].final_size, "SEIRH final size, " + m);
            check_same(seirh[m].peak, seirh["pull"].peak, "SEIRH peak, " + m);
            check_same(seirh[m].incidence_10, seirh["pull"].incidence_10, "SEIRH day-10 incidence, " + m);
        }

        // The outbreaks are neither trivial nor saturated
        REQUIRE(seir["pull"].final_size.mean() > 100.0);
        REQUIRE(seir["pull"].final_size.mean() < 1400.0);
        REQUIRE(seirh["pull"].final_size.mean() > 100.0);
        REQUIRE(seirh["pull"].final_size.mean() < 1400.0);
    }

    // 2. Queuing on == queuing off, in every mode ------------------------------
    for (auto mode : {"push", "auto", "pull"})
    {
        std::vector< Run > runs;
        for (bool queuing : {true, false})
        {
            epimodels::ModelSIR<> model("flu", 0.02, 0.9, 0.3);
            model.seed(3131);
            model.agents_smallworld(2000, 6, false, 0.02);
            if (!queuing)
                model.queuing_off();
            model.add_globalevent(rewire_some_ties, "rewire some ties");
            model.set_transmission_mode(mode);
            model.verbose_off();
            model.run(60, 55);
            runs.push_back(record(model));
        }

        INFO("mode " << mode);
        REQUIRE(runs[0].date.size() > 100u); // An outbreak happened
        REQUIRE(runs[0] == runs[1]);
    }

    // 3. Custom functions pull ------------------------------------------------
    {
        auto sir = [](bool wrapped) {
            Model<> m;
            if (wrapped)
                m.add_state("Susceptible", [](Agent<> * p, Model<> * mm) -> void {
                    default_update_susceptible<>(p, mm);
                });
            else
                m.add_state("Susceptible", default_update_susceptible<>);
            m.add_state("Infected", default_update_exposed<>);
            m.add_state("Recovered");
            Virus<> v("flu", 0.02, true);
            v.set_state(1, 2, 2);
            v.set_prob_infecting(0.3);
            v.set_prob_recovery(0.2);
            m.add_virus(v);
            m.seed(99);
            m.agents_smallworld(1000, 6, false, 0.05);
            m.verbose_off();
            return m;
        };

        Model<> plain_pull = sir(false);
        plain_pull.set_transmission_mode("pull").run(40, 7);

        Model<> wrapped_push = sir(true);
        wrapped_push.set_transmission_mode("push").run(40, 7);
        REQUIRE(wrapped_push.get_last_transmission_mode() == TransmissionMode::pull);
        REQUIRE(record(wrapped_push).date.size() > 50u);
        REQUIRE(record(wrapped_push) == record(plain_pull));

        // ... while the plain function does get pushed
        Model<> plain_push = sir(false);
        plain_push.set_transmission_mode("push").run(40, 7);
        REQUIRE(plain_push.get_last_transmission_mode() == TransmissionMode::push);
    }

    // 4. "auto" switches in a dense, high-prevalence epidemic -------------------
    {
        epimodels::ModelSIR<> model("flu", 0.01, 0.05, 0.2);
        model.seed(5);
        model.agents_smallworld(2000, 50, false, 0.1);
        model.verbose_off();

        int n_push = 0, n_pull = 0;
        model.add_globalevent([&n_push, &n_pull](Model<> * m) -> void {
            if (m->get_last_transmission_mode() == TransmissionMode::push)
                ++n_push;
            else
                ++n_pull;
        }, "count modes");

        model.run(100, 3);
        INFO("push steps " << n_push << ", pull steps " << n_pull);
        REQUIRE(n_push > 0);
        REQUIRE(n_pull > 0);
    }

    // 5. The mode API ----------------------------------------------------------
    {
        Model<> m;
        REQUIRE(m.get_transmission_mode() == TransmissionMode::automatic);
        m.set_transmission_mode("push");
        REQUIRE(m.get_transmission_mode() == TransmissionMode::push);
        m.set_transmission_mode(TransmissionMode::pull);
        REQUIRE(m.get_transmission_mode() == TransmissionMode::pull);
        REQUIRE_THROWS_AS(m.set_transmission_mode("sideways"), std::invalid_argument);
        REQUIRE_THROWS_AS(m.set_transmission_mode("auto", -1.0), std::range_error);
        m.set_transmission_mode("auto", 0.5);
        REQUIRE(m.get_transmission_kappa() == 0.5);
        m.set_transmission_mode(TransmissionMode::pull);
        REQUIRE(m.get_transmission_kappa() == EPI_DEFAULT_TRANSMISSION_KAPPA);

        // Copies keep the mode (run_multiple runs copies on other threads)
        Model<> copy(m);
        REQUIRE(copy.get_transmission_mode() == TransmissionMode::pull);
    }

}
