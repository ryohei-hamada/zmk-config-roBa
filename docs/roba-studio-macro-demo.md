# roBa Studio macro build test

This branch adds one generated zero-parameter ZMK macro for the fictional text `name@example.com` using the host's US layout, 40 ms wait and tap times. Its 16 taps require 32 queue events, within the existing default 64. No queue-size change is needed.

The only change to the original keymap is an include for `roba-studio-macros.dtsi`. All 301 key bindings, layer names, the `to_layer_0` macro, custom hold-tap behavior, and trackball settings remain unchanged. The new macro is deliberately not assigned to a key. With Studio enabled, its revision-specific display name can be discovered and its numeric behavior ID must be read from the actual device after a user-approved firmware update.

This test matrix builds only the right central controller, with artifact name `roBa_R-studio-macro-demo`. It creates no left or settings-reset build. The operating branch's matrix is not modified by this branch until a reviewed merge, which is not performed automatically.

Building is not flashing or an input test. Do not flash without the user's approval. Back up current Studio state first: the firmware stock keymap differs from saved device bindings (including Page Up/Page Down and F1). Never use Restore Stock Settings or settings_reset to make the macro appear. Existing settings and behavior-name mappings must be retained. After a reviewed update, reread behavior IDs and compare all bindings semantically before proposing a macro assignment.

This public test contains fictional data only. A personal macro's keycodes reveal its text even if the literal text is absent from source. Do not build real personal strings in this public repository or its Actions. Use local-only files and a private/local build environment once available.
