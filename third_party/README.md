# DAPLink sources

Vendored / submodule checkout of [ARMmbed/DAPLink](https://github.com/ARMmbed/DAPLink).

This project expects the **develop** branch (GCC 12+/13 syscall stubs and linker flags).

```bash
git submodule update --init --recursive
# or:
git clone --branch develop --depth 1 https://github.com/ARMmbed/DAPLink.git third_party/DAPLink
```
