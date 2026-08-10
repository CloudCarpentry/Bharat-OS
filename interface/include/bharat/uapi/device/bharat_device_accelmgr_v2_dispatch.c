// Dispatch stub

#define OP_GETBROKERINFO 1
#define OP_GETDEVICECOUNT 2
#define OP_GETDEVICEINFO 3
#define OP_REQUESTADMISSION 4
#define OP_RELEASECONTEXT 5
#define OP_CREATEQUEUE 6
#define OP_DESTROYQUEUE 7
#define OP_REGISTERBUFFER 8
#define OP_UNREGISTERBUFFER 9
#define OP_SUBMITJOB 10
#define OP_CANCELJOB 11
#define OP_QUERYJOB 12
#define OP_QUERYFENCE 13
#define OP_QUERYHEALTH 14
#define OP_SETTHERMALCONSTRAINT 15
#define OP_SETSAFEMODE 16
#define OP_GETTELEMETRYSNAPSHOT 17

int dispatch(uint16_t opcode) {
    switch(opcode) {
    case OP_GETBROKERINFO:
        // TODO: call GetBrokerInfo
        break;
    case OP_GETDEVICECOUNT:
        // TODO: call GetDeviceCount
        break;
    case OP_GETDEVICEINFO:
        // TODO: call GetDeviceInfo
        break;
    case OP_REQUESTADMISSION:
        // TODO: call RequestAdmission
        break;
    case OP_RELEASECONTEXT:
        // TODO: call ReleaseContext
        break;
    case OP_CREATEQUEUE:
        // TODO: call CreateQueue
        break;
    case OP_DESTROYQUEUE:
        // TODO: call DestroyQueue
        break;
    case OP_REGISTERBUFFER:
        // TODO: call RegisterBuffer
        break;
    case OP_UNREGISTERBUFFER:
        // TODO: call UnregisterBuffer
        break;
    case OP_SUBMITJOB:
        // TODO: call SubmitJob
        break;
    case OP_CANCELJOB:
        // TODO: call CancelJob
        break;
    case OP_QUERYJOB:
        // TODO: call QueryJob
        break;
    case OP_QUERYFENCE:
        // TODO: call QueryFence
        break;
    case OP_QUERYHEALTH:
        // TODO: call QueryHealth
        break;
    case OP_SETTHERMALCONSTRAINT:
        // TODO: call SetThermalConstraint
        break;
    case OP_SETSAFEMODE:
        // TODO: call SetSafeMode
        break;
    case OP_GETTELEMETRYSNAPSHOT:
        // TODO: call GetTelemetrySnapshot
        break;
    }
    return 0;
}
