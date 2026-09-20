// Trust boundary: constructors and profile come from reviewed, pinned application code,
// never a downloaded manifest or user input. No port chooser or telemetry lives here.
export const BOARD = 'WaveshareESP32-P4-WIFI6-Touch-LCD-4.3';
export const availability = (env = globalThis) => Object.freeze({supported: env.isSecureContext === true && !!env.navigator?.serial?.requestPort, reason: env.isSecureContext !== true ? 'SECURE_CONTEXT_REQUIRED' : !env.navigator?.serial?.requestPort ? 'WEBSERIAL_UNSUPPORTED' : null});
const activePorts = new WeakSet();
// Only locally minted policy errors can disclose their finite code.
const policyErrors = new WeakSet();
const fail = code => { const error = new Error(code); policyErrors.add(error); throw error; };
const hex = b => Array.from(new Uint8Array(b), x => x.toString(16).padStart(2, '0')).join('');
const digest = async b => hex(await crypto.subtle.digest('SHA-256', b));
const freeze = o => { Object.values(o).forEach(v => { if (v && typeof v === 'object') freeze(v); }); return Object.freeze(o); };
export const FACTORY_PROFILE = freeze({"board": "WaveshareESP32-P4-WIFI6-Touch-LCD-4.3", "mode": "publicfirstinstall", "assets": [{"role": "boot", "offset": 8192, "length": 21264, "sha256": "4f00f81aad82838f4e33555e322abfbfff7d1de947d339c86e50be820cbd7bb4", "eraseEnd": 32768}, {"role": "table", "offset": 32768, "length": 3072, "sha256": "d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb", "eraseEnd": 36864}, {"role": "app", "offset": 65536, "length": 1542288, "sha256": "42d5e3b40a3869157171552162189a281c84b893c8533fe3ae8266bfc4c47550", "eraseEnd": 1609728}], "writeHold": false, "additionalEraseRegions": [], "preserveToolRegions": [[0, 8192], [36864, 65536], [1609728, 33554432]], "tailReadbacks": [{"offset": 29456, "length": 3312, "sha256": "1fa7330b398eac6db72a2e7a012308f62f3e75035fb3164acbea583f011a26e8"}, {"offset": 35840, "length": 1024, "sha256": "5f4ecdb7b71c3e403983fe405cddcdc2f2576b655fdb3e80d94a6f7c32e58bc2"}, {"offset": 1607824, "length": 1904, "sha256": "3824b4a06fc8bd3097aefcdad4f2b113cb8970b10f793afcc563eda6c5fc995c"}], "profileId": "education-candidate02-dev-test-01"});
function profileCopy(profile) {
  if (profile.mode === "publicfirstinstall") {
    if (JSON.stringify(profile) !== JSON.stringify(FACTORY_PROFILE)) fail("PROFILE_REFUSED");
    return FACTORY_PROFILE;
  }
  fail('PROFILE_REFUSED'); // This isolated installer accepts only the exact public contract.

}
// Pinned esp-flasher-stub v1.2.2 sends one raw 16-byte MD5 SLIP frame
// after the final cumulative ACK. Finish that transaction before ANY command.
// SHA-256 against the trusted pin below remains the integrity authority; this
// trailer is mandatory framing, not an alternative integrity/authentication check.
export async function readFlashComplete(loader, address, length, onProgress) {
  const data = await loader.readFlash(address, length, onProgress);
  if (!(data instanceof Uint8Array) || data.length !== length) fail('READBACK_MISMATCH');
  const trailer = await loader.transport.read(loader.FLASH_READ_TIMEOUT);
  if (!(trailer instanceof Uint8Array) || trailer.length !== 16) fail('READBACK_TRAILER_REFUSED');
  return data;
}
// Local conservative rollout gate, NOT a vendor-defined version enum.
// Public P4 ROMs copy _rom_eco_version into the fixed 20-byte layout (0/5).
// ECO2 passed observed no-write diagnostics; prior accepted exact-tuple private-path writes are separate; this public policy remains hardware-unexercised.
// Other values, including public 5, await local qualification. No coercion.
const compatibleRomEco = value => value === 0 || value === 2;
// Diagnostics only: no policy decision, IO, raw flags, key purposes or identifiers.
// API/ECO version is the documented unsigned 32-bit word at response offset 16.
export function securityDiagnostic(s) {
  const own = (o,k) => !!o && Object.hasOwn(o,k);
  const bool = v => v === false ? 'false' : v === true ? 'true' : 'invalid';
  const zero = v => v === 0 ? 'zero' : Number.isInteger(v) && v > 0 && v <= 0xffffffff ? 'nonzero' : 'invalid';
  const validFlags = Number.isInteger(s?.flags) && s.flags >= 0 && s.flags <= 0xffffffff;
  const unknown = validFlags && (s.flags & ~0x7ff) !== 0;
  const reason = s?.chipId !== 18 ? 'CHIP_REFUSED' : !validFlags ? 'FLAGS_INVALID' : unknown ? 'FLAGS_UNKNOWN_BITS' : !compatibleRomEco(s.apiVersion) ? 'API_VERSION_REFUSED' : s.parsedFlags?.SECURE_BOOT_EN !== false ? 'SECURE_BOOT_NOT_FALSE' : s.parsedFlags?.SECURE_DOWNLOAD_ENABLE !== false ? 'SECURE_DOWNLOAD_NOT_FALSE' : s.flashCryptCnt !== 0 ? 'FLASH_CRYPT_COUNT_NOT_ZERO' : 'SECURITY_CHECKS_CLEAR';
  return Object.freeze({reason, chipIdMatches: s?.chipId === 18,
    flagsPresent:own(s,'flags'), flags:validFlags ? (unknown ? 'unknown-bits' : 'known-bits-only') : 'invalid',
    apiVersionPresent:own(s,'apiVersion'), apiVersion:zero(s?.apiVersion),
    apiVersionNumber:Number.isInteger(s?.apiVersion) && s.apiVersion >= 0 && s.apiVersion <= 0xffffffff ? s.apiVersion : null,
    secureBootPresent:own(s?.parsedFlags,'SECURE_BOOT_EN'), secureBoot:bool(s?.parsedFlags?.SECURE_BOOT_EN),
    secureDownloadPresent:own(s?.parsedFlags,'SECURE_DOWNLOAD_ENABLE'), secureDownload:bool(s?.parsedFlags?.SECURE_DOWNLOAD_ENABLE),
    flashCryptCntPresent:own(s,'flashCryptCnt'), flashCryptCnt:zero(s?.flashCryptCnt)});
}
// Validate complete, already SLIP-decoded frames before the vendor slices data.
// esptool 453e5fd2 serial-protocol.rst: header 8, security data 20,
// P4 ROM trailer 4 (last 2 reserved); stub trailer 2. Reserved bytes
// are deliberately not constrained. No changes to SLIP or other commands.
export async function readSecurityInfo(loader) {
  const transport = loader.transport;
  const read = transport.read;
  const size = 20 + (loader.IS_STUB === true ? 2 : 4);
  transport.read = async function (...args) {
    const p = await read.apply(this, args);
    if (!(p instanceof Uint8Array)) fail('SECURITY_FRAME_REFUSED');
    // Empty SLIP padding and short noise remain the vendor's bounded skip path.
    if (p.length < 8) return p;
    if (p[1] === 0x14) {
      if (p[0] !== 1 || p.length !== 8 + size ||
          (p[2] | (p[3] << 8)) !== size || p[28] !== 0 || p[29] !== 0)
        fail('SECURITY_FRAME_REFUSED');
    }
    return p;
  };
  // Keep the pinned decoder's at-most-two read-only requests, no new retries.
  // A fallback decoded as S2 still lacks P4 identity and cannot pass policy.
  try { return await loader.getSecurityInfo(false); }
  finally { transport.read = read; }
}
export class GuardedSession {
  #p; #Loader; #Transport; #P4; #state; #used = false; #abort = new AbortController();
  #transport; #port; #writer; #cleanup; #closed = false; #opening; #closing; #deadline;
  #progress; #startedAt;
  #lastPhase = 'validation'; #phaseStart = performance.now(); #writeEntered = false;
  #timedOut = false; #endsAt; #phaseEndsAt; #securityDiagnostic;
  constructor({profile, Loader, Transport, ESP32P4ROM, onState = () => {}, onProgress = () => {}, phaseTimeoutMs, cleanupTimeoutMs = 5000}) {
    this.#p = profileCopy(profile); this.#Loader = Loader; this.#Transport = Transport; this.#P4 = ESP32P4ROM; this.#state = onState; this.#progress = onProgress;
    const maximum = this.#p === FACTORY_PROFILE ? 900000 : 600000;
    phaseTimeoutMs ??= maximum;
    if (!Number.isFinite(phaseTimeoutMs) || phaseTimeoutMs < 1 || phaseTimeoutMs > maximum || !Number.isFinite(cleanupTimeoutMs) || cleanupTimeoutMs < 1) fail('TIMEOUT_REFUSED');
    this.#deadline = phaseTimeoutMs; this.cleanupTimeoutMs = cleanupTimeoutMs;
  }
  #emit(state, details = {}) { try { this.#state(Object.freeze({state, ...details})); } catch {} }
  #report(kind, assetIndex, bytes, totalBytes) {
    if (this.#closed || this.#abort.signal.aborted || !Number.isSafeInteger(bytes) || !Number.isSafeInteger(totalBytes) || bytes < 0 || bytes > totalBytes) return;
    const now = performance.now();
    try { this.#progress(Object.freeze({phase:this.#lastPhase, kind, assetIndex, bytes, totalBytes, elapsedMs:Math.max(0, now-this.#startedAt), phaseElapsedMs:Math.max(0, now-this.#phaseStart)})); } catch {}
  }
  #check() {
    // Timers may be delayed by synchronous work or tab scheduling. Check the
    // absolute authority before every guarded continuation as well as racing IO.
    if (performance.now() >= Math.min(this.#endsAt ?? Infinity, this.#phaseEndsAt ?? Infinity)) {
      this.#timedOut = true; this.#abort.abort();
    }
    if (this.#abort.signal.aborted) fail('SESSION_CANCELLED');
  }
  cancel() { this.#abort.abort(); return this.#stop(); }
  async #stop() {
    if (this.#cleanup) return this.#cleanup;
    this.#cleanup = (async () => {
      if (!this.#port) return true;
      // Invalidate every future IO first. Cancel the actual held stream locks, not
      // only the waiting Promise. Late open resolution is closed by guarded open.
      this.#closed = true;
      const work = async () => {
        await Promise.allSettled([this.#transport?.reader?.cancel(), this.#writer?.abort()]);
        if (this.#opening) await this.#opening.catch(() => {});
        // An authorized vendor close may already be pending when cancelled.
        // Do not close/release ownership ahead of that physical operation.
        if (this.#closing) await this.#closing.catch(() => {});
        if (this.#writer) { try { this.#writer.releaseLock(); } catch {} }
        if (this.#transport?.reader) { try { this.#transport.reader.releaseLock(); } catch {} }
        try { await this.#port.close(); } catch (e) {
          if (this.#port.readable || this.#port.writable) throw e;
        }
        activePorts.delete(this.#port);
        return true;
      };
      let timer;
      try { return await Promise.race([work().catch(() => false), new Promise(resolve => { timer = setTimeout(() => resolve(false), this.cleanupTimeoutMs); })]); }
      finally { clearTimeout(timer); }
    })();
    return this.#cleanup;
  }
  async #phase(name, operation) {
    this.#check(); this.#lastPhase = name; this.#phaseStart = performance.now();
    if (name === 'writing') this.#writeEntered = true;
    // Total is shared; phase caps cannot renew operation authority.
    const cap = name === 'writing' ? 240000 : ['verifying-readback','predecessor-readback','factory-preflight'].includes(name) ? 300000 : 60000;
    this.#phaseEndsAt = Math.min(this.#endsAt, performance.now() + cap);
    this.#emit(name);
    let timer, listener;
    const interruption = new Promise((_, reject) => {
      listener = () => { void this.#stop(); reject(new Error('SESSION_CANCELLED')); };
      this.#abort.signal.addEventListener('abort', listener, {once:true});
      timer = setTimeout(() => { this.#timedOut = true; this.#abort.abort(); }, Math.max(1, this.#phaseEndsAt - performance.now()));
    });
    try { const result = await Promise.race([Promise.resolve().then(() => { this.#check(); return operation(); }), interruption]); this.#check(); return result; }
    finally { clearTimeout(timer); this.#abort.signal.removeEventListener('abort', listener); }
  }
  #makeTransport(port) {
    if (!port || activePorts.has(port)) fail('PORT_BUSY');
    activePorts.add(port);
    this.#port = port;
    const self = this;
    const guardedPort = new Proxy(port, {get(target, key) {
      // Revoked transports must not see a successor's streams: vendor changeBaud
      // starts an unawaited readLoop after its final delay. Returning null lets
      // that stale loop exit normally, without acquiring locks or rejecting.
      if ((key === 'readable' || key === 'writable') && (self.#closed || self.#abort.signal.aborted)) return null;
      if (key === 'close') return async () => {
        self.#check(); if (self.#closed) fail('IO_CLOSED');
        const closing = self.#closing = target.close();
        try { await closing; } finally { if (self.#closing === closing) self.#closing = undefined; }
      };
      if (key === 'open') return async options => {
        self.#check(); if (self.#closed) fail('IO_CLOSED');
        self.#opening = target.open(options);
        await self.#opening;
        if (self.#closed || self.#abort.signal.aborted) { await target.close(); fail('IO_CLOSED'); }
      };
      if (key === 'setSignals') return async signals => { self.#check(); if (self.#closed) fail('IO_CLOSED'); return target.setSignals(signals); };
      const value = Reflect.get(target, key, target); return typeof value === 'function' ? value.bind(target) : value;
    }});
    const t = new this.#Transport(guardedPort, false);
    t.trace = () => {}; // upstream has unconditional trace calls in readLoop
    t.write = async data => {
      self.#check(); if (self.#closed || !port.writable) fail('IO_CLOSED');
      const writer = port.writable.getWriter(); self.#writer = writer;
      try { await writer.write(t.slipWriter(data)); self.#check(); }
      finally { try { writer.releaseLock(); } catch {} if (self.#writer === writer) self.#writer = undefined; }
    };
    const read = t.read.bind(t);
    t.read = async (...args) => { self.#check(); if (self.#closed) fail('IO_CLOSED'); args[0] = Math.max(1, Math.min(args[0] ?? 3000, self.#phaseEndsAt - performance.now())); const result = await read(...args); self.#check(); return result; };
    t.setDeviceLostCallback(() => self.cancel());
    this.#transport = t;
    return t;
  }
  run(args) { return this.#run(args, false); }
  checkDevice(args) { return this.#run(args, true); }
  async #run({port, assets, boardConfirmation, firstInstallConsent = false, resetOnSuccess = false}, diagnostic) {
    if (this.#used) fail('SESSION_ALREADY_USED'); this.#used = true;
    this.#startedAt = performance.now();
    this.#endsAt = this.#startedAt + this.#deadline;
    try {
      this.#check();
      if (boardConfirmation !== BOARD) fail('BOARD_CONFIRMATION_REQUIRED');
      if (!diagnostic && ['publicfirstinstall'].includes(this.#p.mode) && firstInstallConsent !== true) fail('FIRSTINSTALL_CONSENT_REQUIRED');
      // Copy every input synchronously before the first await. Caller mutation can
      // neither alter a verified image nor race verification of a later image.
      if (!Array.isArray(assets) || assets.length !== this.#p.assets.length) fail('ASSET_REFUSED');
      const images = this.#p.assets.map((pin, i) => {
        const a = assets[i];
        if (!a || a.offset !== pin.offset || !(a.bytes instanceof Uint8Array) || a.bytes.length !== pin.length || (typeof SharedArrayBuffer !== 'undefined' && a.bytes.buffer instanceof SharedArrayBuffer)) fail('ASSET_REFUSED');
        return {address:pin.offset, data:new Uint8Array(a.bytes)};
      });
      await this.#phase('verifying-assets', async () => {
        for (let i = 0; i < images.length; i++) if (await digest(images[i].data) !== this.#p.assets[i].sha256) fail('ASSET_HASH_MISMATCH');
      });
      const transport = this.#makeTransport(port);
      const l = new this.#Loader({transport, baudrate:115200, romBaudrate:115200, debugLogging:false, terminal:{clean(){}, write(){}, writeLine(){}}});
      l.WRITE_BLOCK_ATTEMPTS = 1;
      await this.#phase('connecting-rom', () => l.connect('default_reset', 1, false));
      if (l.syncStubDetected || l.IS_STUB) fail('EXISTING_STUB_REFUSED');
      await this.#phase('security-preflight', async () => {
        const s = await readSecurityInfo(l);
        this.#securityDiagnostic = securityDiagnostic(s);
        if (s.chipId !== 18) fail('CHIP_REFUSED');
        if (!Number.isInteger(s.flags) || s.flags < 0 || s.flags > 0xffffffff || (s.flags & ~0x7ff) !== 0 || !compatibleRomEco(s.apiVersion) || s.parsedFlags?.SECURE_BOOT_EN !== false || s.parsedFlags?.SECURE_DOWNLOAD_ENABLE !== false || s.flashCryptCnt !== 0) fail('SECURITY_REFUSED');
        const chip = this.#P4 ? new this.#P4() : l.romFromChipId(18);
        if (!chip || chip.IMAGE_CHIP_ID !== 18) fail('CHIP_REFUSED');
        l.applyDetectedChip(chip); l.secureDownloadMode = false;
        const rev = await chip.getChipRevision(l);
        if (!Number.isInteger(rev) || rev < 100 || rev > 199) fail('REVISION_REFUSED');
        // P4 peripheral/watchdog setup is only permitted after identity/security.
        if (typeof chip.postConnect === 'function') await chip.postConnect(l);
      });
      await this.#phase('official-stub', async () => {
        await l.runStub(); if (l.IS_STUB !== true) fail('STUB_REFUSED');
      });
      await this.#phase('changing-baud', async () => {
        if (typeof l.changeBaud !== 'function') fail('BAUD_SWITCH_UNSUPPORTED');
        l.baudrate = 460800;
        // Validate the complete SLIP-decoded ACK before vendor readPacket drops
        // its header (the pinned implementation ignores declared payload length).
        const read = transport.read;
        transport.read = async function (...args) {
          const p = await read.apply(this, args);
          if (!(p instanceof Uint8Array) || p.length !== 10 ||
              p[0] !== 1 || p[1] !== l.ESP_CHANGE_BAUDRATE ||
              (p[2] | (p[3] << 8)) !== 2 || p[8] !== 0 || p[9] !== 0)
            fail('BAUD_SWITCH_REFUSED');
          return p;
        };
        // Retain the decoded-status check for the command boundary as well.
        const command = l.command;
        l.command = async function (...args) {
          const response = await command.apply(this, args);
          if (args[0] === this.ESP_CHANGE_BAUDRATE &&
              (!(response?.[1] instanceof Uint8Array) || response[1].length !== 2 || response[1][0] !== 0 || response[1][1] !== 0)) fail('BAUD_SWITCH_REFUSED');
          return response;
        };
        try { await l.changeBaud(); } finally { l.command = command; transport.read = read; }
        if (transport.baudrate !== 460800) fail('BAUD_SWITCH_REFUSED');
      });
      await this.#phase('jedec-preflight', async () => {
        const id = await l.readFlashId();
        // Raw JEDEC density code 0x19 (2^25 bytes), no image/header fallback.
        if (!Number.isInteger(id) || id < 1 || id > 0xffffff || ((id >>> 16) & 255) !== 0x19 || (id & 255) === 0 || (id & 255) === 255) fail('JEDEC_REFUSED');
      });
      const verify = async pins => {
        for (const [index, a] of pins.entries()) {
          this.#check(); const data = await readFlashComplete(l, a.offset, a.length, (_chunk, bytes, total) => this.#report('image-read', index, bytes, total));
          if (!(data instanceof Uint8Array) || data.length !== a.length || ![a.sha256,...(a.alternatives || [])].includes(await digest(data))) fail('READBACK_MISMATCH');
          this.#report('image-verified', index, a.length, a.length);
        }
      };
      if (this.#p.mode === 'appupdate') {
        await this.#phase('compatibility-readback', () => verify(this.#p.compatibility));
        if (this.#p.predecessor) await this.#phase('predecessor-readback', () => verify([this.#p.predecessor]));
      }
      // Public destructive consent replaces private predecessor authentication; no content identity is inferred.
      if (diagnostic) {
        if (resetOnSuccess === true) await this.#phase('resetting-diagnostic', () => l.after('hard_reset'));
        const clean = await this.#stop(); if (!clean) fail('CLEANUP_INCOMPLETE');
        this.#emit('diagnostic-complete', {firmwareWriteEntered:false, cleanupSuccess:true, securityDiagnostic:this.#securityDiagnostic});
        return Object.freeze({ok:true, diagnostic:true, verified:false, reset:resetOnSuccess === true});
      }
      await this.#phase('writing', () => l.writeFlash({fileArray:images, flashSize:'keep', flashMode:'keep', flashFreq:'keep', compress:true, eraseAll:false, reportProgress:(index, bytes, total) => this.#report('compressed-write', index, bytes, total)}));
      await this.#phase('verifying-readback', () => verify([...(this.#p.compatibility || []), ...this.#p.assets, ...(this.#p.tailReadbacks || [])]));
      if (resetOnSuccess === true) await this.#phase('resetting-verified', () => l.after('hard_reset'));
      const clean = await this.#stop(); if (!clean) fail('CLEANUP_INCOMPLETE');
      this.#emit('complete'); return Object.freeze({ok:true, verified:true, reset:resetOnSuccess === true});
    } catch (error) {
      const elapsed = Math.min(3600000, Math.max(0, Math.round(performance.now() - this.#phaseStart)));
      const errorCode = this.#timedOut ? 'OPERATION_TIMEOUT' : this.#abort.signal.aborted ? 'SESSION_CANCELLED' : policyErrors.has(error) ? error.message : 'DEPENDENCY_ERROR';
      this.#abort.abort(); const clean = await this.#stop();
      this.#emit(clean ? 'failed-disconnected' : 'failed-cleanup-incomplete', {
        phase:this.#lastPhase, phaseElapsedMs:elapsed, errorCode,
        ...(this.#lastPhase === 'security-preflight' && this.#securityDiagnostic ? {securityDiagnostic:this.#securityDiagnostic} : {}),
        errorClass:errorCode === 'OPERATION_TIMEOUT' ? 'timeout' : errorCode === 'SESSION_CANCELLED' ? 'cancelled' : policyErrors.has(error) ? 'policy' : 'dependency',
        firmwareWriteEntered:this.#writeEntered, cleanupSuccess:clean
      });
      // Never expose dependency error text: may contain raw device/serial data.
      throw new Error(clean ? 'SESSION_FAILED' : 'CLEANUP_INCOMPLETE', {cause: undefined});
    }
  }
}
