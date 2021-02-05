/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_RN_FPDEC_H
#define RATIONAL_RN_FPDEC_H

// Large unsigned int
#ifdef __SIZEOF_INT128__
#include "uint128_math_native.h"
#else
#include "uint128_math.h"
#endif // __int128

#endif //RATIONAL_RN_FPDEC_H
