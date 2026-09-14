import tempfile
import unittest
from pathlib import Path

from thor_evidence.cartographer import Cartographer, digest


def node(kind, key, status="PROVEN", attrs=None, scope="global"):
    return {"kind": kind, "key": key, "scope": scope, "status": status,
            "attributes": attrs or {}, "lineage": [{"source": "test"}]}


def edge(source, target, relation="DEPENDS", status="PROVEN", scope="global"):
    nid = lambda item: digest({"kind": item["kind"], "key": item["key"], "scope": item.get("scope", "global")})
    return {"source": nid(source), "target": nid(target), "relation": relation,
            "scope": scope, "status": status, "lineage": [{"source": "test"}]}


def bundle(nodes, edges=(), frontiers=(), resolves=()):
    return {"nodes": list(nodes), "edges": list(edges), "frontiers": list(frontiers),
            "resolves_frontiers": list(resolves)}


class Map1Test(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="map1-test-")
        self.graph = Cartographer(Path(self.temp.name) / "map.sqlite", "rom")

    def tearDown(self):
        self.graph.close()
        self.temp.cleanup()

    def test_new_chain_delta(self):
        a, b = node("ROM_INSTRUCTION", "A"), node("ROM_INSTRUCTION", "B")
        delta = self.graph.merge(bundle([a, b], [edge(a, b)]), "a", "a")
        self.assertEqual((delta.new_nodes, delta.new_edges), (2, 1))

    def test_exact_replay_zero_and_hash(self):
        a, b = node("ROM_INSTRUCTION", "A"), node("ROM_INSTRUCTION", "B")
        data = bundle([a, b], [edge(a, b)])
        first = self.graph.merge(data, "a", "a")
        replay = self.graph.merge(data, "a", "a")
        self.assertEqual(replay.new_nodes + replay.new_edges, 0)
        self.assertEqual(first.graph_hash, replay.graph_hash)

    def test_known_prefix_new_tail(self):
        a, b, c = [node("ROM_INSTRUCTION", x) for x in "ABC"]
        self.graph.merge(bundle([a, b], [edge(a, b)]), "a", "a")
        delta = self.graph.merge(bundle([b, c], [edge(b, c)]), "b", "b")
        self.assertEqual((delta.new_nodes, delta.new_edges), (1, 1))

    def test_new_prefix_known_subtree(self):
        a, b, c = [node("ROM_INSTRUCTION", x) for x in "ABC"]
        self.graph.merge(bundle([b, c], [edge(b, c)]), "b", "b")
        delta = self.graph.merge(bundle([a, b], [edge(a, b)]), "a", "a")
        self.assertEqual((delta.new_nodes, delta.new_edges), (1, 1))

    def test_new_branch(self):
        a, b, c = [node("ROM_INSTRUCTION", x) for x in "ABC"]
        self.graph.merge(bundle([a, b], [edge(a, b)]), "base", "base")
        delta = self.graph.merge(bundle([c, b], [edge(c, b)]), "branch", "branch")
        self.assertEqual((delta.new_nodes, delta.new_edges), (1, 1))

    def test_runtime_occurrences_are_distinct(self):
        a = node("REGISTER_VERSION", "A:epoch=1:sequence=2", scope="epoch:1")
        b = node("REGISTER_VERSION", "A:epoch=2:sequence=2", scope="epoch:2")
        self.graph.merge(bundle([a, b]), "occurrences", "x")
        self.assertEqual(self.graph.metrics()["nodes"], 2)

    def test_value_versions_are_distinct(self):
        a = node("RAM_VALUE_VERSION", "0x100:frame=1", scope="frame:1")
        b = node("RAM_VALUE_VERSION", "0x100:frame=2", scope="frame:2")
        self.graph.merge(bundle([a, b]), "values", "x")
        self.assertEqual(self.graph.metrics()["nodes"], 2)

    def test_observed_does_not_count_proven(self):
        a, b = node("ROM_INSTRUCTION", "A", "OBSERVED"), node("ROM_INSTRUCTION", "B", "OBSERVED")
        self.graph.merge(bundle([a, b], [edge(a, b, status="OBSERVED")]), "observed", "x")
        self.assertEqual(self.graph.metrics()["proven_edges"], 0)

    def test_observed_promotes_to_proven(self):
        a, b = node("ROM_INSTRUCTION", "A", "OBSERVED"), node("ROM_INSTRUCTION", "B", "OBSERVED")
        self.graph.merge(bundle([a, b], [edge(a, b, status="OBSERVED")]), "observed", "x")
        a["status"], b["status"] = "PROVEN", "PROVEN"
        delta = self.graph.merge(bundle([a, b], [edge(a, b)]), "proven", "y")
        self.assertEqual((delta.promoted_nodes, delta.promoted_edges), (2, 1))

    def test_frontier_dedupe(self):
        item = {"anchor": "A", "role": "caller", "reason": "missing", "target": "B", "status": "OPEN", "lineage": [{"source": "x"}]}
        first = self.graph.merge(bundle([], frontiers=[item]), "f1", "x")
        second = self.graph.merge(bundle([], frontiers=[item]), "f2", "x")
        self.assertEqual((first.new_frontiers, second.new_frontiers), (1, 0))

    def test_frontier_resolution(self):
        item = {"anchor": "A", "role": "caller", "reason": "missing", "target": "B", "status": "OPEN"}
        self.graph.merge(bundle([], frontiers=[item]), "f1", "x")
        delta = self.graph.merge(bundle([], resolves=[item]), "f2", "x")
        self.assertEqual(delta.resolved_frontiers, 1)
        self.assertEqual(self.graph.metrics()["open_frontiers"], 0)

    def test_conflict_keeps_both_claims(self):
        self.graph.merge(bundle([node("ROM_INSTRUCTION", "A", attrs={"opcode": "1"})]), "one", "1")
        delta = self.graph.merge(bundle([node("ROM_INSTRUCTION", "A", attrs={"opcode": "2"})]), "two", "2")
        self.assertEqual(delta.new_conflicts, 1)
        self.assertEqual(self.graph.metrics()["conflicts"], 1)

    def test_component_join_once(self):
        a, b, c, d = [node("ROM_INSTRUCTION", x) for x in "ABCD"]
        self.graph.merge(bundle([a, b], [edge(a, b)]), "one", "1")
        self.graph.merge(bundle([c, d], [edge(c, d)]), "two", "2")
        delta = self.graph.merge(bundle([b, c], [edge(b, c)]), "join", "3")
        replay = self.graph.merge(bundle([b, c], [edge(b, c)]), "join", "3")
        self.assertEqual(delta.component_joins, 1)
        self.assertEqual(replay.component_joins, 0)

    def test_lineage_union(self):
        a = node("ROM_INSTRUCTION", "A")
        a["lineage"] = [{"source": "one"}]
        self.graph.merge(bundle([a]), "one", "1")
        a["lineage"] = [{"source": "two"}]
        self.graph.merge(bundle([a]), "two", "2")
        count = self.graph.db.execute("SELECT lineage FROM map_node").fetchone()[0]
        self.assertEqual(len(__import__("json").loads(count)), 2)

    def test_import_order_deterministic(self):
        a, b, c = [node("ROM_INSTRUCTION", x) for x in "ABC"]
        one, two = bundle([a, b], [edge(a, b)]), bundle([b, c], [edge(b, c)])
        self.graph.merge(one, "one", "1")
        self.graph.merge(two, "two", "2")
        first = self.graph.graph_hash()
        with tempfile.TemporaryDirectory(prefix="map1-order-") as directory:
            other = Cartographer(Path(directory) / "map.sqlite", "rom")
            other.merge(two, "two", "2")
            other.merge(one, "one", "1")
            self.assertEqual(first, other.graph_hash())
            other.close()

    def test_source_owned_separate(self):
        graph = Cartographer(Path(self.temp.name) / "owned.sqlite", "rom", 123)
        graph.merge(bundle([node("ROM_RANGE", "0:10")]), "owned", "x")
        self.assertEqual(graph.metrics()["source_owned_bytes"], 123)
        graph.close()


if __name__ == "__main__":
    unittest.main()
