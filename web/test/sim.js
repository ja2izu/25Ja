// 実行: cd web/test && npm install jsdom@24 && node sim.js
// © 2026 JA2IZU, Apache-2.0
// 25Ja Tool を jsdom 上で動かし、プロトコルを話す模擬無線機と通信させて結果を検証する
'use strict';
const fs = require('fs');
const path = require('path');
const { JSDOM } = require('jsdom');
const { webcrypto } = require('crypto');

const WEB = require('path').resolve(__dirname, '..');
const html = fs.readFileSync(path.join(WEB, 'index.html'), 'utf8');

// ---- 枠（ツールと同じ規則を独立に実装）----
const OB = [0x16,0x6c,0x14,0xe6,0x2e,0x91,0x0d,0x40,0x21,0x35,0xd5,0x40,0x13,0x03,0xe9,0x80];
function crc16(b, o, n) { let c = 0; for (let i = 0; i < n; i++) { c ^= b[o+i] << 8; for (let j = 0; j < 8; j++) c = (c & 0x8000) ? ((c << 1) ^ 0x1021) & 0xffff : (c << 1) & 0xffff; } return c; }
function pack(type, data) {
  const body = new Uint8Array(4 + data.length); const bv = new DataView(body.buffer);
  bv.setUint16(0, type, true); bv.setUint16(2, data.length, true); body.set(data, 4);
  let len = body.length; if (len % 2) len++;
  const buf = new Uint8Array(8 + len); const v = new DataView(buf.buffer);
  v.setUint16(0, 0xCDAB, true); v.setUint16(2, len, true); buf.set(body, 4);
  v.setUint16(4 + len, crc16(buf, 4, len), true); v.setUint16(6 + len, 0xBADC, true);
  for (let i = 0; i < 2 + len; i++) buf[4 + i] ^= OB[i % 16];
  return buf;
}
function unpackAll(rx) {
  const out = [];
  for (;;) {
    let pb = -1; for (let i = 0; i + 1 < rx.length; i++) if (rx[i] === 0xab && rx[i+1] === 0xcd) { pb = i; break; }
    if (pb < 0 || rx.length - pb < 8) break;
    const len = rx[pb+2] | (rx[pb+3] << 8); const pe = pb + 6 + len;
    if (rx.length < pe + 2) break;
    const m = Uint8Array.from(rx.slice(pb + 4, pe + 2));
    for (let i = 0; i < m.length; i++) m[i] ^= OB[i % 16];
    const crcOk = crc16(m, 0, len) === (m[len] | (m[len+1] << 8));
    rx.splice(0, pe + 2);
    if (!crcOk) { out.push({ bad: true }); continue; }
    const dv = new DataView(m.buffer);
    out.push({ type: dv.getUint16(0, true), size: dv.getUint16(2, true), data: m.slice(4, len) });
  }
  return out;
}

