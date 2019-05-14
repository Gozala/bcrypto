/*!
 * salsa20.js - salsa20 for bcrypto
 * Copyright (c) 2018, Christopher Jeffrey (MIT License).
 * https://github.com/bcoin-org/bcrypto
 */

'use strict';

const assert = require('bsert');

/*
 * Constants
 */

const BIG_ENDIAN = new Int8Array(new Int16Array([1]).buffer)[0] === 0;

/**
 * Salsa20
 */

class Salsa20 {
  /**
   * Create a Salsa20 context.
   * @constructor
   */

  constructor() {
    this.state = new Uint32Array(16);
    this.stream = new Uint32Array(16);
    this.bytes = new Uint8Array(this.stream.buffer);
    this.pos = 0;

    if (BIG_ENDIAN)
      this.bytes = Buffer.allocUnsafe(64);
  }

  /**
   * Initialize salsa20 with a key, nonce, and counter.
   * @param {Buffer} key
   * @param {Buffer} nonce
   * @param {Number} counter
   */

  init(key, nonce, counter) {
    if (counter == null)
      counter = 0;

    assert(Buffer.isBuffer(key) && key.length >= 32);
    assert(Buffer.isBuffer(nonce) && nonce.length >= 8);
    assert(Number.isSafeInteger(counter) && counter >= 0);

    if (nonce.length === 24) {
      key = hsalsa20(key, nonce.slice(0, 16));
      nonce = nonce.slice(16);
    }

    this.state[0] = 0x61707865;
    this.state[1] = readU32(key, 0);
    this.state[2] = readU32(key, 4);
    this.state[3] = readU32(key, 8);
    this.state[4] = readU32(key, 12);
    this.state[5] = 0x3320646e;

    if (nonce.length === 8) {
      this.state[6] = readU32(nonce, 0);
      this.state[7] = readU32(nonce, 4);
      this.state[8] = counter >>> 0;
      this.state[9] = (counter / 0x100000000) >>> 0;
    } else if (nonce.length === 12) {
      this.state[6] = readU32(nonce, 0);
      this.state[7] = readU32(nonce, 4);
      this.state[8] = readU32(nonce, 8);
      this.state[9] = (counter / 0x100000000) >>> 0;
    } else if (nonce.length === 16) {
      this.state[6] = readU32(nonce, 0);
      this.state[7] = readU32(nonce, 4);
      this.state[8] = readU32(nonce, 8);
      this.state[9] = readU32(nonce, 12);
    } else {
      assert(false, 'Invalid nonce size.');
    }

    this.state[10] = 0x79622d32;
    this.state[11] = readU32(key, 16);
    this.state[12] = readU32(key, 20);
    this.state[13] = readU32(key, 24);
    this.state[14] = readU32(key, 28);
    this.state[15] = 0x6b206574;

    this.pos = 0;

    return this;
  }

  /**
   * Encrypt/decrypt data.
   * @param {Buffer} data - Will be mutated.
   * @returns {Buffer}
   */

  encrypt(data) {
    return this.crypt(data, data);
  }

  /**
   * Encrypt/decrypt data.
   * @param {Buffer} input
   * @param {Buffer} output
   * @returns {Buffer} output
   */

  crypt(input, output) {
    assert(Buffer.isBuffer(input));
    assert(Buffer.isBuffer(output));

    if (output.length < input.length)
      throw new Error('Invalid output size.');

    for (let i = 0; i < input.length; i++) {
      if ((this.pos & 63) === 0) {
        this._block();
        this.pos = 0;
      }

      output[i] = input[i] ^ this.bytes[this.pos++];
    }

    return output;
  }

  /**
   * Stir the stream.
   */

  _block() {
    for (let i = 0; i < 16; i++)
      this.stream[i] = this.state[i];

    for (let i = 0; i < 10; i++) {
      qround(this.stream, 0, 4, 8, 12);
      qround(this.stream, 5, 9, 13, 1);
      qround(this.stream, 10, 14, 2, 6);
      qround(this.stream, 15, 3, 7, 11);
      qround(this.stream, 0, 1, 2, 3);
      qround(this.stream, 5, 6, 7, 4);
      qround(this.stream, 10, 11, 8, 9);
      qround(this.stream, 15, 12, 13, 14);
    }

    for (let i = 0; i < 16; i++)
      this.stream[i] += this.state[i];

    if (BIG_ENDIAN) {
      for (let i = 0; i < 16; i++)
        writeU32(this.bytes, this.stream[i], i * 4);
    }

    this.state[8] += 1;

    if (this.state[8] === 0)
      this.state[9] += 1;
  }

  /**
   * Destroy context.
   */

  destroy() {
    for (let i = 0; i < 16; i++) {
      this.state[i] = 0;
      this.stream[i] = 0;
    }

    if (BIG_ENDIAN) {
      for (let i = 0; i < 64; i++)
        this.bytes[i] = 0;
    }

    this.pos = 0;

    return this;
  }
}

/*
 * Static
 */

Salsa20.native = 0;

/*
 * HSalsa20
 */

function hsalsa20(key, nonce) {
  assert(Buffer.isBuffer(key));
  assert(Buffer.isBuffer(nonce));
  assert(key.length === 32);
  assert(nonce.length === 16);

  const state = new Uint32Array(16);

  state[0] = 0x61707865;
  state[1] = readU32(key, 0);
  state[2] = readU32(key, 4);
  state[3] = readU32(key, 8);
  state[4] = readU32(key, 12);
  state[5] = 0x3320646e;
  state[6] = readU32(nonce, 0);
  state[7] = readU32(nonce, 4);
  state[8] = readU32(nonce, 8);
  state[9] = readU32(nonce, 12);
  state[10] = 0x79622d32;
  state[11] = readU32(key, 16);
  state[12] = readU32(key, 20);
  state[13] = readU32(key, 24);
  state[14] = readU32(key, 28);
  state[15] = 0x6b206574;

  for (let j = 0; j < 10; j++) {
    qround(state, 0, 4, 8, 12);
    qround(state, 5, 9, 13, 1);
    qround(state, 10, 14, 2, 6);
    qround(state, 15, 3, 7, 11);
    qround(state, 0, 1, 2, 3);
    qround(state, 5, 6, 7, 4);
    qround(state, 10, 11, 8, 9);
    qround(state, 15, 12, 13, 14);
  }

  const out = Buffer.alloc(32);

  writeU32(out, state[0], 0);
  writeU32(out, state[5], 4);
  writeU32(out, state[10], 8);
  writeU32(out, state[15], 12);
  writeU32(out, state[6], 16);
  writeU32(out, state[7], 20);
  writeU32(out, state[8], 24);
  writeU32(out, state[9], 28);

  return out;
}

/*
 * Helpers
 */

function qround(x, a, b, c, d) {
  x[b] ^= rotl32(x[a] + x[d], 7);
  x[c] ^= rotl32(x[b] + x[a], 9);
  x[d] ^= rotl32(x[c] + x[b], 13);
  x[a] ^= rotl32(x[d] + x[c], 18);
}

function rotl32(w, b) {
  return (w << b) | (w >>> (32 - b));
}

function readU32(data, off) {
  return (data[off++]
        + data[off++] * 0x100
        + data[off++] * 0x10000
        + data[off] * 0x1000000);
}

function writeU32(dst, num, off) {
  dst[off++] = num;
  num >>>= 8;
  dst[off++] = num;
  num >>>= 8;
  dst[off++] = num;
  num >>>= 8;
  dst[off++] = num;
  return off;
}

/*
 * Expose
 */

module.exports = Salsa20;