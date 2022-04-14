#ifndef EPIWORLD_MODEL_MEAT_VISITED_HPP
#define EPIWORLD_MODEL_MEAT_VISITED_HPP

template<typename TSeq>
inline void Model<TSeq>::print() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    // Prints a message if debugging is on
    EPI_DEBUG_NOTIFY_ACTIVE()

    printf_epiworld("\n%s\n%s\n\n",line.c_str(), "SIMULATION STUDY");
    printf_epiworld("Population size    : %i\n", static_cast<int>(size()));
    printf_epiworld("Days (duration)    : %i (of %i)\n", today(), ndays);
    printf_epiworld("Number of variants : %i\n", static_cast<int>(db.get_nvariants()));
    if (n_replicates > 0u)
    {
        std::string abbr;
        epiworld_double elapsed;
        epiworld_double total;
        get_elapsed("auto", &elapsed, &total, &abbr, false);
        printf_epiworld("Last run elapsed t : %.2f%s\n", elapsed, abbr.c_str());
        if (n_replicates > 1u)
        {
            printf_epiworld("Total elapsed t    : %.2f%s (%i runs)\n", total, abbr.c_str(), n_replicates);
        }

    } else {
        printf_epiworld("Last run elapsed t : -\n");
    }
    
    if (rewire_fun)
    {
        printf_epiworld("Rewiring           : on (%.2f)\n\n", rewire_prop);
    } else {
        printf_epiworld("Rewiring           : off\n\n");
    }

    printf_epiworld("Virus(es):\n");
    int i = 0;
    for (auto & v : viruses)
    {    

        if (prevalence_virus_as_proportion[i])
        {

            printf_epiworld(
                " - %s (baseline prevalence: %.2f%%)\n",
                v.get_name().c_str(),
                prevalence_virus[i++] * 100.00
            );

        }
        else
        {

            printf_epiworld(
                " - %s (baseline prevalence: %i seeds)\n",
                v.get_name().c_str(),
                static_cast<int>(prevalence_virus[i++])
            );

        }

    }

    printf_epiworld("Tool(s):\n");
    i = 0;
    for (auto & t : tools)
    {   

        if (prevalence_tool_as_proportion[i])
        {

            printf_epiworld(
                " - %s (baseline prevalence: %.2f%%)\n",
                t.get_name().c_str(),
                prevalence_tool[i++] * 100.0
                );

        }
        else
        {

            printf_epiworld(
                " - %s (baseline prevalence: %i seeds)\n",
                t.get_name().c_str(),
                static_cast<int>(prevalence_tool[i++])
                );

        }
        

    }

    // Information about the parameters included
    printf_epiworld("\nModel parameters:\n");
    unsigned int nchar = 0u;
    for (auto & p : parameters)
        if (p.first.length() > nchar)
            nchar = p.first.length();

    std::string fmt = " - %-" + std::to_string(nchar + 1) + "s: ";
    for (auto & p : parameters)
    {
        std::string fmt_tmp = fmt;
        if (std::fabs(p.second) < 0.0001)
            fmt_tmp += "%.1e\n";
        else
            fmt_tmp += "%.4f\n";

        printf_epiworld(
            fmt_tmp.c_str(),
            p.first.c_str(),
            p.second
        );
        
    }


    nchar = 0u;
    for (auto & p : status_susceptible_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    for (auto & p : status_exposed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    for (auto & p : status_removed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    if (initialized) 
    {
        
        if (today() != 0)
            fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %7i -> %i\n";
        else
            fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %i\n";

    }
    else
        fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %s\n";
        
    printf_epiworld("\nDistribution of the population at time %i:\n", today());
    for (unsigned int s = 0u; s < status_susceptible.size(); ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {

                printf_epiworld(
                    fmt.c_str(),
                    (status_susceptible_labels[s] + " (S)").c_str(),
                    db.hist_total_counts[status_susceptible[s]],
                    db.today_total[ status_susceptible[s] ]
                    );

            }
            else
            {

                printf_epiworld(
                    fmt.c_str(),
                    (status_susceptible_labels[s] + " (S)").c_str(),
                    db.today_total[ status_susceptible[s] ]
                    );

            }
            

        }
        else
        {

            printf_epiworld(
                fmt.c_str(),
                (status_susceptible_labels[s] + " (S)").c_str(),
                " - "
                );

        }
    }

    for (unsigned int s = 0u; s < status_exposed.size(); ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_exposed_labels[s] + " (E)").c_str(),
                    db.hist_total_counts[ status_exposed[s] ],
                    db.today_total[ status_exposed[s] ]
                    );
            }
            else
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_exposed_labels[s] + " (E)").c_str(),
                    db.today_total[ status_exposed[s] ]
                    );
            }
            

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_exposed_labels[s] + " (E)").c_str(),
                " - "
                );
        }
    }

    // printf_epiworld("\nStatistics (removed):\n");
    for (unsigned int s = 0u; s < status_removed.size(); ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_removed_labels[s] + " (R)").c_str(),
                    db.hist_total_counts[ status_removed[s] ],
                    db.today_total[ status_removed[s] ]
                    );
            }
            else
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_removed_labels[s] + " (R)").c_str(),
                    db.today_total[ status_removed[s] ]
                    );
            }
            

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_removed_labels[s] + " (R)").c_str(),
                " - "
                );
        }
    }
    
    printf_epiworld(
        "\n(S): Susceptible, (E): Exposed, (R): Removed\n%s\n\n",
        line.c_str()
        );

    return;

}

#endif