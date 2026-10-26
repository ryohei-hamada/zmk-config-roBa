# Custom RPC backport

Official ZMK base: acfd8e5ea76cf23ad1c9b6b99848f97a95224257 (same as the previous demo).
Custom RPC source: cormoran/zmk b227a48deb420a3caf97ad523a4b661e81112744, MIT.
Only Studio protocol/build/linker and transport buffer fixes are imported.
No fork BLE, split, event-manager or advertising changes are imported.
Studio messages: 89b81d2e587fce807b668dff2a6967a40beef421, same custom schema already used by roBa Studio.
PMW3610 remains at the previous build's exact revision 5e04553ab803d24405bd45621a41310ea3050e59.
The local macro module is a narrow text-only implementation informed by qtrnmr's per-slot NVS pattern, using ZMK's normal behavior APIs. It does not import runtime hold-tap, trackball, conditional-layer, combo or monitoring features.
