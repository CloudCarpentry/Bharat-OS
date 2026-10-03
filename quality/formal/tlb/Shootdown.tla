--------------------------- MODULE Shootdown ---------------------------
EXTENDS Integers, FiniteSets, TLC

CONSTANTS
    CORES,
    PAGES

VARIABLES
    tlb,        \* tlb[core] = set of cached pages
    pending_sd, \* pending_sd[core] = set of pages requested for shootdown
    acked_sd,   \* acked_sd[core] = set of cores that acked the shootdown for a page
    reclaimed   \* reclaimed = set of reclaimed pages

Init ==
    /\ tlb = [c \in 1..CORES |-> {}]
    /\ pending_sd = [c \in 1..CORES |-> {}]
    /\ acked_sd = [c \in 1..CORES |-> [p \in 1..PAGES |-> {}]]
    /\ reclaimed = {}

MapPage(core, page) ==
    /\ page \notin reclaimed
    /\ tlb' = [tlb EXCEPT ![core] = tlb[core] \cup {page}]
    /\ UNCHANGED <<pending_sd, acked_sd, reclaimed>>

RequestShootdown(core, page) ==
    /\ pending_sd' = [pending_sd EXCEPT ![core] = pending_sd[core] \cup {page}]
    /\ acked_sd' = [acked_sd EXCEPT ![core][page] = {}]
    /\ UNCHANGED <<tlb, reclaimed>>

AckShootdown(ack_core, req_core, page) ==
    /\ page \in pending_sd[req_core]
    /\ tlb' = [tlb EXCEPT ![ack_core] = tlb[ack_core] \ {page}]
    /\ acked_sd' = [acked_sd EXCEPT ![req_core][page] = acked_sd[req_core][page] \cup {ack_core}]
    /\ UNCHANGED <<pending_sd, reclaimed>>

ReclaimPage(core, page) ==
    /\ page \in pending_sd[core]
    /\ acked_sd[core][page] = (1..CORES)
    /\ pending_sd' = [pending_sd EXCEPT ![core] = pending_sd[core] \ {page}]
    /\ reclaimed' = reclaimed \cup {page}
    /\ UNCHANGED <<tlb, acked_sd>>

Next ==
    \E c \in 1..CORES, p \in 1..PAGES:
       \/ MapPage(c, p)
       \/ RequestShootdown(c, p)
       \/ \E ac \in 1..CORES: AckShootdown(ac, c, p)
       \/ ReclaimPage(c, p)

NoPrematureReclaim ==
    \A p \in 1..PAGES:
        (p \in reclaimed) => (\A c \in 1..CORES: p \notin tlb[c])

Spec == Init /\ [][Next]_<<tlb, pending_sd, acked_sd, reclaimed>>

=============================================================================