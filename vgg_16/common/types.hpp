#ifndef TYPES_HPP
#define TYPES_HPP
#ifdef HW_ONLY
//#include "hls_half.h"
////
//typedef half t_conv;
//typedef half t_fc;
//typedef half t_pool;

//#include "ap_fixed.h"
//
//typedef ap_fixed<24, 10, AP_RND_CONV, AP_SAT> t_conv;
//typedef ap_fixed<24, 10, AP_RND_CONV, AP_SAT> t_fc;
//typedef ap_fixed<24, 10, AP_RND_CONV, AP_SAT> t_pool;

typedef float t_conv;
typedef float t_fc;
typedef float t_pool;

#else
typedef float t_conv;
typedef float t_fc;
typedef float t_pool;
#endif



#endif // TYPES_HPP
