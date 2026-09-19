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
function profileCopy(profile) {
  const p = structuredClone(profile);
  if (p.board !== BOARD || !['firstinstall','appupdate'].includes(p.mode)) fail('PROFILE_REFUSED');
  const expected = p.mode === 'firstinstall' ? ['boot','table','app'] : ['app'];
  if (JSON.stringify(p.assets?.map(x => x.role)) !== JSON.stringify(expected)) fail('PROFILE_REFUSED');
  const validate = a => {
    const ranges = {boot:[0x2000,0x6000],table:[0x8000,0x1000],app:[0x10000,0x400000]};
    if (!a || !ranges[a.role] || a.offset !== ranges[a.role][0] || !Number.isSafeInteger(a.length) || a.length <= 0 || a.length > ranges[a.role][1] || a.length % 4 || !/^[0-9a-f]{64}$/.test(a.sha256)) fail('PROFILE_REFUSED');
  };
  p.assets.forEach(validate);
  if (p.mode === 'appupdate') {
    if (JSON.stringify(p.compatibility?.map(a => a.role)) !== '["boot","table"]') fail('PROFILE_REFUSED');
    if (p.predecessor !== undefined) { validate(p.predecessor); if (p.predecessor.role !== 'app' || p.predecessor.alternatives !== undefined) fail('PROFILE_REFUSED'); }
    p.compatibility.forEach(a => { validate(a); if (a.alternatives !== undefined && (!Array.isArray(a.alternatives) || a.alternatives.length > 1 || a.alternatives.some(h => !/^[0-9a-f]{64}$/.test(h)))) fail('PROFILE_REFUSED'); });
  }
  return freeze(p);
}
// Pinned esp-flasher-stub v1.2.2 sends one raw 16-byte MD5 SLIP frame
// after the final cumulative ACK. Finish that transaction before ANY command.
// SHA-256 against the trusted pin below remains the integrity authority; this
// trailer is mandatory framing, not an alternative integrity/authentication check.
export async function readFlashComplete(loader, address, length) {
  const data = await loader.readFlash(address, length);
  if (!(data instanceof Uint8Array) || data.length !== length) fail('READBACK_MISMATCH');
  const trailer = await loader.transport.read(loader.FLASH_READ_TIMEOUT);
  if (!(trailer instanceof Uint8Array) || trailer.length !== 16) fail('READBACK_TRAILER_REFUSED');
  return data;
}
// Diagnostics only: no policy decision, IO, raw values, key purposes or identifiers.
export function securityDiagnostic(s) {
  const own = (o,k) => !!o && Object.hasOwn(o,k);
  const bool = v => v === false ? 'false' : v === true ? 'true' : 'invalid';
  const zero = v => v === 0 ? 'zero' : Number.isInteger(v) && v > 0 && v <= 0xffffffff ? 'nonzero' : 'invalid';
  const validFlags = Number.isInteger(s?.flags) && s.flags >= 0 && s.flags <= 0xffffffff;
  const unknown = validFlags && (s.flags & ~0x7ff) !== 0;
  const reason = s?.chipId !== 18 ? 'CHIP_REFUSED' : !validFlags ? 'FLAGS_INVALID' : unknown ? 'FLAGS_UNKNOWN_BITS' : s.apiVersion !== 0 ? 'API_VERSION_REFUSED' : s.parsedFlags?.SECURE_BOOT_EN !== false ? 'SECURE_BOOT_NOT_FALSE' : s.parsedFlags?.SECURE_DOWNLOAD_ENABLE !== false ? 'SECURE_DOWNLOAD_NOT_FALSE' : s.flashCryptCnt !== 0 ? 'FLASH_CRYPT_COUNT_NOT_ZERO' : 'SECURITY_CHECKS_CLEAR';
  return Object.freeze({reason, chipIdMatches: s?.chipId === 18,
    flagsPresent:own(s,'flags'), flags:validFlags ? (unknown ? 'unknown-bits' : 'known-bits-only') : 'invalid',
    apiVersionPresent:own(s,'apiVersion'), apiVersion:zero(s?.apiVersion),
    secureBootPresent:own(s?.parsedFlags,'SECURE_BOOT_EN'), secureBoot:bool(s?.parsedFlags?.SECURE_BOOT_EN),
    secureDownloadPresent:own(s?.parsedFlags,'SECURE_DOWNLOAD_ENABLE'), secureDownload:bool(s?.parsedFlags?.SECURE_DOWNLOAD_ENABLE),
    flashCryptCntPresent:own(s,'flashCryptCnt'), flashCryptCnt:zero(s?.flashCryptCnt)});
}
export class GuardedSession {
  #p; #Loader; #Transport; #P4; #state; #used = false; #abort = new AbortController();
  #transport; #port; #writer; #cleanup; #closed = false; #opening; #deadline;
  #lastPhase = 'validation'; #phaseStart = performance.now(); #writeEntered = false;
  #timedOut = false; #endsAt; #securityDiagnostic;
  constructor({profile, Loader, Transport, ESP32P4ROM, onState = () => {}, phaseTimeoutMs = 300000, cleanupTimeoutMs = 5000}) {
    this.#p = profileCopy(profile); this.#Loader = Loader; this.#Transport = Transport; this.#P4 = ESP32P4ROM; this.#state = onState;
    if (!Number.isFinite(phaseTimeoutMs) || phaseTimeoutMs < 1 || !Number.isFinite(cleanupTimeoutMs) || cleanupTimeoutMs < 1) fail('TIMEOUT_REFUSED');
    this.#deadline = phaseTimeoutMs; this.cleanupTimeoutMs = cleanupTimeoutMs;
  }
  #emit(state, details = {}) { try { this.#state(Object.freeze({state, ...details})); } catch {} }
  #check() { if (this.#abort.signal.aborted) fail('SESSION_CANCELLED'); }
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
    this.#emit(name);
    let timer, listener;
    const interruption = new Promise((_, reject) => {
      listener = () => { void this.#stop(); reject(new Error('SESSION_CANCELLED')); };
      this.#abort.signal.addEventListener('abort', listener, {once:true});
      timer = setTimeout(() => { this.#timedOut = true; this.#abort.abort(); }, Math.max(1, this.#endsAt - performance.now()));
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
    t.read = async (...args) => { self.#check(); if (self.#closed) fail('IO_CLOSED'); const result = await read(...args); self.#check(); return result; };
    t.setDeviceLostCallback(() => self.cancel());
    this.#transport = t;
    return t;
  }
  run(args) { return this.#run(args, false); }
  checkDevice(args) { return this.#run(args, true); }
  async #run({port, assets, boardConfirmation, firstInstallConsent = false, resetOnSuccess = false}, diagnostic) {
    if (this.#used) fail('SESSION_ALREADY_USED'); this.#used = true;
    this.#endsAt = performance.now() + this.#deadline;
    try {
      this.#check();
      if (boardConfirmation !== BOARD) fail('BOARD_CONFIRMATION_REQUIRED');
      if (!diagnostic && this.#p.mode === 'firstinstall' && firstInstallConsent !== true) fail('FIRSTINSTALL_CONSENT_REQUIRED');
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
        const s = await l.getSecurityInfo(false);
        this.#securityDiagnostic = securityDiagnostic(s);
        if (s.chipId !== 18) fail('CHIP_REFUSED');
        if (!Number.isInteger(s.flags) || s.flags < 0 || s.flags > 0xffffffff || (s.flags & ~0x7ff) !== 0 || s.apiVersion !== 0 || s.parsedFlags?.SECURE_BOOT_EN !== false || s.parsedFlags?.SECURE_DOWNLOAD_ENABLE !== false || s.flashCryptCnt !== 0) fail('SECURITY_REFUSED');
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
      await this.#phase('jedec-preflight', async () => {
        const id = await l.readFlashId();
        // Raw JEDEC density code 0x19 (2^25 bytes), no image/header fallback.
        if (!Number.isInteger(id) || id < 1 || id > 0xffffff || ((id >>> 16) & 255) !== 0x19 || (id & 255) === 0 || (id & 255) === 255) fail('JEDEC_REFUSED');
      });
      const verify = async pins => {
        for (const a of pins) {
          this.#check(); const data = await readFlashComplete(l, a.offset, a.length);
          if (!(data instanceof Uint8Array) || data.length !== a.length || ![a.sha256,...(a.alternatives || [])].includes(await digest(data))) fail('READBACK_MISMATCH');
        }
      };
      if (this.#p.mode === 'appupdate') await this.#phase('compatibility-readback', () => verify([...this.#p.compatibility, ...(this.#p.predecessor ? [this.#p.predecessor] : [])]));
      if (diagnostic) {
        if (resetOnSuccess === true) await this.#phase('resetting-diagnostic', () => l.after('hard_reset'));
        const clean = await this.#stop(); if (!clean) fail('CLEANUP_INCOMPLETE');
        this.#emit('diagnostic-complete', {firmwareWriteEntered:false, cleanupSuccess:true});
        return Object.freeze({ok:true, diagnostic:true, verified:false, reset:resetOnSuccess === true});
      }
      await this.#phase('writing', () => l.writeFlash({fileArray:images, flashSize:'keep', flashMode:'keep', flashFreq:'keep', compress:true, eraseAll:false}));
      await this.#phase('verifying-readback', () => verify([...(this.#p.compatibility || []), ...this.#p.assets]));
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
