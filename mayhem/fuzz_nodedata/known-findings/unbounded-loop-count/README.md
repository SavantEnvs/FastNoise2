# Integer node parameters are unbounded loop counts: one 8x8 grid takes minutes

`input` is a 32-byte encoded tree. The decoder accepts any `int` for the `PowInt` node's exponent
(`Blends.inl`, `GenT` multiplies in a loop of that length per sample), so a tiny input hangs
`GenUniformGrid2D` (a DoS; libFuzzer reports `timeout`). The same pattern shows up with a fractal's
octave count (`Fractal.inl`), which the fuzzer also drove to hangs.

Fix: clamp these counts when they are set or deserialised.

Replay: `timeout 10 /mayhem/mayhem/out/fuzz_nodedata-standalone input` (exits 124).