// ---- 模擬無線機 ----
class Radio {
  constructor(mode) {
    this.mode = mode;                         // 'boot' | 'normal'
    this.spi = new Uint8Array(0x200000).fill(0xff);
    for (let i = 0; i < 512; i++) this.spi[0x10000 + i] = (i * 7 + 3) & 0xff;       // 校正（仮の内容）
    for (let i = 0; i < 0x220; i++) this.spi[0xA000 + i] = (i * 13 + 1) & 0xff;      // 設定
    this.mcu = new Uint8Array(118 * 1024).fill(0xff);
    this.rx = []; this.ts = null; this.blOk = 0; this.rebooted = false; this.log = [];
    this.stats = { spiWrite: 0, fwPages: 0, badCrc: 0, stage: 0, secWrite: 0 };
    this.legacy = false;                      // true: 0x0523（段階書込）を知らない古いF/W
    this.stageSec = -1; this.stageBuf = null; this.stageDirty = false;
  }
  eepromToSpi(off) { if (off >= 0xB000 && off < 0xB200) return 0x10000 + off - 0xB000; if (off >= 0x1E00 && off < 0x2000) return 0x10000 + off - 0x1E00; if (off >= 0xA000 && off < 0xA170) return off; return -1; }
  start(push) {
    this.push = push;
    if (this.mode === 'boot') this.timer = setInterval(() => {
      if (this.blOk >= 3 && this.fwStarted) return;
      const d = new Uint8Array(32); for (let i = 0; i < 16; i++) d[i] = 0x50 + i; d.set(Buffer.from('7.03.01'), 16);
      push(pack(0x0518, d));
    }, 40);
  }
  stop() { clearInterval(this.timer); }
  stageFlush() { if (this.stageDirty) { this.spi.set(this.stageBuf, this.stageSec); this.stats.secWrite++; this.stageDirty = false; } }
  onData(bytes) {
    for (const b of bytes) this.rx.push(b);
    for (const m of unpackAll(this.rx)) {
      if (m.bad) { this.stats.badCrc++; continue; }
      this.handle(m);
    }
  }
  reply(type, data) {
    this.nReply = (this.nReply || 0) + 1;
    if (this.dropEvery && this.nReply % this.dropEvery === 0) { this.stats.dropped = (this.stats.dropped || 0) + 1; return; }   // 実機で見えた「応答が来ない」を模す
    setTimeout(() => this.push(pack(type, data)), 2);
  }
  handle(m) {
    const d = m.data, v = new DataView(d.buffer, d.byteOffset, d.byteLength);
    if (this.mode === 'boot') {
      if (m.type === 0x0530) { this.blOk++; return; }
      if (m.type === 0x0519) {
        this.fwStarted = true;
        const page = v.getUint16(4, true), total = v.getUint16(6, true);
        this.mcu.set(d.slice(12, 12 + 256), page * 256); this.stats.fwPages++; this.fwTotal = total;
        const r = new Uint8Array(8); const rv = new DataView(r.buffer); rv.setUint32(0, v.getUint32(0, true), true); rv.setUint16(4, page, true); rv.setUint16(6, 0, true);
        this.reply(0x051A, r); return;
      }
      if (m.type === 0x0514) return;   // ブートローダーは通常の問い合わせに応えない
      return;
    }
    // normal
    if (m.type === 0x0514) {
      this.ts = v.getUint32(0, true);
      const r = new Uint8Array(40); r.set(Buffer.from('F4HWN v5.2.8'), 0);
      if (this.jaFlag !== undefined) { r[13] = 0x25; r[14] = 0x4A; r[15] = this.jaFlag; }   // 25Ja の目印（0009 以降）
      this.reply(0x0515, r); return;
    }
    if (m.type === 0x051B) {
      const off = v.getUint16(0, true), size = d[2]; if (v.getUint32(4, true) !== this.ts) return;
      const r = new Uint8Array(4 + size); new DataView(r.buffer).setUint16(0, off, true); r[2] = size;
      for (let i = 0; i < size; i++) { const a = this.eepromToSpi(off + i); r[4 + i] = a < 0 ? 0xff : this.spi[a]; }
      this.reply(0x051C, r); return;
    }
    if (m.type === 0x051D) {
      const off = v.getUint16(0, true), size = d[2]; if (v.getUint32(4, true) !== this.ts) return;
      for (let i = 0; i < (size >> 3) * 8; i++) { const a = this.eepromToSpi(off + i); if (a >= 0) this.spi[a] = d[8 + i]; }
      const r = new Uint8Array(4); new DataView(r.buffer).setUint16(0, off, true); this.reply(0x051E, r); return;
    }
    if (m.type === 0x051F) {
      const addr = v.getUint32(0, true), size = Math.min(v.getUint16(4, true), 120); if (v.getUint32(8, true) !== this.ts) return;
      const r = new Uint8Array(8 + size); const rv = new DataView(r.buffer); rv.setUint32(0, addr, true); rv.setUint16(4, size, true);
      r.set(this.spi.slice(addr, addr + size), 8); this.reply(0x0520, r); return;
    }
    if (m.type === 0x0523 && !this.legacy) {   // 25Ja 段階書込: セクタ単位に溜め、別セクタ/確定で書き出す（読み出しには反映しない）
      const addr = v.getUint32(0, true), size = Math.min(v.getUint16(4, true), 120), flags = v.getUint16(6, true); if (v.getUint32(8, true) !== this.ts) return;
      let a = addr, o = 0;
      while (o < size) {
        const sec = a - (a % 4096), n = Math.min(4096 - (a - sec), size - o);
        if (sec !== this.stageSec) { this.stageFlush(); this.stageSec = sec; this.stageBuf = this.spi.slice(sec, sec + 4096); }
        this.stageBuf.set(d.slice(12 + o, 12 + o + n), a - sec); this.stageDirty = true;
        a += n; o += n;
      }
      this.stats.stage++;
      if (flags & 1) this.stageFlush();
      const r = new Uint8Array(8); new DataView(r.buffer).setUint32(0, addr, true); this.reply(0x0524, r); return;
    }
    if (m.type === 0x0521) {
      const addr = v.getUint32(0, true), size = Math.min(v.getUint16(4, true), 120); if (v.getUint32(8, true) !== this.ts) return;
      this.stageFlush();
      this.spi.set(d.slice(12, 12 + size), addr); this.stats.spiWrite++;
      const r = new Uint8Array(6); new DataView(r.buffer).setUint32(0, addr, true); this.reply(0x0522, r); return;
    }
    if (m.type === 0x05DD) { this.stageFlush(); this.rebooted = true; return; }
  }
}

