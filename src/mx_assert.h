#ifdef MX_DEBUG

#include <cassert>
#define MX_ASSERT(x) assert(x)

#else//MX_DEBUG

#define MX_ASSERT(x)

#endif//MX_DEBUG