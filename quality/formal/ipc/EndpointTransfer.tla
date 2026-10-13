------------------------- MODULE EndpointTransfer -------------------------
EXTENDS Integers, FiniteSets, TLC

CONSTANTS
    CORES,
    ENDPOINTS,
    CAP_SLOTS

VARIABLES
    endpoints,  \* endpoints[e] = [valid, receiver_core]
    queues,     \* queues[e] = set of pending messages (represented by cap_slot)
    cspace      \* cspace[core][slot] = [valid, is_endpoint, ep_id]

Init ==
    /\ endpoints = [e \in 1..ENDPOINTS |-> [valid |-> FALSE, receiver_core |-> 0]]
    /\ queues = [e \in 1..ENDPOINTS |-> {}]
    /\ cspace = [c \in 1..CORES |-> [s \in 1..CAP_SLOTS |-> [valid |-> FALSE, is_endpoint |-> FALSE, ep_id |-> 0]]]

CreateEndpoint(core, ep, slot) ==
    /\ \neg endpoints[ep].valid
    /\ \neg cspace[core][slot].valid
    /\ endpoints' = [endpoints EXCEPT ![ep] = [valid |-> TRUE, receiver_core |-> core]]
    /\ cspace' = [cspace EXCEPT ![core][slot] = [valid |-> TRUE, is_endpoint |-> TRUE, ep_id |-> ep]]
    /\ UNCHANGED queues

Send(sender_core, slot, msg_slot) ==
    /\ cspace[sender_core][slot].valid
    /\ cspace[sender_core][slot].is_endpoint
    /\ cspace[sender_core][msg_slot].valid
    /\ LET ep == cspace[sender_core][slot].ep_id IN
        /\ endpoints[ep].valid
        /\ queues' = [queues EXCEPT ![ep] = queues[ep] \cup {msg_slot}]
        /\ UNCHANGED <<endpoints, cspace>>

Receive(receiver_core, ep) ==
    /\ endpoints[ep].valid
    /\ endpoints[ep].receiver_core = receiver_core
    /\ queues[ep] /= {}
    /\ \E msg \in queues[ep]:
        queues' = [queues EXCEPT ![ep] = queues[ep] \ {msg}]
    /\ UNCHANGED <<endpoints, cspace>>

RevokeEndpoint(core, slot) ==
    /\ cspace[core][slot].valid
    /\ cspace[core][slot].is_endpoint
    /\ LET ep == cspace[core][slot].ep_id IN
        /\ endpoints[ep].receiver_core = core
        /\ endpoints' = [endpoints EXCEPT ![ep].valid = FALSE]
        /\ queues' = [queues EXCEPT ![ep] = {}]
        /\ cspace' = [cspace EXCEPT ![core][slot].valid = FALSE]

Next ==
    \E c \in 1..CORES, ep \in 1..ENDPOINTS, s \in 1..CAP_SLOTS:
       \/ CreateEndpoint(c, ep, s)
       \/ \E ms \in 1..CAP_SLOTS: Send(c, s, ms)
       \/ Receive(c, ep)
       \/ RevokeEndpoint(c, s)

NoUnauthorizedSend ==
    \A c \in 1..CORES, ep \in 1..ENDPOINTS, s \in 1..CAP_SLOTS:
        (s \in queues[ep]) =>
            (\E sc \in 1..CORES, ss \in 1..CAP_SLOTS:
                cspace[sc][ss].valid /\ cspace[sc][ss].is_endpoint /\ cspace[sc][ss].ep_id = ep)

Spec == Init /\ [][Next]_<<endpoints, queues, cspace>>

=============================================================================