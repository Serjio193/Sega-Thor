#!/usr/bin/env python3
"""Contract test for the persistent GPGX bitmap merge semantics."""


def commit(global_known, session_all):
    new = session_all - global_known
    return global_known | session_all, new


def main():
    known, new = commit(set(), {100, 102, 104})
    assert new == {100, 102, 104}
    assert known == {100, 102, 104}

    known, new = commit(known, {100, 102, 106})
    assert new == {106}
    assert known == {100, 102, 104, 106}

    known, new = commit(known, {100, 102, 104, 106})
    assert new == set()
    assert known == {100, 102, 104, 106}


if __name__ == "__main__":
    main()
