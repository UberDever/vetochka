# Proper tail calls

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- EPIC: 20260529-103914

- Decision (user, 2026-08-30): proper tail calls are an obligation while writing the CESK rules.
- Frames arise only when a sub-result is needed; every other transition replaces control.
- Caveat (2026-07-09): storage still grows per iteration until reclamation exists.
