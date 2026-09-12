"""Versioned full-hash identities using the existing Carver canonical encoding."""
import hashlib
import re

from m12_carver import canonical as _canonical

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3145728
STATE_SHA = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
SCHEMA = "thor.evidence.capture.v0.1"
STATUSES = {"OBSERVED", "UNKNOWN", "UNSUPPORTED", "CONFLICT"}


def canonical(value):
    def check(item):
        if item is None or type(item) in (str, int, bool):
            return
        if type(item) is list:
            for child in item:
                check(child)
            return
        if type(item) is dict and all(type(key) is str for key in item):
            for child in item.values():
                check(child)
            return
        raise ValueError("canonical payload requires JSON integers, not floats")
    check(value)
    return _canonical(value)


def digest(value):
    return hashlib.sha256(canonical(value).encode("utf-8")).hexdigest()


def identity(kind, value):
    return digest({"identity_schema": "thor.evidence.identity.v0", "kind": kind,
                   "value": value})


def require_hash(value):
    if not isinstance(value, str) or re.fullmatch(r"[0-9a-f]{64}", value) is None:
        raise ValueError("expected lowercase full SHA256")
    return value


def location_key(rom_hash, mapping_hash, location):
    if set(location) != {"space", "key"}:
        raise ValueError("location requires space and key")
    space, key = location["space"], location["key"]
    if space in {"M68K_BUS", "ROM_OFFSET", "RAM", "VRAM"}:
        if type(key) is not int or not 0 <= key < 0x1000000:
            raise ValueError("invalid physical/bus location")
    elif space == "REGISTER":
        if key not in {"PC", "SR", "USP", "SSP", *[f"D{i}" for i in range(8)],
                       *[f"A{i}" for i in range(8)]}:
            raise ValueError("unknown register")
    else:
        raise ValueError("unknown address space")
    return identity("location", [rom_hash, mapping_hash, location])
