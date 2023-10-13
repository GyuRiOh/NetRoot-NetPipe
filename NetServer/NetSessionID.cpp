#include "NetSessionID.h"
#include "NetEnums.h"
using namespace server_baby;

MemTLS<NetSessionIDSet>* NetSessionIDSet::idSetPool_ = new MemTLS<NetSessionIDSet>(100, 1, eSESSION_ID_SET_POOL_CODE);
