# Decoded node tree with a missing source asserts at generation time

`input` is a 4-byte encoded node tree. `NewFromEncodedNodeTree` accepts it and returns a node whose
required generator source is unset. The first `Gen*` call then fails
`Assertion 'memberVariable.simdGeneratorPtr' failed` (`Generator.inl:64`, `GetSourceValue`; a null
dereference with asserts off, `Generator.inl:73` `GetSourceSIMD`).

Impact: any untrusted encoded tree crashes the process once it is evaluated.
Fix: have `NewFromEncodedNodeTree` / the deserialiser reject a tree whose `MemberNodeLookup` links are null.

Replay: `/mayhem/mayhem/out/fuzz_encoded_tree-standalone input`
