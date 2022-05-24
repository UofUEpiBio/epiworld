#ifndef EPIWORLD_MODEL_MEAT_PRINT_HPP
#define EPIWORLD_MODEL_MEAT_PRINT_HPP

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
    printf_epiworld("Number of variants : %i\n", static_cast<int>(db.get_n_variants()));
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
                v->get_name().c_str(),
                prevalence_virus[i++] * 100.00
            );

        }
        else
        {

            printf_epiworld(
                " - %s (baseline prevalence: %i seeds)\n",
                v->get_name().c_str(),
                static_cast<int>(prevalence_virus[i++])
            );

        }

    }

    printf_epiworld("\nTool(s):\n");
    i = 0;
    for (auto & t : tools)
    {   

        if (prevalence_tool_as_proportion[i])
        {

            printf_epiworld(
                " - %s (baseline prevalence: %.2f%%)\n",
                t->get_name().c_str(),
                prevalence_tool[i++] * 100.0
                );

        }
        else
        {

            printf_epiworld(
                " - %s (baseline prevalence: %i seeds)\n",
                t->get_name().c_str(),
                static_cast<int>(prevalence_tool[i++])
                );

        }
        

    }

    if (tools.size() == 0u)
    {
        printf_epiworld(" (none)\n");
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

    if (parameters.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    nchar = 0u;
    for (auto & p : status_labels)
        if (p.length() > nchar)
            nchar = p.length();

    

    if (initialized) 
    {
        
        if (today() != 0)
            fmt = " - (%" + std::to_string(nstatus).length() +
                std::string("d) Total %-") + std::to_string(nchar) + "s : %" +
                std::to_string(std::to_string(size()).length()) + "i -> %i\n";
        else
            fmt = " - (%" + std::to_string(nstatus).length() +
                std::string("d) Total %-") + std::to_string(nchar) + "s : %i\n";

    }
    else
        fmt = " - (%" + std::to_string(nstatus).length() +
            std::string("d) Total %-") + std::to_string(nchar) + "s : %s\n";
        
    printf_epiworld("\nDistribution of the population at time %i:\n", today());
    for (size_t s = 0u; s < nstatus; ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {

                printf_epiworld(
                    fmt.c_str(),
                    s,
                    status_labels[s].c_str(),
                    db.hist_total_counts[s],
                    db.today_total[ s ]
                    );

            }
            else
            {

                printf_epiworld(
                    fmt.c_str(),
                    s,
                    status_labels[s].c_str(),
                    db.today_total[ s ]
                    );

            }
            

        }
        else
        {

            printf_epiworld(
                fmt.c_str(),
                s,
                status_labels[s].c_str(),
                " - "
                );

        }
    }

    return;

}

#endif