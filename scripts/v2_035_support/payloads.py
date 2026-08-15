"""Deterministic and exact-size gzip payload construction."""

from __future__ import annotations

import gzip
import hashlib
import zlib

from .core import BLOB_MAX_BYTES, EvidenceError, require


def deterministic_bytes(label: str, length: int) -> bytes:
    output = bytearray()
    counter = 0
    while len(output) < length:
        output.extend(hashlib.sha256(f"{label}:{counter}".encode()).digest())
        counter += 1
    return bytes(output[:length])


def gzip_member(payload: bytes, extra_length: int = 0) -> bytes:
    if extra_length < 0 or extra_length > 65_535:
        raise EvidenceError("gzip extra field length is out of range")
    flags = 0x04 if extra_length else 0x00
    header = bytearray(b"\x1f\x8b\x08")
    header.extend(bytes((flags, 0, 0, 0, 0, 0, 255)))
    if extra_length:
        header.extend(extra_length.to_bytes(2, "little"))
        header.extend(b"V" * extra_length)
    compressor = zlib.compressobj(level=0, wbits=-15)
    deflated = compressor.compress(payload) + compressor.flush()
    trailer = zlib.crc32(payload).to_bytes(4, "little")
    trailer += (len(payload) & 0xFFFFFFFF).to_bytes(4, "little")
    return bytes(header) + deflated + trailer


def exact_gzip_payload(label: str, target_length: int = BLOB_MAX_BYTES) -> bytes:
    if target_length < 128:
        raise EvidenceError("target gzip length is too small")
    first = gzip_member(deterministic_bytes(label, target_length - 64))
    remainder = target_length - len(first)
    require(remainder >= 25, "target leaves too little room for a padding gzip member")
    padding = gzip_member(b"", extra_length=remainder - 25)
    candidate = first + padding
    require(len(candidate) == target_length, "exact-size gzip construction drifted")
    gzip.decompress(candidate)
    return candidate


def small_payload(label: str) -> bytes:
    return gzip.compress(deterministic_bytes(label, 4096), compresslevel=6, mtime=0)

