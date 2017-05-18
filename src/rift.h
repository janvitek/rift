#ifndef RIFT_H
#define RIFT_H

#include <string>
#include <sstream>

/** Convert values to string as long as they support the ostream << operator.  */
#define STR(WHAT) static_cast<std::stringstream&&>(std::stringstream() << WHAT).str()

/** Set to true for additional debug print. */
extern bool DEBUG;

#endif
