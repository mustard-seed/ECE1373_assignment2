#ifndef TYPES_HPP
#define TYPES_HPP
#ifdef HW_ONLY
#include "ap_fixed.h"

typedef ap_fixed<16, 8, AP_RND_CONV, AP_SAT> t_conv;
typedef ap_fixed<16, 8, AP_RND_CONV, AP_SAT> t_fc;
typedef ap_fixed<16, 8, AP_RND_CONV, AP_SAT> t_pool;
#else
typedef float t_conv;
typedef float t_fc;
typedef float t_pool;
#endif



#endif // TYPES_HPP
