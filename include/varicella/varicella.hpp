#ifndef VARICELLA_HPP
#define VARICELLA_HPP

/* Versioning */
#define VARICELLA_VERSION_MAJOR 0
#define VARICELLA_VERSION_MINOR 1
#define VARICELLA_VERSION_PATCH 0

#define VARICELLA_VERSION_PRERELEASE ""

static constexpr int varicella_version_major = VARICELLA_VERSION_MAJOR;
static constexpr int varicella_version_minor = VARICELLA_VERSION_MINOR;
static constexpr int varicella_version_patch = VARICELLA_VERSION_PATCH;
static constexpr std::string_view varicella_version_prerelease =
    VARICELLA_VERSION_PRERELEASE;

inline std::string varicella_version() {
    std::string v =
        std::to_string(varicella_version_major) + "." +
        std::to_string(varicella_version_minor) + "." +
        std::to_string(varicella_version_patch);

    if (!varicella_version_prerelease.empty()) {
        v += "-";
        v += varicella_version_prerelease;
    }

    return v;
}

namespace varicella {

    // Models
    #include "varicellaschool.hpp"

}

#endif
