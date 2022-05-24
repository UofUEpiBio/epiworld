#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

////////////////////////////////////////////////////////////////////////////////
/**
 * DETAILS OF THE MODEL
 * 
 * # ROWS
 * ====================
 * 
 * ## Optimistic waning of protection against infection
 * 
 * - Slow immune waning, median transition time to partially immune state =
 *  10 months
 * 
 * - In the partially immune state, there is a 40% reduciton in protection from
 *  baseline levels reported immediately after exposure (vaccination or
 *  infection.)
 * 
 * ## Pessimistic waning of protection against infection
 * 
 * - Fast immune waning, median transition time to partially immune state =
 *  4 months.
 * 
 * - In the partially immune state, there is a 60% reduction in protection from
 *  baseline levels reported immediately after exposure (vaccination or
 *  infection.)
 * 
 * # COLUMNS
 * ====================
 * 
 * ## No new variant
 * 
 * - Projections are initialized with the mix of strains circulating at the
 *  start of the projection period.
 * 
 * - New variant X emerges on May 1st, 2022. There is a continuous influx of 50
 *  weekly infections of variant X for the following 16 wks. Variant X has
 *  30% immune escape, and the same intrinsic transmissibility and severity as
 *  Omicron.
 */

int x_weekly_counter   = 0;
int x_initial_date     = 14;

EPI_NEW_MUTFUN(variant_x_mut, int)
{

    // The virus originates in a single individual
    if ((m->today() >= x_initial_date) & (x_weekly_counter < 50))
    {

        // Starting to move things around
        x_weekly_counter++;

        v.set_sequence(1111); // To make the sequence different
        v.set_name("Variant X");

        return true;
    }

    return false;
}

// The counters restart everytime we reach the end of the week
EPI_NEW_GLOBALFUN(reset_x_counter, int)
{

    // After the date, we reset the weekly counter
    if (((m->today() - x_initial_date) % 7) == 0)
        x_weekly_counter = 0u;

}

EPI_NEW_UPDATEFUN(update_infected, int)
{

    epiworld::VirusPtr<int> & v = p->get_virus(0);
    
    // Probability of infection
    m->array_double_tmp[0u] = p->get_recovery_enhancer(v);
    m->array_double_tmp[1u] = m->par("Infection Prob");
    
    int which = epiworld::roulette(2, m);

    // Becomes recovered
    if (which == 0)
        p->rm_virus(v);
    else // Becomes infected
        p->change_status(2);

    // Nothing happens
    return;
    
}

EPI_NEW_UPDATEFUN(update_susceptible, int)
{

    size_t tmpcount = 0;
    for (const auto & n : p->get_neighbors())
    {
        for (const auto & v: n->get_viruses())
        {

            if (*v->get_sequence() != 1111)
            {

                m->array_double_tmp[tmpcount] = v->get_prob_infecting();
                m->array_virus_tmp[tmpcount++] = &(*v);

            } else if (x_weekly_counter < 50)
            {
                x_weekly_counter++;
                p->add_virus(v);
                return;
            }

            
        }
    }

    int which = epiworld::roulette(tmpcount, m);

    if (which < 0)
        return;

    p->add_virus(*m->array_virus_tmp[which]);

}

// Designing variants ------------------------------------------------------
int main()
{   

    epiworld::Model<> model;
    model.add_status("Susceptible", update_susceptible);
    model.add_status("Exposed", update_infected);
    model.add_status("Infected", epiworld::default_update_exposed<>);
    model.add_status("Recovered", epiworld::default_update_susceptible<>);
    model.add_status("Removed");

    model.add_param(14, "Emergence date");   // will be recorded as p0
    model.add_param(0, "Weekly count");      // will be recorded as p1
    model.add_param(0, "Total left");        // will be recorded as p2
    model.add_param(.4, "Omicron Immunity"); // will be recorded as p3
    model.add_param(.3, "Variant X scaped immunity"); // will be recorded as p4
    model.add_param(.2, "Infection Prob");

    // Delta
    epiworld::Virus<> variant_delta("Delta");
    variant_delta.set_status(1,3,4);
    variant_delta.set_prob_recovery(.3);
    model.add_virus(variant_delta, .025);

    // Omicron
    epiworld::Virus<> variant_omicron("Omicron");
    variant_omicron.set_status(1,3,4);
    variant_omicron.set_prob_recovery(.3);

    variant_omicron.set_mutation(variant_x_mut);

    model.add_virus(variant_omicron, .05);

    // Adding the reset    
    model.add_global_action(reset_x_counter);

    // Setup
    model.population_smallworld(10000);

    // Initializing and running the model
    model.init(100, 226);
    model.run();

    // Printing the output
    model.print();

    model.write_data(
        "variant_info.txt", "variant_hist.txt",
        "tool_info.txt", "tool_hist.txt",
        "total_hist.txt", "transmission.txt", "transition.txt"
    );

    return 0;

}