// Dispatch stub

#define OP_VALIDATE 1
#define OP_CREATE 2
#define OP_REVOKE 3
#define OP_TRANSFER 4
#define OP_INSPECT 5

int dispatch(uint16_t opcode) {
    switch(opcode) {
    case OP_VALIDATE:
        // TODO: call Validate
        break;
    case OP_CREATE:
        // TODO: call Create
        break;
    case OP_REVOKE:
        // TODO: call Revoke
        break;
    case OP_TRANSFER:
        // TODO: call Transfer
        break;
    case OP_INSPECT:
        // TODO: call Inspect
        break;
    }
    return 0;
}
