# Left encoder MIDI candidate — no hardware installation

Starting remote HEAD: `5e159a5f8750722bfa4976ae5bc69d957ddc8735`, based on `feat/runtime-macros` (`de83276`). The existing commit changes only the default-layer sensor binding to `&inc_dec_kp F13 F14`. It is retained, not reapplied. App companion: `ryohei-hamada/roba-studio`, branch `feat/encoder-midi-integration`, stacked on the 0.5.1 macro-management PR.

## Source path and conditions

The manifest pins official ZMK `acfd8e5ea76cf23ad1c9b6b99848f97a95224257`; the existing Custom RPC patch does not replace the split/sensor implementation. PMW3610 remains `5e04553ab803d24405bd45621a41310ea3050e59`.

* `boards/shields/roBa/roBa.dtsi`: sensor list is left then right (indices 0, 1). `roBa_L.overlay` enables the left EC11 with existing pins/steps; `roBa_L.conf` enables EC11 and its global trigger thread. The right encoder is disabled. These are unchanged.
* `Kconfig.defconfig`: right is central, left peripheral. Both use split support.
* Pinned ZMK `app/src/sensors.c` samples rotation and raises `zmk_sensor_event`; `app/src/split/peripheral.c` subscribes under `ZMK_KEYMAP_HAS_SENSORS` and reports sensor index plus channel data.
* `app/src/split/bluetooth/service.c` exposes the sensor characteristic and queues BLE notifications; `central.c` receives them. `app/src/split/central.c` reconstructs the event. `app/src/keymap.c` resolves `zmk_sensor_keymap[layer][sensor_index]`; sensor-rotate behavior produces key press/release via the central HID endpoint.
* Thus a compatible left build can forward sensor 0 to the right; changing its key binding normally requires only the right build. Stock/default layer is the changed layer. Other active layers, physical A/B wiring and actual detents must be tested; clockwise→F13 is a candidate assumption, not measured hardware behavior.

If the installed left firmware lacks enabled EC11, a valid sensors node, the peripheral sensor-event subscriber or the matching BLE characteristic/protocol, right-only installation cannot create events that the left never sends. The installed left build is not identified here. No evidence currently establishes that a left update is required. If later established, report exact revision/protocol differences, effect on split/pairing/settings and a known-good **left** recovery UF2 before requesting separate approval. Never try settings_reset or bond deletion to diagnose it.

Sources: [pinned peripheral listener](https://github.com/zmkfirmware/zmk/blob/acfd8e5ea76cf23ad1c9b6b99848f97a95224257/app/src/split/peripheral.c), [pinned BLE service](https://github.com/zmkfirmware/zmk/blob/acfd8e5ea76cf23ad1c9b6b99848f97a95224257/app/src/split/bluetooth/service.c), [encoder configuration](https://zmk.dev/docs/hardware-integration/encoders).

## Build and compatibility

The existing workflow builds **only** `seeeduino_xiao_ble / roBa_R` with USB Studio. This change gives its artifact the distinct name `roBa_R-encoder-midi-0.6.0.uf2`. It creates no left or reset image. Firmware logic added in this work: none. Existing runtime macro tests still run. This branch retains all 301 normal bindings, home row mods, Layer 3 pointing, 800 CPI, BLE HID/split, settings partitions and macro implementation from the runtime base. Source compatibility and build success do not establish hardware/settings retention.

## Approval boundary and staged hardware checks

No UF2 is installed, no settings reset/bond deletion is performed, and no real F13/F14 is generated. F13 currently conflicts with the user's macOS Sync Keybindings service (external Gist synchronization). Resolve that shortcut separately after review, before any encoder/keyboard input test. Preserve Hammerspoon F19 and Ableton input-source functions. The app candidate includes an **uninstalled** MIDI-only ownership guard and starts off.

Before a future separately approved right update, preserve a fresh 301-binding/behavior inventory, all dynamic macro slot records and backups, plus the known-good right UF2. After update re-read/compare without restoring stock settings. Check CW/CCW first, then CC21/ch1 increments, Decay movement, Arrangement automation recording and typing/macros/trackball/BLE. Record each individually. A successful build is not a passed hardware test.

Recovery: stop app MIDI, use the prior app/Hammerspoon configuration as applicable; for a firmware regression install the known-good **right** UF2 only after approval. Do not reset settings, restore stock keymaps or flash the left. Recompare identities/binding meanings before any approved restore.
