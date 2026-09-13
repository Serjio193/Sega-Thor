"""V5 static request/response and non-owning Carver bridge."""
from dataclasses import asdict, dataclass

from .identity import ROM_SHA, ROM_SIZE, canonical, digest, require_hash

STATUSES = {"OBSERVED", "PROVISIONAL", "UNKNOWN", "CONFLICT"}
CERTIFICATE_TYPES = {"STRUCTURE", "DOMAIN", "BOUNDARY"}


def _plain(value):
    if isinstance(value, tuple):
        return [_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _plain(item) for key, item in value.items()}
    return value


@dataclass(frozen=True)
class StaticRequest:
    id: str
    trace: str
    rom_sha256: str
    start: int
    end: int
    query_kind: str
    seed_ids: tuple
    constraints: tuple


@dataclass(frozen=True)
class StaticCertificate:
    id: str
    request_id: str
    certificate_type: str
    start: int
    end: int
    status: str
    rule_id: str
    witnesses: tuple


class StaticBridge:
    """Static evidence remains evidence-only when merged into IntervalDB."""

    def __init__(self, trace, rom_sha256=ROM_SHA, rom_size=ROM_SIZE):
        self.trace = require_hash(trace)
        if rom_sha256 != ROM_SHA or rom_size != ROM_SIZE:
            raise ValueError("static bridge requires canonical ROM identity")
        self.rom_sha256 = rom_sha256
        self.rom_size = rom_size
        self.requests = {}
        self.responses = {}
        self.merged = []

    def request(self, start, end, query_kind, seed_ids=(), constraints=()):
        if type(start) is not int or type(end) is not int or not 0 <= start < end <= self.rom_size:
            raise ValueError("static request range is invalid")
        if not query_kind:
            raise ValueError("static request kind is required")
        payload = {"trace": self.trace, "rom_sha256": self.rom_sha256,
                   "start": start, "end": end, "query_kind": query_kind,
                   "seed_ids": list(seed_ids), "constraints": list(constraints)}
        ident = digest({"kind": "thor-v5-static-request", "value": payload})
        request = StaticRequest(ident, self.trace, self.rom_sha256, start, end,
                                query_kind, tuple(seed_ids), tuple(constraints))
        self.requests[ident] = request
        return request

    def request_from_runtime(self, runtime_id, start, end, query_kind="RUNTIME_TO_STATIC"):
        return self.request(start, end, query_kind, (runtime_id,), ("runtime-witness",))

    def response(self, request, records=(), certificates=(), unresolved=()):
        request = self.requests.get(getattr(request, "id", request))
        if request is None:
            raise ValueError("unknown static request")
        normalized_records = []
        for record in records:
            item = dict(record)
            left, right = item.get("start"), item.get("end")
            if type(left) is not int or type(right) is not int or not request.start <= left < right <= request.end:
                raise ValueError("static record escapes request range")
            if item.get("source_owned") or item.get("promotion_transaction"):
                raise ValueError("static bridge cannot carry ownership claims")
            item["confidence"] = item.get("confidence", "EVIDENCE_ONLY")
            normalized_records.append(item)
        normalized_certificates = []
        for certificate in certificates:
            item = dict(certificate)
            left, right = item.get("start"), item.get("end")
            if item.get("certificate_type") not in CERTIFICATE_TYPES:
                raise ValueError("unknown static certificate type")
            if type(left) is not int or type(right) is not int or not request.start <= left < right <= request.end:
                raise ValueError("static certificate escapes request range")
            if item.get("status", "PROVISIONAL") not in STATUSES:
                raise ValueError("invalid static certificate status")
            item.setdefault("status", "PROVISIONAL")
            item.setdefault("rule_id", "STATIC_RULE")
            item.setdefault("witnesses", [])
            payload = {"request_id": request.id, **item}
            item["id"] = digest({"kind": "thor-v5-static-certificate", "value": payload})
            normalized_certificates.append(item)
        response_payload = {"trace": self.trace, "rom_sha256": self.rom_sha256,
                            "request_id": request.id, "records": normalized_records,
                            "certificates": normalized_certificates,
                            "unresolved": list(unresolved)}
        response_id = digest({"kind": "thor-v5-static-response", "value": response_payload})
        response = {"schema": "thor.evidence.v5.static-response", "id": response_id,
                    **response_payload}
        self.responses[response_id] = response
        return response

    def merge_to_carver(self, db, response_id):
        response = self.responses.get(response_id)
        if response is None:
            raise ValueError("unknown static response")
        request = self.requests[response["request_id"]]
        for record in response["records"]:
            item = dict(record)
            item.update({"producer": "thor_v5_static_bridge", "source_ref": response_id,
                         "confidence": item.get("confidence", "EVIDENCE_ONLY")})
            evidence_id = db.add_evidence(item)
            self.merged.append(evidence_id)
            db.add_edge(response_id, evidence_id, "STATIC_EVIDENCE", [evidence_id])
        for certificate in response["certificates"]:
            evidence = {"id": certificate["id"], "type": certificate["certificate_type"],
                        "producer": "thor_v5_static_bridge", "confidence": "EVIDENCE_ONLY",
                        "start": certificate["start"], "end": certificate["end"],
                        "details": {"rule_id": certificate["rule_id"],
                                    "status": certificate["status"],
                                    "witnesses": certificate["witnesses"],
                                    "request_id": request.id, "response_id": response_id}}
            evidence_id = db.add_evidence(evidence)
            self.merged.append(evidence_id)
            db.add_edge(response_id, evidence_id, "STATIC_CERTIFICATE", [evidence_id])
        return {"response_id": response_id, "evidence_ids": list(self.merged),
                "source_owned_before": db.source_owned_bytes(),
                "source_owned_after": db.source_owned_bytes()}

    def carver_export(self, db):
        gaps, campaigns = db.gap_report()
        return {"schema": "thor.evidence.v5.carver-export", "trace": self.trace,
                "rom_sha256": self.rom_sha256, "interval_db": db.interval_db(),
                "gap_report": {"gaps": gaps, "campaigns": campaigns},
                "merged_evidence": sorted(set(self.merged))}

    def export(self):
        return {"schema": "thor.evidence.v5.static-bridge", "trace": self.trace,
                "rom_sha256": self.rom_sha256,
                "requests": [_plain(asdict(v)) for v in sorted(self.requests.values(), key=lambda x: x.id)],
                "responses": [self.responses[key] for key in sorted(self.responses)],
                "merged_evidence": sorted(set(self.merged))}
