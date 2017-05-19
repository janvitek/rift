#pragma once

#include <string>
#include <sstream>

using namespace std;

/** Convert values to string as long as they support the ostream << operator.  */
#define STR(WHAT) static_cast<stringstream&&>(stringstream() << WHAT).str()

/** Set to true for additional debug print. */
extern bool DEBUG;




