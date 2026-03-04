#include "tests.hpp"

using namespace epiworld;

inline std::vector< double > counts_risk;

EPI_NEW_GLOBALFUN(count_risk, int)
{
    auto * model = dynamic_cast< epimodels::ModelMeaslesMixingRiskQuarantine<> * >(m);

    auto q_risk_today = model->get_quarantine_risk_level();

    // Counting how many are in each risk group
    std::vector< int > counts(3, 0);
    for (const auto & r : q_risk_today)
        counts[r]++;

    // Appending to global variable
    counts_risk.push_back(static_cast<double>(m->today()));
    counts_risk.push_back(static_cast<double>(counts[0]));
    counts_risk.push_back(static_cast<double>(counts[1]));
    counts_risk.push_back(static_cast<double>(counts[2]));
    
}



EPIWORLD_TEST_CASE(
    "Measles risk quarantine - quarantine process validation",
    "[ModelMeaslesMixingRiskQuarantine_quarantine]"
) {
    
    // Contact matrix for 3 groups with equal mixing
    size_t n = 100;

    // More contact within groups
    // According to Toth and others, 83% of contacts are within the same class
    // (for school-aged children). 17% are with other classes.
    double rate_self = 1.0;
    double rate_others = (1.0 - rate_self) / 2.0;
    std::vector<double> contact_matrix = {
        rate_self, rate_others, rate_others,
        rate_others, rate_self, rate_others,
        rate_others, rate_others, rate_self
    };

    double R0 = 15;
    double c_rate = 10.0;
    double p_infect = R0 / (c_rate) * (1.0/4.0);
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,           // Number of agents
        0.1 ,        // Initial prevalence
        c_rate,      // Contact rate
        p_infect,    // Transmission rate
        1.0,         // Vaccination efficacy
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        0.0,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        .9,          // Quarantine willingness
        .9,          // Isolation willingness
        4,           // Isolation period
        0.5,         // Proportion vaccinated
        0.1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Adding a single entity
    model.add_entity(Entity<>("Population", dist_factory<>(0, 1)));
    model.add_entity(Entity<>("Population", dist_factory<>(1, 2*n/3 - 1)));
    model.add_entity(Entity<>("Population", dist_factory<>(2*n/3, n)));

    // Event to count risk levels
    model.add_globalevent(count_risk, "Count risk levels");

    // Moving the virus to the first agent
    model.get_virus(0).set_distribution(
        distribute_virus_to_set<>({0u})
    );

    model.verbose_off();

    model.run(100, 233);

    // Retrieving the transmission data
    std::vector<int> date, source, target, virus, source_exposure_date;
    model.get_db().get_transmissions(
        date,
        source,
        target,
        virus,
        source_exposure_date
    );  

    // Priting the counts
    auto quarantine_days = model.get_days_quarantine_triggered();

    std::set< size_t > days_set(
        quarantine_days.begin(),
        quarantine_days.end()
        );

    std::cout << std::endl << "Day Susceptible LowRisk MidRisk HighRisk\n";
    for (size_t i = 0; i < counts_risk.size()/4; ++i)
    {
        if (days_set.find(counts_risk[4*i + 0]) != days_set.end())
            std::cout << "Quarantine: ";
        else
            continue;
        
        printf(" %3.0f ", counts_risk[4*i + 0]);
        printf(" %3.0f ", counts_risk[4*i + 1]);
        printf(" %3.0f ", counts_risk[4*i + 2]);
        printf(" %3.0f ", counts_risk[4*i + 3]);
        std::cout << std::endl;
        
    }

    std::vector< size_t > expected_first_quarantine = {100, 0, 0};

    size_t day_first_quarantine = quarantine_days[0u] - 1u;
    std::vector< size_t > observed_first_quarantine = {
        static_cast< size_t >(counts_risk[4*day_first_quarantine + 1]),
        static_cast< size_t >(counts_risk[4*day_first_quarantine + 2]),
        static_cast< size_t >(counts_risk[4*day_first_quarantine + 3])
    };

    REQUIRE(observed_first_quarantine == expected_first_quarantine);
}
