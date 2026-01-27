#ifndef EPIWORLD_PROGRESS_HPP
#define EPIWORLD_PROGRESS_HPP

#include "epiworld/config.hpp"

/**
 * @brief A simple progress bar
  */
class Progress {
private:
    int    width;     ///< Total width size (number of bars)
    int    n;         ///< Total number of iterations
    epiworld_double step_size; ///< Size of the step
    int last_loc;     ///< Last location of the bar
    int cur_loc;      ///< Last location of the bar
    int i;            ///< Current iteration step

public:
    Progress() = default;
    Progress(const Progress &) = default;
    Progress(Progress &&) = delete;
    Progress(int n_, int width_);
    ~Progress() = default;
    
    Progress& operator=(const Progress& other) {
        if (this != &other) {
            width = other.width;
            n = other.n;
            step_size = other.step_size;
            last_loc = other.last_loc;
            cur_loc = other.cur_loc;
            i = other.i;
        }
        return *this;
    }

    void start();
    void next();
};

inline Progress::Progress(int n_, int width_) {
    if (n_ < 0)
        throw std::invalid_argument("n must be greater or equal than 0.");

    if (width_ <= 0)
        throw std::invalid_argument("width must be greater than 0");

    width     = std::max(EPIWORLD_PROGRESS_MIN_WIDTH, width_ - EPIWORLD_PROGRESS_MIN_WIDTH);
    n         = n_;

    step_size = n == 0 ? static_cast<epiworld_double>(width) : 
        static_cast<epiworld_double>(width) / static_cast<epiworld_double>(n);
    last_loc  = 0;
    i         = 0;

}

inline void Progress::start()
{

    #ifndef EPI_DEBUG
    for (int j = 0; j < (width); ++j)
    {
        printf_epiworld("_");
    }
    printf_epiworld("\n");
    #endif
}

inline void Progress::next() {

    if (i == 0)
        start();

    cur_loc = static_cast<int>(std::floor((static_cast<epiworld_double>(++i)) * step_size));

    #ifndef EPI_DEBUG
    for (int j = 0; j < (cur_loc - last_loc); ++j)
    { 
        printf_epiworld("|");
    }

    if (i == n)
    {
        printf_epiworld(" done.\n");
    }
    #endif

    last_loc = cur_loc;

}

#endif