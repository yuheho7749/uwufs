#ifndef c_api_h
#define c_api_h


#ifndef create_instance
#define create_instance(typename, ...) \
    new typename(__VA_ARGS__)
#endif

#ifndef destroy_instance
// don't call free() on the instance
#define destroy_instance(instance) \
    delete instance
#endif

#ifndef call_method
#define call_method(methodname, instance, ...) \
    instance->methodname(__VA_ARGS__)
#endif


#include "cpp/INode.h"
#include "cpp/DataBlockIterator.h"


#endif