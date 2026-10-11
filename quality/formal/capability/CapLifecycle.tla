--------------------------- MODULE CapLifecycle ---------------------------
EXTENDS Integers, FiniteSets, TLC

CONSTANTS
    CORES,
    SLOTS,
    MAX_GENERATION

VARIABLES
    cspace,     \* cspace[core][slot] = [valid, rights, generation, parent_core, parent_slot]
    epoch,      \* current revocation epoch
    revoked     \* revoked[epoch] = set of revoked slots (simplified)

Init ==
    /\ cspace = [c \in 1..CORES |-> [s \in 1..SLOTS |-> [valid |-> FALSE, rights |-> 0, generation |-> 0, parent_core |-> 0, parent_slot |-> 0]]]
    /\ epoch = 0
    /\ revoked = [e \in 0..10 |-> {}]

Mint(core, slot, rights) ==
    /\ \neg cspace[core][slot].valid
    /\ cspace' = [cspace EXCEPT ![core][slot] = [valid |-> TRUE, rights |-> rights, generation |-> 1, parent_core |-> 0, parent_slot |-> 0]]
    /\ UNCHANGED <<epoch, revoked>>

Delegate(src_core, src_slot, dst_core, dst_slot, atten_rights) ==
    /\ cspace[src_core][src_slot].valid
    /\ \neg cspace[dst_core][dst_slot].valid
    /\ atten_rights <= cspace[src_core][src_slot].rights  \* C1: authority granted to a child is subset of parent
    /\ cspace' = [cspace EXCEPT ![dst_core][dst_slot] = [valid |-> TRUE, rights |-> atten_rights, generation |-> 1, parent_core |-> src_core, parent_slot |-> src_slot]]
    /\ UNCHANGED <<epoch, revoked>>

Revoke(core, slot) ==
    /\ cspace[core][slot].valid
    /\ cspace' = [cspace EXCEPT ![core][slot].valid = FALSE, ![core][slot].generation = cspace[core][slot].generation + 1]
    /\ epoch' = epoch + 1
    /\ revoked' = [revoked EXCEPT ![epoch'] = revoked[epoch] \cup {<<core, slot>>}]

Next ==
    \E c \in 1..CORES, s \in 1..SLOTS:
       \/ \E r \in 1..3: Mint(c, s, r)
       \/ \E dc \in 1..CORES, ds \in 1..SLOTS, ar \in 1..3: Delegate(c, s, dc, ds, ar)
       \/ Revoke(c, s)

NoEscalation ==
    \A c \in 1..CORES, s \in 1..SLOTS:
        (cspace[c][s].valid /\ cspace[c][s].parent_core /= 0) =>
            cspace[c][s].rights <= cspace[cspace[c][s].parent_core][cspace[c][s].parent_slot].rights

Spec == Init /\ [][Next]_<<cspace, epoch, revoked>>

=============================================================================