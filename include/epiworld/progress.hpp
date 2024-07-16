#ifndef EPIWORLD_PROGRESS_HPP
#define EPIWORLD_PROGRESS_HPP

#ifndef EPIWORLD_PROGRESS_BAR_WIDTH
#define EPIWORLD_PROGRESS_BAR_WIDTH 80
#endif

class Progress {
protected:
    int    width;     ///< Total width size (number of bars)
    int    n;         ///< Total number of iterations
    epiworld_double step_size; ///< Size of the step
    int last_loc;     ///< Last location of the bar
    int cur_loc;      ///< Last location of the bar
    int i;            ///< Current iteration step
    bool done;        ///< If the progress bar has finished.

public:
    Progress() {};
    Progress(int n_, int width_);
    ~Progress() {};

    void start();
    void next();
    void end();

    int get_total();
    int get_current();
    bool is_done();
};

inline Progress::Progress(int n_, int width_) {


    if (n_ < 0)
        throw std::invalid_argument("n must be greater or equal than 0.");

    if (width_ <= 0)
        throw std::invalid_argument("width must be greater than 0");

    width     = std::max(7, width_ - 7);
    n         = n_;
    step_size = n == 0? width : static_cast<epiworld_double>(width)/
        static_cast<epiworld_double>(n);
    last_loc  = 0;
    i         = 0;

}

#ifdef __wasm__
inline void Progress::start()
{

	done = false;

}

inline void Progress::next()
{

	if (i == 0)
        start();

    cur_loc = std::floor((++i) * step_size);

    if (i >= n)
        end();

    last_loc = cur_loc;

}

inline void Progress::stop()
{

	done = true;

}
#else
inline void Progress::start()
{

	done = false;

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

    cur_loc = std::floor((++i) * step_size);

    #ifndef EPI_DEBUG
    for (int j = 0; j < (cur_loc - last_loc); ++j)
    {
        printf_epiworld("|");
    }
    #endif

    if (i >= n)
        end();

    last_loc = cur_loc;

}

inline void Progress::end() {

	done = true;

    #ifndef EPI_DEBUG
    printf_epiworld(" done.\n");
    #endif

}
#endif /* __wasm__ */

inline int Progress::get_total() {

	return n;

}

inline int Progress::get_current()
{

	return i;

}

inline bool Progress::is_done()
{

	return done;

}

#endif
