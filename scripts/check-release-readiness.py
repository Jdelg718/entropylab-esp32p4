#!/usr/bin/env python3
"""Offline snapshot coherence, not new hardware acceptance or release approval.

When new acceptance arrives, review the gate contract, ledger and current prose
 together. Never rewrite predecessor receipts to make this check pass.
"""
import hashlib
import json
from pathlib import Path

RUNTIME = 'dc5c65420aa96013ae3847ab4771a7f520f1c3a8'
EVIDENCE = ('evidence/physical-acceptance.md', 'evidence/remaining-gates.json',
            'GATES-REPORT.md')
DOCUMENTS = ('docs/CURRENT-STATUS.md', 'docs/current/ROADMAP.md',
             'docs/RELEASE-READINESS.md')
GATES = {
    'one_board_install_retention': 'accepted_human_observation',
    'six_image_tail_hashes': 'accepted_browser_result',
    'native_platform_suites': 'passed_synthetic_serial',
    'historical_candidate_package': 'verified_unpublished',
    'physical_speed': 'not_measured',
    'outside_window': 'not_pass',
    'recovery': 'not_qualified',
    'final_candidate_package_review': 'required_before_publication',
    'publication_authorization': 'required_before_publication',
}
START = '<!-- release-readiness:start -->'
END = '<!-- release-readiness:end -->'


def summary(gates):
    return '\n'.join([START, '| Gate | Evidence state |', '|---|---|',
                      *(f'| {key} | {value} |' for key, value in gates.items()), END])


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check(root):
    data = json.loads((root / 'evidence/release-readiness.json').read_text())
    require(data['schema_version'] == 1, 'unsupported schema')
    require(data['runtime_commit'] == RUNTIME, 'runtime contract drift')
    require(data['gates'] == GATES, 'gate contract drift; review new evidence explicitly')
    require(set(data['evidence_sha256']) == set(EVIDENCE), 'evidence path set drift')
    for name in EVIDENCE:
        path = root / name
        require(path.is_file(), 'missing evidence: ' + name)
        require(hashlib.sha256(path.read_bytes()).hexdigest() == data['evidence_sha256'][name],
                'evidence drift: ' + name)
    historical = json.loads((root / 'evidence/remaining-gates.json').read_text())
    require(historical['runtime_commit'] == RUNTIME, 'historical runtime drift')
    require(historical['physical']['status'] == 'BLOCKED_BEFORE_PORT_OPEN',
            'earlier zero-write receipt must remain historical')
    require(historical['physical']['writes_requested'] == 0, 'earlier write receipt drift')
    for platform in ('mac', 'win'):
        receipt = historical['platforms'][platform]
        require((receipt['node_passed'], receipt['node_failed']) == (62, 0), 'native Node receipt drift')
        for name, cases in (('test-speed-browser', 16), ('test-speed-ui', 8)):
            require((receipt['tests'][name]['exit'], receipt['tests'][name]['cases']) == (0, cases),
                    'native browser receipt drift')
    require(historical['package']['published'] is False, 'historical package publication drift')
    for name in DOCUMENTS:
        text = (root / name).read_text()
        require(text.count(START) == 1 and text.count(END) == 1 and summary(GATES) in text,
                'documentation gate summary drift: ' + name)
    return len(EVIDENCE), len(DOCUMENTS), len(GATES)


if __name__ == '__main__':
    evidence, documents, gates = check(Path(__file__).resolve().parents[1])
    print(f'PASS readiness coherence: {evidence} pinned evidence records, {documents} current summaries, {gates} gates; no hardware, speed or publication qualification performed')
