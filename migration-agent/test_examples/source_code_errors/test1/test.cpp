#include <string>
#include <stdexcept>
#include "test.h"

opc_status SyncDBOpc::GetRightStatus(std::string str_status)
{
    opc_status status;
    switch (stoi(str_status))
    {
    case 0:
        status = OPC_STATUS_OK;
        break;
    case 1:
        status = OPC_STATUS_WARNING;
        break;
    case 2:
        status = OPC_STATUS_ERROR;
        break;
    default:
        status = OPC_STATUS_UNKNOWN;
        break;
    }
}
