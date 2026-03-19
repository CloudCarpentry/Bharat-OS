// Dispatch stub

#define OP_REMOTEGRANT 1
#define OP_REMOTEREVOKE 2

int dispatch(uint16_t opcode) {
    switch(opcode) {
    case OP_REMOTEGRANT:
        // TODO: call RemoteGrant
        break;
    case OP_REMOTEREVOKE:
        // TODO: call RemoteRevoke
        break;
    }
    return 0;
}
