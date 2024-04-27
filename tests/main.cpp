#define CATCH_CONFIG_MAIN
#include "../include/catch2/catch.hpp"

#define epiworld_double double
#include "../include/epiworld/epiworld.hpp"
#include "tests.hpp"


#include "00-cloning-model.cpp"
#include "00-lfmcmc.cpp"
#include "01-sir.cpp"
#include "01b-sir.cpp"
#include "01c-sir.cpp"
#include "01-sirconnected.cpp"
#include "02-reproducible-sir.cpp"
#include "02-reproducible-sirconn.cpp"
#include "04-initial-dist.cpp"
#include "05-mixing.cpp"