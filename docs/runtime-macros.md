# roBa runtime macro 0.5.0 candidate

Unassigned USB-configured text macro behavior added to the existing roBa_R Studio demo. This is an experimental candidate; no firmware flashing, real runtime-macro save, reboot-retention or physical input test has been performed by this development task. User approval is required before installing it.

## Scope and compatibility

Keep official ZMK `acfd8e5ea76cf23ad1c9b6b99848f97a95224257` and PMW3610 `5e04553ab803d24405bd45621a41310ea3050e59`, the exact revisions used by the previous demo. Backport only Studio Custom RPC/build/linker/buffer changes from cormoran/ZMK `b227a48deb420a3caf97ad523a4b661e81112744`. Protocol messages are pinned to `89b81d2e587fce807b668dff2a6967a40beef421`. See `patches/README.md` and the reviewable patch. No fork BLE/split/event-manager code is imported.

BLE HID and split BLE remain enabled; PMW3610 configuration and existing source key bindings remain unchanged. **Studio configuration over BLE is disabled**: macro configuration is USB UART only. Existing BLE pairing/settings partitions and settings-table behavior IDs are retained; no reset code or reset UF2 is introduced. Initial startup can register the new behavior in the existing ID table.

Existing runtime NVS settings, rather than compiled keymap defaults alone, contain the user's current layout. Keep a private USB keymap backup before installation. Do not publish that backup or private macro text in this repository or CI.

## Comparison and limits

Research: [DYA Studio](https://github.com/cormoran/dya-studio) `0bec72f2c9b624c132cbcf88f166d199e973f5c3`; [cormoran runtime macro](https://github.com/cormoran/zmk-feature-runtime-macro) `38530962eb7061b60efcb98e84585cfb1f0cb6de`; [qtrnmr runtime config](https://github.com/qtrnmr/zmk-module-runtime-config) `76703f306052a69be869f55d98a518ab92efa020`; [official Studio](https://zmk.dev/docs/features/studio).

The complete current modules require Custom Studio fork integration and include evolving pooled-step editing or unrelated runtime behaviors. This narrow module uses the fixed-slot/per-slot NVS pattern and normal ZMK behavior APIs, with a whole-record compare-and-swap protocol. It does not import runtime hold-tap, combo or pointer configuration.

8 slots, 64 printable ASCII characters each, US/JIS mappings. UTF-8 names up to 63 bytes. Wait 0–1000ms, tap 1–1000ms, maximum 30 seconds per macro. No truncation, arbitrary behavior sequences or non-ASCII text. One immutable playback at a time, repeated presses while busy ignored. Normal `&kp` press/release on the low-priority work queue, no global macro-queue expansion.

The generic behavior has parameters slot (0–7) and owner token (1–2147483647). Updates preserve UUID/token. Reusing a deleted slot requires a new token so old key assignments cannot trigger the replacement macro. The app obtains the real behavior ID and exact metadata before allowing assignment.

## Persistence and RPC

Custom subsystem `roba__runtime_macro_v1`; protocol version 1 with typed info/read/commit. Header `RS`, version, op, slot and reserved zeros. Commit supplies expected revision and a canonical 168-byte record: magic, next revision, owner token, UUID, layout, name/text lengths, tombstone, wait/tap, 64B text, 64B name, CRC32. All bounds and reserved bytes are checked. Revision exhaustion, duplicate UUID, changing the token of the same macro, overwriting an active different macro, and stale commits fail.

Settings prefix `roba_runtime_v1/<slot>/<bank>` with two alternating CRC-protected banks per slot. Total live record payload 2688B plus NVS overhead. Persist the inactive bank before updating RAM. Boot selects the highest valid generation; torn invalid records do not replace the prior bank. Settings write errors return failure; a lost response is uncertain and must be followed by a read, never an automatic retry. CRC detects accidental corruption, not malicious access.

The app requires a fresh full inventory, pre-backups, explicit native user approval, post-approval state check, one slot commit, response check, all-slot and all-301-key reread, and post-backups. Generic MCP cannot write custom payloads or approve a transaction. Macro assignment uses the existing separately approved keymap transaction.

Macro contents are plaintext. Tombstones and history are not secure erasure. Never store passwords, recovery codes or private keys. Dynamic slots are empty in the public firmware; personal content is never passed to Actions. No OpenAI API or other paid API is required.

## Build and validation

`tests/runtime/run.py` compiles the exact C core and tests every 169 interrupted-write boundary, full-record corruption detection, stale revisions and reboot selection. The app tests compare all 190 US/JIS printable keycodes and run simulated UI/approval/backup/regression scenarios. These do not establish actual hardware behavior.

Right-only artifact: `roBa_R-studio-runtime-0.5.0.uf2`. Successful code build `f7898df89cd659cee6c8754bace535e468b142e2`, [run 40276790287](https://github.com/ryohei-hamada/zmk-config-roBa/actions/runs/40276790287). FLASH 263836B/788KB; RAM 71748B/256KB. SHA-256 `d8c8d9b0417f828832f627924fc6836f6068cf8be939ec840c491f5e9b55629a`. UF2 nRF52840 blocks cover 0x27000 through 0x67700 exclusive, leaving settings 0xec000–0xf4000 and bootloader untouched. Build success does not prove BLE, trackball, MIDI or input success.

After user-approved installation, compare all existing 301 bindings and their behavior IDs/metadata against the private preflight backup; new unassigned behavior metadata can change the whole snapshot hash. Verify BLE/pairing, split, home-row modifiers, pointer/scroll/clicks, function/page keys and MIDI before saving a runtime macro. Then separately approve one fictional macro save, one selected key assignment, physical input, reboot retention, edit/delete/restore tests. Stop on an unexpected change.

## Rollback

Preserve the prior working right-side demo UF2 and the private keymap/library/history backups. Prior UF2 SHA-256 `ceb86c10d09178cedfed4d007f216564ce57feeb88b6da8b05dac77b8989b496`. Stable source branches and 0.4.1 app are not replaced by this branch.

If reverting after assigning dynamic macros, first restore those keys to their backed-up bindings via the existing approved keymap flow when possible. Old firmware lacks the dynamic behavior. If communication is broken, install the prior right UF2 only after approval, read again, and review/apply the exact affected-key recovery diff. A UF2 alone is not a backup of current runtime settings. Never automatically reset NVS, erase pairings or discard settings. Keep dynamic history for future recovery; it is harmlessly ignored by old firmware.
