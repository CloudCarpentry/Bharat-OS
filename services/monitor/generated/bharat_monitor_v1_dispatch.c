// Dispatch stub

#define OP_HEARTBEAT 1
#define OP_NODEJOIN 2
#define OP_TLBINVALIDATE 3

int dispatch(uint16_t opcode) {
    switch(opcode) {
    case OP_HEARTBEAT:
        // TODO: call Heartbeat
        break;
    case OP_NODEJOIN:
        // TODO: call NodeJoin
        break;
    case OP_TLBINVALIDATE:
        // TODO: call TlbInvalidate
        break;
    }
    return 0;
}