const ONLY = process.env.CASES ? new RegExp(process.env.CASES) : null;   // 例: CASES='^7' node sim.js
async function runCase(name, mode, act, check, setup, opts = {}) {
  if (ONLY && !ONLY.test(name)) return true;
  const radio = new Radio(mode);
  if (setup) setup(radio);
  const dom = new JSDOM(html, { runScripts: 'outside-only', url: opts.url || 'http://localhost/' });
  const w = dom.window;
  // 実機の Web Serial と同じく、open のたびに新しい読み書きの流れを作る（1 ケースで複数回接続できるように）
  const port = {
    async open() {
      let ctrl;
      this.readable = new ReadableStream({ start(c) { ctrl = c; } });
      this.writable = new WritableStream({ write(chunk) { radio.onData(chunk); } });
      radio.start(b => { try { ctrl.enqueue(b); } catch {} });
    },
    async close() { radio.stop(); },
  };
  Object.defineProperty(w.navigator, 'serial', { value: { requestPort: async () => port } });
  Object.defineProperty(w, 'crypto', { value: webcrypto });
  w.confirm = () => true;
  w.URL.createObjectURL = blob => { w.__lastBlob = blob; return 'blob:x'; };
  w.HTMLAnchorElement.prototype.click = function () { w.__downloaded = this.download; };   // jsdom はダウンロード（遷移）できない
  w.fetch = async url => { const p = path.join(WEB, url); const b = fs.readFileSync(p); return { ok: true, status: 200, arrayBuffer: async () => b.buffer.slice(b.byteOffset, b.byteOffset + b.length) }; };
  w.ReadableStream = ReadableStream; w.WritableStream = WritableStream; w.TextEncoder = TextEncoder; w.TextDecoder = TextDecoder;
  const js = html.match(/<script>([\s\S]*)<\/script>/)[1];
  w.eval(js);
  const logs = () => Array.from(w.document.querySelectorAll('#log div')).map(d => d.textContent);
  const t0 = Date.now();
  await act(w, radio);
  // 完了待ち（busy が解けるまで）
  for (let i = 0; i < 6000; i++) { await new Promise(r => setTimeout(r, 50)); if (!w.document.getElementById('calibDumpBtn').disabled) break; }
  const L = logs(); const ok = await check(w, radio, L);
  console.log((ok ? 'PASS' : 'FAIL') + '  ' + name + '  (' + ((Date.now() - t0) / 1000).toFixed(1) + ' s, spiWrite=' + radio.stats.spiWrite + ', fwPages=' + radio.stats.fwPages + ', badCrc=' + radio.stats.badCrc + ')');
  if (!ok) console.log(L.slice(-12).join('\n'));
  w.close();
  return ok;
}
const click = (w, id) => w.document.getElementById(id).click();
const blobBytes = w => new Promise(res => { const fr = new w.FileReader(); fr.onload = () => res(new Uint8Array(fr.result)); fr.readAsArrayBuffer(w.__lastBlob); });
async function setFile(w, id, name, bytes) {
  const input = w.document.getElementById(id);
  const file = new w.File([bytes], name);
  Object.defineProperty(input, 'files', { value: [file], configurable: true });
  input.dispatchEvent(new w.Event('change'));
  await new Promise(r => setTimeout(r, 200));
}
const eq = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);

