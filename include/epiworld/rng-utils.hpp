#ifndef EPIWORLD_RNG_UTILS_HPP
#define EPIWORLD_RNG_UTILS_HPP

/**
 * @brief Draw a uniform [0, 1) random number from a mt19937 engine.
 *
 * Uses direct multiplication instead of std::uniform_real_distribution to
 * avoid the quad-precision floating-point promotion that occurs with some
 * compiler/stdlib combinations (e.g., Clang linked against libstdc++), which
 * can cause a severe performance regression.
 *
 * @param engine A mt19937 random engine.
 * @return epiworld_double in [0, 1).
 */
inline epiworld_double runif_mt19937(std::mt19937 & engine) {
    static constexpr epiworld_double factor =
        epiworld_double(1) / (epiworld_double(std::mt19937::max()) + epiworld_double(1));
    epiworld_double res = epiworld_double(engine()) * factor;
    if (res >= epiworld_double(1))
        res = std::nextafter(epiworld_double(1), epiworld_double(0));
    return res;
}

#endif
