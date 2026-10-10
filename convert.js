// Turns Kelli's fx-9860 Wolfenstein 3D (.g1a) into a fx-CG50 add-in (.g3a), in the browser.
// The payload (payload.js) is a complete .g3a with zeros where the game goes: this copies the
// game's code in, applies the patches that point it to the shim, and recomputes the checksum.

(function (root) {
  'use strict';

  function fromBase64(s) {
    const bin = atob(s);
    const out = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
    return out;
  }

  function fromHex(s) {
    const out = new Uint8Array(s.length / 2);
    for (let i = 0; i < out.length; i++) out[i] = parseInt(s.substr(2 * i, 2), 16);
    return out;
  }

  async function sha1(bytes) {
    const h = new Uint8Array(await crypto.subtle.digest('SHA-1', bytes));
    return Array.from(h, (b) => b.toString(16).padStart(2, '0')).join('');
  }

  // g1a: Uint8Array of the .g1a file. Returns the .g3a as a Uint8Array, or throws an Error
  // whose message is meant for the user.
  async function convertG1a(g1a, payload) {
    const hash = await sha1(g1a);
    if (hash !== payload.sha1) {
      if (payload.others[hash])
        throw new Error('This is ' + payload.others[hash] + ' of the game. Take the SH4 version (file 2).');
      throw new Error('This is not the SH4 version of Kelli’s Wolfenstein 3D (file 2 on Planète Casio).');
    }
    const g3a = fromBase64(payload.template);
    const code = payload.code;
    g3a.set(g1a.subarray(0x200), code + 0x200);
    for (const [off, hex] of payload.patches) g3a.set(fromHex(hex), code + off);

    // .g3a checksum: the bytes 0..0x1F (stored inverted) and 0x24 up to the last 4, which
    // repeat it
    let sum = 0;
    for (let i = 0; i < 0x20; i++) sum += g3a[i];
    for (let i = 0x24; i < g3a.length - 4; i++) sum += g3a[i];
    const view = new DataView(g3a.buffer);
    view.setUint32(0x20, sum >>> 0);
    view.setUint32(g3a.length - 4, sum >>> 0);
    return g3a;
  }

  if (typeof module === 'object' && module.exports) module.exports = { convertG1a };
  else root.convertG1a = convertG1a;
})(this);
