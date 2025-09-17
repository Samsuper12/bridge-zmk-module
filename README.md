# ZMK Bridge module

A [ZMK](https://zmk.dev) module that enables dynamic keyboard configuration and extended features through a custom communication protocol, configured via bridge-cli.

---

## Features
- Dynamic keyboard configuration at runtime
- Configurable via [bridge-cli](https://github.com/Samsuper12/zmk-bridge-cli)
- Easy extensible through additional modules
---

## Installation

Add the module to your `config/west.yml`:
```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: Samsuper12                                   # <--- add this
      url-base: https://github.com/bridge-zmk-module     # <--- and this
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: bridge-zmk-module                     # <--- and these
      remote: Samsuper12                          # <---
      revision: main                              # <---
  self:
    path: config
```
At last, add the `bridge-usb-uart` snippet to `build.yaml`
```yaml
include:
# Example
  - board: example_board
    shield: example_shield_dongle
    cmake-args: -DCONFIG_ZMK_BRIDGE=y             # <--- add this
    snippet: bridge-usb-uart                      # <--- and this
```

---
