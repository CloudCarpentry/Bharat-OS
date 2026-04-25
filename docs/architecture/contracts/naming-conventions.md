# Naming Conventions

## Naming Rules

* C files and headers use `snake_case`
* Architecture docs use `kebab-case`
* Struct types use `_t`
* Enum constants are fully prefixed
* Capability types use `CAP_TYPE_*`
* Capability rights use `CAP_RIGHT_*`
* uRPC state enums use `URPC_*`
* DMA grant state enums use `DMA_GRANT_*`
* Public core/kernel/UAPI names must be stable once merged
* Do not use synonyms for the same security concept across modules

## Canonical Terms

* service identity -> `service_id`
* endpoint generation -> `endpoint_gen`
* transaction id -> `tx_id`
* capability instance id -> `cap_instance_id`
* revocation epoch -> `revocation_epoch`
* DMA grant -> `dma_grant`
* transfer scope -> `transfer_scope`
