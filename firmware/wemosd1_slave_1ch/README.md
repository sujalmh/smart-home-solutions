# DEPRECATED — Merged into Unified Slave Firmware

This 1-channel variant has been merged into the unified parameterised firmware at:

    ../wemosd1_slave/wemosd1_slave.ino

To build for a **1-channel** relay board, set `NUM_CHANNELS` to `1` in the
configuration block at the top of that file:

```cpp
#define NUM_CHANNELS 1
```

The original source is preserved in this directory for reference only.
Do **not** use it for new deployments.