(async () => {
  const fw = new Uint8Array(fs.readFileSync(path.join(WEB, 'firmware/25Ja_v0.4.1.bin')));
  const font = new Uint8Array(fs.readFileSync(path.join(WEB, 'font/25Ja_font_v4.bin')));
  const res = [];

  res.push(await runCase('1 校正の読み出し', 'normal', async w => click(w, 'calibDumpBtn'),
    async (w, r) => eq(await blobBytes(w), r.spi.slice(0x10000, 0x10200))));

  res.push(await runCase('2 F/W書込（同梱）', 'boot', async w => { click(w, 'fwBundledBtn'); await new Promise(r => setTimeout(r, 300)); click(w, 'fwFlashBtn'); },
    async (w, r) => r.fwTotal === Math.ceil(fw.length / 256) && eq(r.mcu.slice(0, fw.length), fw)));

  res.push(await runCase('2b 通常起動中にF/W書込 → 拒否', 'normal', async w => { click(w, 'fwBundledBtn'); await new Promise(r => setTimeout(r, 300)); click(w, 'fwFlashBtn'); },
    async (w, r, L) => r.stats.fwPages === 0 && L.some(s => s.includes('刷机モード') || s.includes('通常起動'))));

  res.push(await runCase('2c F/W欄に字庫を選ぶ → 拒否', 'boot', async w => { await setFile(w, 'fwFile', 'cn_font.bin', font); },
    async (w, r, L) => w.document.getElementById('fwFlashBtn').disabled && L.some(s => s.includes('字庫のファイル'))));

  res.push(await runCase('3 字庫書込（同梱, 高速）', 'normal', async w => { click(w, 'fontBundledBtn'); await new Promise(r => setTimeout(r, 300)); click(w, 'fontFlashBtn'); },
    async (w, r, L) => { console.log('   stage=' + r.stats.stage + ' secWrite=' + r.stats.secWrite); return eq(r.spi.slice(0x024000, 0x024000 + font.length), font) && L.some(s => s.includes('照合 OK')) && L.some(s => s.includes('高速')) && r.stats.secWrite === 46 && r.stats.spiWrite === 0; },
    r => { for (let i = 0; i < font.length; i++) r.spi[0x024000 + i] = (i * 31) & 0xff; }));   // 旧字庫が入っている状態から

  res.push(await runCase('3c 旧F/Wに字庫 → 従来方式', 'normal', async w => { click(w, 'fontBundledBtn'); await new Promise(r => setTimeout(r, 300)); click(w, 'fontFlashBtn'); },
    async (w, r, L) => eq(r.spi.slice(0x024000, 0x024000 + font.length), font) && L.some(s => s.includes('従来の方式')) && L.some(s => s.includes('照合 OK')),
    r => { r.legacy = true; }));

  res.push(await runCase('3b 刷机モードで字庫書込 → 拒否', 'boot', async w => { click(w, 'fontBundledBtn'); await new Promise(r => setTimeout(r, 300)); click(w, 'fontFlashBtn'); },
    async (w, r, L) => r.stats.spiWrite === 0 && L.some(s => s.includes('刷机モードになっています'))));

  const cal = new Uint8Array(512).map((_, i) => (255 - i) & 0xff);
  res.push(await runCase('4 校正の復元', 'normal', async w => { await setFile(w, 'calibFile', 'calibration.dat', cal); click(w, 'calibRestoreBtn'); },
    async (w, r) => eq(r.spi.slice(0x10000, 0x10200), cal) && r.rebooted));

  res.push(await runCase('5 設定の読み出し', 'normal', async w => click(w, 'cfgBackupBtn'),
    async (w, r) => eq(await blobBytes(w), r.spi.slice(0xA000, 0xA220))));

  const cfg = new Uint8Array(0x220).map((_, i) => (i * 3) & 0xff);
  res.push(await runCase('5b 設定の復元（544B）', 'normal', async w => { await setFile(w, 'cfgFile', '25Ja_config.bin', cfg); click(w, 'cfgRestoreBtn'); },
    async (w, r) => eq(r.spi.slice(0xA000, 0xA220), cfg) && r.rebooted));

  res.push(await runCase('6 起動画像を無効化', 'normal', async w => click(w, 'logoClearBtn'),
    async (w, r) => eq(r.spi.slice(0x1FF000, 0x1FF008), new Uint8Array(8).fill(0xff))));

  // ---- 7 FM 局名 ----
  // 期待値はツールと独立に組む（App/app/fm.h の形式）
  const fmTable = list => {
    const b = new Uint8Array(16 + list.length * 32).fill(0xff);
    b.set([0x46, 0x4d, 0x53, 0x54, 1, list.length]);
    list.forEach(([f10, name], i) => { const o = 16 + i * 32; b[o] = f10 & 0xff; b[o + 1] = f10 >> 8; b.fill(0, o + 4, o + 32); b.set(Buffer.from(name, 'utf8'), o + 4); });
    return b;
  };
  const fmList = [[825, 'NHK FM 名古屋'], [778, 'ZIP-FM'], [807, 'エフエム愛知名古屋'], [899, '東海ラジオ放送']];
  const csvUtf8 = Buffer.from('\uFEFF周波数,局名\r\n82.5,NHK FM 名古屋\r\n77.8,ZIP-FM\r\n80.7,エフエム愛知名古屋\r\n89.9,"東海ラジオ放送"\r\n', 'utf8');
  res.push(await runCase('7a CSV(UTF-8) → 書込', 'normal', async w => { await setFile(w, 'fmCsvFile', 'fm.csv', csvUtf8); await new Promise(r => setTimeout(r, 500)); click(w, 'fmWriteBtn'); },
    async (w, r, L) => { const t = fmTable(fmList); return eq(r.spi.slice(0x12000, 0x12000 + t.length), t) && L.some(s => s.includes('照合 OK')); }));

  // Shift_JIS: 「82.5,名古屋」 名=96BC 古=8CC3 屋=89AE、見出しなし
  const csvSjis = new Uint8Array([...Buffer.from('82.5,'), 0x96, 0xBC, 0x8C, 0xC3, 0x89, 0xAE, 0x0d, 0x0a, ...Buffer.from('90.0,ABC\r\n')]);
  res.push(await runCase('7b CSV(Shift_JIS) → 書込', 'normal', async w => { await setFile(w, 'fmCsvFile', 'fm_sjis.csv', csvSjis); await new Promise(r => setTimeout(r, 500)); click(w, 'fmWriteBtn'); },
    async (w, r, L) => { const t = fmTable([[825, '名古屋'], [900, 'ABC']]); return eq(r.spi.slice(0x12000, 0x12000 + t.length), t) && L.some(s => s.includes('Shift_JIS')); }));

  res.push(await runCase('7c 本体から読み出し → CSV 書出', 'normal', async w => { click(w, 'fmReadBtn'); for (let i = 0; i < 100 && w.document.getElementById('fmReadBtn').disabled !== false; i++) await new Promise(r => setTimeout(r, 50)); await new Promise(r => setTimeout(r, 3000)); click(w, 'fmCsvBtn'); },
    async (w, r, L) => {
      const b = await blobBytes(w); const t = new TextDecoder('utf-8', { ignoreBOM: true }).decode(b);
      return t === '\uFEFF周波数,局名\r\n82.5,NHK FM 名古屋\r\n77.8,ZIP-FM\r\n80.7,エフエム愛知名古屋\r\n89.9,東海ラジオ放送\r\n' && L.some(s => s.includes('4 局'));
    }, r => r.spi.set(fmTable(fmList), 0x12000)));

  // 検査: 10 字（幅超過）/ 字庫にない字（半角カナ）/ 範囲外 / 同じ周波数 → 書かない
  const csvBad = Buffer.from('82.5,あいうえおかきくけこ\n77.8,ｱｲｳ\n120.0,X\n82.5,Y\n80.05,Z\n', 'utf8');
  res.push(await runCase('7d 不正な行 → 書込拒否', 'normal', async w => { await setFile(w, 'fmCsvFile', 'bad.csv', csvBad); await new Promise(r => setTimeout(r, 500)); click(w, 'fmWriteBtn'); },
    async (w, r, L) => r.stats.spiWrite === 0 && ['超過', '字庫にない字', '範囲外', '同じ周波数', '0.1MHz'].every(k => L.some(s => s.includes(k)))));

  res.push(await runCase('7e 局名表の消去', 'normal', async w => click(w, 'fmEraseBtn'),
    async (w, r) => eq(r.spi.slice(0x12000, 0x12010), new Uint8Array(16).fill(0xff)) && r.spi[0x12010] === 0x39, r => r.spi.set(fmTable(fmList), 0x12000)));

  res.push(await runCase('7f FM プリセットの周波数を取り込み → CSV 書出', 'normal', async w => { click(w, 'fmPresetBtn'); await new Promise(r => setTimeout(r, 3000)); click(w, 'fmCsvBtn'); },
    async (w, r, L) => {
      const t = new TextDecoder('utf-8', { ignoreBOM: true }).decode(await blobBytes(w));
      return t === '\uFEFF周波数,局名\r\n82.5,\r\n77.8,\r\n90.0,\r\n' && L.some(s => s.includes('3 件のうち 3 件'));
    }, r => { r.spi.fill(0xff, 0xA028, 0xA028 + 96); [825, 778, 0xffff, 900].forEach((f, i) => { r.spi[0xA028 + i * 2] = f & 0xff; r.spi[0xA029 + i * 2] = f >> 8; }); }));

  const csvPre = Buffer.from('89.9,東海ラジオ放送\n76.5,\n82.5,NHK\n120.0,範囲外\n82.5,重複\n', 'utf8');
  res.push(await runCase('7g 表の周波数を FM プリセットに書く（低い順）', 'normal', async w => { await setFile(w, 'fmCsvFile', 'pre.csv', csvPre); await new Promise(r => setTimeout(r, 500)); click(w, 'fmPresetWriteBtn'); },
    async (w, r, L) => {
      const exp = new Uint8Array(96).fill(0xff); [765, 825, 899].forEach((f, i) => { exp[i * 2] = f & 0xff; exp[i * 2 + 1] = f >> 8; });
      return eq(r.spi.slice(0xA028, 0xA028 + 96), exp) && r.rebooted && L.some(s => s.includes('3 局を書きました'));
    }));

  // ---- 0 初めての手順（各欄の結果が手順の欄に ✔ / ✖ で出る）----
  const st = (w, n) => w.document.getElementById('suSt' + n).textContent;
  const call = (w, id) => w.document.getElementById(id).onclick();
  res.push(await runCase('0a 校正を読み出す → 4 の欄に入る → 書き戻し', 'normal', async w => { await call(w, 'calibDumpBtn'); await call(w, 'calibRestoreBtn'); },
    async (w, r) => st(w, 1).startsWith('✔') && st(w, 5).startsWith('✔') && r.rebooted && w.document.getElementById('calibInfo').textContent.includes('1 で読み出した')));
  res.push(await runCase('0b F/W（刷机モード）→ 手順 2 に ✔', 'boot', async w => { await call(w, 'fwBundledBtn'); await call(w, 'fwFlashBtn'); },
    async (w, r) => st(w, 2).startsWith('✔') && eq(r.mcu.slice(0, fw.length), fw)));
  res.push(await runCase('0c 字庫 → 手順 3 に ✔', 'normal', async w => { await call(w, 'fontBundledBtn'); await call(w, 'fontFlashBtn'); },
    async (w, r) => st(w, 4).startsWith('✔') && eq(r.spi.slice(0x024000, 0x024000 + font.length), font)));
  res.push(await runCase('0d 通常起動のままF/W → 手順 2 に ✖', 'normal', async w => { await call(w, 'fwBundledBtn'); await call(w, 'fwFlashBtn'); },
    async (w, r) => st(w, 2).startsWith('✖') && r.stats.fwPages === 0));
  res.push(await runCase('0e 手順の番号 → その欄へ移動（処理はしない）', 'normal', async w => { w.HTMLElement.prototype.scrollIntoView = function () { w.__scrolled = this.id; }; w.document.querySelector('a.jump[href="#sec3"]').dispatchEvent(new w.MouseEvent('click', { bubbles: true, cancelable: true })); },
    async (w, r) => w.__scrolled === 'sec3' && r.stats.spiWrite === 0 && !r.ts));

  // ---- 直開き（file://）: 同梱ボタンは隠し、ファイル選択だけ ----
  const F = 'file:///C:/Users/x/25Ja_Tool/index.html';
  res.push(await runCase('F1 直開き: 同梱ボタンを隠す・ファイル選択でF/Wを書く', 'boot', async w => { await setFile(w, 'fwFile', '25Ja_v0.4.1.bin', fw); await call(w, 'fwFlashBtn'); },
    async (w, r, L) => !w.document.getElementById('fileProto').hidden && w.document.getElementById('fwBundledBtn').hidden && w.document.getElementById('fontBundledBtn').hidden && eq(r.mcu.slice(0, fw.length), fw),
    null, { url: F }));
  res.push(await runCase('F2 直開き: 字庫を選ぶと局名の字の検査が効く', 'normal', async w => {
      await setFile(w, 'fontFile', '25Ja_font_v4.bin', font); await new Promise(r => setTimeout(r, 300));
      await setFile(w, 'fmCsvFile', 'k.csv', Buffer.from('82.5,ｱｲｳ\n', 'utf8')); await new Promise(r => setTimeout(r, 300)); },
    async (w, r, L) => w.document.querySelector('#fmBody td.w').textContent.includes('字庫にない字'),
    null, { url: F }));

  res.push(await runCase('3d 応答が時々消える（37 回に 1 回）→ 再送で完了・照合 OK', 'normal', async w => { await call(w, 'fontBundledBtn'); await call(w, 'fontFlashBtn'); },
    async (w, r, L) => { console.log('   dropped=' + r.stats.dropped + ' / ' + L.find(s => s.includes('再送'))); return eq(r.spi.slice(0x024000, 0x024000 + font.length), font) && L.some(s => s.includes('照合 OK')); },
    r => { r.dropEvery = 37; }));

  // ---- 9 ファイル選択の表示・ログのコピー／消去・25Ja の目印（0.1.10）----
  res.push(await runCase('9a ファイルを選ぶ → 名前が出る（標準部品は隠す）', 'normal', async w => {
      await setFile(w, 'fwFile', '25Ja_v0.4.1.bin', fw); await setFile(w, 'fmCsvFile', 'k.csv', Buffer.from('82.5,NHK\n', 'utf8')); },
    async (w, r, L) => {
      const d = w.document, picks = d.querySelectorAll('button.pick'), nat = d.querySelectorAll('input[type=file]');
      const csvName = d.getElementById('fmCsvFile').nextElementSibling;
      console.log('   pick=' + picks.length + ' / fwInfo=' + d.getElementById('fwInfo').textContent + ' / csv=' + csvName.textContent);
      return picks.length === nat.length && Array.from(nat).every(i => i.classList.contains('native') && i.previousElementSibling.classList.contains('pick'))
        && d.getElementById('fwInfo').textContent.startsWith('✔ 25Ja_v0.4.1.bin') && csvName.textContent.startsWith('✔ k.csv');
    }));
  res.push(await runCase('9b ログをコピー → 版・日時付き、ログを消す → 1 行', 'normal', async w => {
      Object.defineProperty(w.navigator, 'clipboard', { value: { writeText: async t => { w.__clip = t; } } });
      await w.document.getElementById('logCopyBtn').onclick();
      w.__copied = w.__clip; w.document.getElementById('logClearBtn').onclick(); },
    async (w, r, L) => { console.log('   clip=' + JSON.stringify(w.__copied.slice(0, 60)));
      return /^25Ja Tool Ver 0\.1\.15 \/ /.test(w.__copied) && w.__copied.includes('25Ja Tool を開きました') && L.length === 1 && L[0].includes('ログを消しました')
        && w.document.getElementById('logMsg').textContent.includes('消しました'); }));
  res.push(await runCase('9c 25Ja の目印（CH 名 31 B）をログに出す', 'normal', async w => click(w, 'calibDumpBtn'),
    async (w, r, L) => L.some(s => s.includes('F4HWN v5.2.8（25Ja, CH 名 31 B）')), r => { r.jaFlag = 1; }));
  res.push(await runCase('9d 目印なし（旧F/W）は従来どおり', 'normal', async w => click(w, 'calibDumpBtn'),
    async (w, r, L) => L.some(s => s.includes('本体: F4HWN v5.2.8 / '))));

  console.log(res.every(Boolean) ? 'ALL PASS' : 'SOME FAILED');
  process.exit(0);
})();
