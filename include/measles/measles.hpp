#ifndef MEASLES_HPP
#define MEASLES_HPP

/* Versioning */
#define MEASLES_VERSION_MAJOR 0
#define MEASLES_VERSION_MINOR 0
#define MEASLES_VERSION_PATCH 1

#define MEASLES_VERSION_PRERELEASE "dev"

static constexpr int measles_version_major = MEASLES_VERSION_MAJOR;
static constexpr int measles_version_minor = MEASLES_VERSION_MINOR;
static constexpr int measles_version_patch = MEASLES_VERSION_PATCH;
static constexpr std::string_view measles_version_prerelease =
    MEASLES_VERSION_PRERELEASE;

inline std::string measles_version() {
    std::string v =
        std::to_string(measles_version_major) + "." +
        std::to_string(measles_version_minor) + "." +
        std::to_string(measles_version_patch);

    if (!measles_version_prerelease.empty()) {
        v += "-";
        v += measles_version_prerelease;
    }

    return v;
}

namespace measles {

    // Tools and interventions
    #include "immunoglobulin.hpp"
    #include "interventionmeaslespep-meat.hpp"

    // Models
    #include "measlesschool.hpp"
    #include "measlesmixing.hpp"
    #include "measlesmixingriskquarantine.hpp"
}

#endif