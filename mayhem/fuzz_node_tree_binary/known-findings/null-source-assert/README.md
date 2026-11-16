# Binary node stream with a missing source asserts at generation time

Same root cause as `fuzz_encoded_tree/known-findings/null-source-assert`: the decoder accepts a node
whose required source link is unset and `Gen*` asserts (`Generator.inl:64`, `simdGeneratorPtr`).

Replay: `/mayhem/mayhem/out/fuzz_node_tree_binary-standalone input`
