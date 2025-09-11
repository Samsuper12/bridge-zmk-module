# ZMK Bridge module

A [ZMK](https://zmk.dev) module that enables dynamic keyboard configuration and extended features through a custom communication protocol, configured via bridge-cli.

---

## Features
- Dynamic keyboard configuration at runtime
- Custom communication protocol for extended flexibility
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

And add the collect-proto job to your `.github/workflows/build.yml`:
```yaml
name: Build ZMK firmware
on: [push, pull_request, workflow_dispatch]

jobs:
  build:
    uses: zmkfirmware/zmk/.github/workflows/build-user-config.yml@main
  
  bridge:                                                                         # <--- add these
    uses: Samsuper12/bridge-zmk-module/.github/workflows/collect-proto.yml@main   # 

```

---
