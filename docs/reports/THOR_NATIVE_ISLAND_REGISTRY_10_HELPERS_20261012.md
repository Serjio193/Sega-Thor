# Native island registry metadata — ten verified helpers

This is a metadata-only extension of the nine-helper registry preparation. It preserves the existing gate, adapter, and fallback design. No dispatch was implemented or activated.

The tenth entry is `0x009D70`, restricted to direct caller `0x009144` with transfer kind `BSR.W`, contract `M68K_009D70_CALLER_009144_V1`, adapter `apply_subtraction_bound`, and 22 replay captures plus four canonical-ROM synthetic branch cases. Its enable flag defaults false. Global verification remains NO and runtime replacement remains inactive.

The machine-readable registry is `THOR_NATIVE_ISLAND_REGISTRY_10_HELPERS_20261012.json`; the prior nine-helper snapshot remains unchanged.
