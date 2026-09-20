# Candidate progress contract (UI worker)
Scope: flash/first-install/adapter/adapter.mjs only. Existing onState phase events retained. New onProgress callback receives frozen {phase, kind, assetIndex, bytes, totalBytes, elapsedMs, phaseElapsedMs}. kind is compressed-write (vendor compressed bytes acknowledged), image-read (received, NOT yet SHA verified), or image-verified (full image/tail SHA-256 matched). assetIndex indexes the phase's ordered pin list; six readbacks remain images then FF tails. Times use performance.now since run/phase start. No aggregate percentage, no sensitive identifiers. Phase changing-baud occurs after official-stub before jedec-preflight at 460800; ROM remains 115200. Complete remains only after all verification and cleanup. Callback exceptions cannot change policy. No visual redesign in this candidate.

## Isolated future UI candidate: event-driven per-image track

This follow-on changes only installer presentation/tests, not the adapter contract,
protocol, pins, firmware, archives, consent gates, or one-shot execution policy.
The original contract above describes the accepted baseline.

- One small decorative track accompanies the exact current image/tail byte label.
  Known totals set its width directly to `bytes / totalBytes`; unknown totals show
  bytes only. This is never an installation-wide percentage.
- A real advancing compressed-write acknowledgement or image-read event may
  trigger one 180ms opacity pulse. Width and text update synchronously (the old
  250ms text throttle is removed); neither counts nor elapsed time interpolate.
  No timers, indeterminate spinners, CSS loops, or animation-frame progress.
- Readback at full length still says SHA-256 pending. Only image-verified events
  produce the static verified style/label. Writing, reading and verified states
  remain textually distinct; color is supplementary, not the source of truth.
- Phase/status changes, errors, cancellation and terminal completion cancel motion
  and hide the track. Stale/malformed/regressing events retain existing rejection.
  Repeated byte counts do not restart motion. A silent event stream stays still.
- Reduced-motion preference disables the pulse; changing preference cancels any
  current pulse without changing bytes. Missing Web Animations support is static.
- Existing `role=status` is explicitly polite/atomic; the decorative track is
  aria-hidden to avoid duplicate announcements or an invented overall progressbar.
- Regression gates: Node first-install tests, scripts/test-progress-animation.py,
  existing speed UI/keyboard/native accessibility and Web Streams harnesses.
  The browser tests use synthetic sessions, not hardware qualification. Host
  suite failures must be reported; no firmware/pin repairs are in this scope.
