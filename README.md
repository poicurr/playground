# playground

CLI playground for terminal graphics, experiments, and bad ideas.

## What's here

- `include/Terminal.hpp` + platform impls — low-level ANSI cursor/color
- `include/Canvas.hpp` — **recommended** for new experiments: double-buffer + efficient batched present + live resize + dim() for trails
- `programs/bounce.cpp` — colorful bouncing orbs demo (uses Canvas)
- Other toys: digital_clock (bouncy 7seg), img_viewer (ASCII art), heatmap viz, etc.

## Quick start

```bash
cmake -B build -S . && cmake --build build -j
./build/Debug/bounce
./build/Debug/digital_clock
```

Terminal.hpp was a bit shit (stale size, flush spam, no frame concept). It's now less shit:
- getWidth/Height refresh live
- present() for end-of-frame
- much less per-op flushing
- Canvas gives you proper cell buffer + one big write

Do whatever. Have fun. ヽ(´ー｀)ノ

'⠋','⠙','⠹','⠸','⠼','⠴','⠦','⠧','⠇','⠏'
