## NET-P1-003 Observed Failures and Recommended Fixes

### Test Commands
```bash
gcc -Icore/services/netstack/src -Icore/lib/packet/include -Iinterface/include -Iinterface/uapi -Icore/lib/runtime/include quality/tests/test_netstack_boundary.c core/services/netstack/src/ipv4.c core/services/netstack/src/udp.c core/services/netstack/src/icmp.c core/services/netstack/src/netbuf.c core/services/netstack/src/checksum.c core/services/netstack/src/socket_table.c core/lib/packet/src/packet.c -o test_boundary
./test_boundary
```

### Observed Failures in `udp.c`
1. **UDP length smaller than 8 bytes** (`test_udp_len_smaller_than_8`)
   - **Defect:** `udp_rx` checks if `netbuf_len(nb) < sizeof(udphdr_t)` but does not check if the parsed `udp_len` from the header is less than `sizeof(udphdr_t)` (8 bytes). This can lead to integer underflows later or accepting malformed packets.
   - **Observed:** The test currently outputs `XFAIL: test_udp_len_smaller_than_8 observed defect.` because `udp_rx` mistakenly accepts the packet instead of returning `-1`.

2. **UDP length smaller than the available payload** (`test_udp_len_smaller_than_payload`)
   - **Defect:** When a UDP packet has a length field smaller than the actual buffer data (e.g., ethernet padding or concatenated data), `udp_rx` does not trim the `netbuf` to match `udp_len`. It passes the full `netbuf_len(nb)` to the socket callback, giving the application garbage data at the end.
   - **Observed:** The test outputs `XFAIL: test_udp_len_smaller_than_payload observed defect.` because the payload received by the callback includes the extra padding (length is 10 instead of 5).

### Recommended Fixes for `udp.c`
Propose a separate PR with the following changes to `core/services/netstack/src/udp.c`:

```c
--- core/services/netstack/src/udp.c
+++ core/services/netstack/src/udp.c
@@ -10,6 +10,10 @@
     udphdr_t *udph = (udphdr_t *)netbuf_data(nb);
     uint16_t udp_len = bnet_ntohs(udph->len);

+    if (udp_len < sizeof(udphdr_t)) {
+        return -1; // Invalid length
+    }
+
     if (netbuf_len(nb) < udp_len) {
         return -1; // Truncated UDP packet
     }
@@ -29,6 +33,11 @@
     uint16_t src_port = bnet_ntohs(udph->source);
     uint16_t dst_port = bnet_ntohs(udph->dest);

+    // Trim any padding beyond the udp_len
+    if (netbuf_len(nb) > udp_len) {
+        nb->tail -= (netbuf_len(nb) - udp_len);
+    }
+
     netbuf_pull(nb, sizeof(udphdr_t));

     socket_t *sock = socket_lookup(dst_ip, dst_port);
```

### CMake Configuration Issues
The CMake configuration for the host tests is currently broken on the `developer` branch.

**Command Run:**
```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
```
*(and subsequently direct cmake commands to build the host tests specifically)*

**First Meaningful Error:**
```
CMake Error at quality/tests/host/CMakeLists.txt:526 (add_executable):
  Cannot find source file:

    /app/core/tests/host/drivers/test_spi_registry.c
```
This error, along with dozens of similar missing file errors (e.g., `test_spi_transfer_mock.c`, `test_drm_registry.c`), indicates that the host-test `CMakeLists.txt` is attempting to compile source files that are genuinely absent from the repository in the current `developer` branch. This points to stale or improperly migrated CMake configuration rather than the wrong CMake entry point.
