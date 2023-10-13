#include "LanSessionID.h"
#include "LanEnums.h"
using namespace server_baby;

MemTLS<LanSessionIDSet>* LanSessionIDSet::idSetPool_ = new MemTLS<LanSessionIDSet>(500, 1, eLAN_SESSION_ID_SET_POOL_CODE);
