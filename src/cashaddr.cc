#include <assert.h>
#include <string.h>
#include <node.h>
#include <nan.h>

#include "common.h"
#include "cashaddr/cashaddr.h"
#include "cashaddr.h"

void
BCashAddr::Init(v8::Local<v8::Object> &target) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  Nan::Export(obj, "serialize", BCashAddr::Serialize);
  Nan::Export(obj, "deserialize", BCashAddr::Deserialize);
  Nan::Export(obj, "is", BCashAddr::Is);
  Nan::Export(obj, "convertBits", BCashAddr::ConvertBits);
  Nan::Export(obj, "encode", BCashAddr::Encode);
  Nan::Export(obj, "decode", BCashAddr::Decode);
  Nan::Export(obj, "test", BCashAddr::Test);

  Nan::Set(target, Nan::New("cashaddr").ToLocalChecked(), obj);
}

NAN_METHOD(BCashAddr::Serialize) {
  if (info.Length() < 2)
    return Nan::ThrowError("cashaddr.serialize() requires arguments.");

  if (!info[0]->IsString())
    return Nan::ThrowTypeError("First argument must be a string.");

  Nan::Utf8String prefix_str(info[0]);

  v8::Local<v8::Object> dbuf = info[1].As<v8::Object>();

  if (!node::Buffer::HasInstance(dbuf))
    return Nan::ThrowTypeError("Second argument must be a buffer.");

  const char *prefix = (const char *)*prefix_str;
  const uint8_t *data = (uint8_t *)node::Buffer::Data(dbuf);
  size_t data_len = node::Buffer::Length(dbuf);

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  char output[197];
  memset(output, 0, 197);
  size_t olen = 0;

  if (!bcrypto_cashaddr_serialize(&err, output, prefix, data, data_len))
    return Nan::ThrowError(bcrypto_cashaddr_strerror(err));

  olen = strlen((char *)output);

  info.GetReturnValue().Set(
    Nan::New<v8::String>((char *)output, olen).ToLocalChecked());
}

NAN_METHOD(BCashAddr::Deserialize) {
  if (info.Length() < 2)
    return Nan::ThrowError("cashaddr.deserialize() requires arguments.");

  if (!info[0]->IsString())
    return Nan::ThrowTypeError("First argument must be a string.");

  if (!info[1]->IsString())
    return Nan::ThrowTypeError("Second argument must be a string.");

  Nan::Utf8String addr_(info[0]);
  const char *addr = (const char *)*addr_;

  Nan::Utf8String default_prefix_(info[1]);
  const char *default_prefix = (const char *)*default_prefix_;

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  char prefix[84];
  memset(prefix, 0, 84);
  size_t prefix_len;

  uint8_t data[112 + 1];
  memset(data, 0, 112 + 1);
  size_t data_len = 0;

  if (!bcrypto_cashaddr_deserialize(&err, prefix, data,
                                    &data_len, default_prefix, addr)) {
    return Nan::ThrowError(bcrypto_cashaddr_strerror(err));
  }

  prefix_len = strlen((char *)&prefix[0]);

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();

  Nan::Set(ret, 0,
    Nan::New<v8::String>((char *)&prefix[0], prefix_len).ToLocalChecked());

  Nan::Set(ret, 1,
    Nan::CopyBuffer((char *)&data[0], data_len).ToLocalChecked());

  info.GetReturnValue().Set(ret);
}

NAN_METHOD(BCashAddr::Is) {
  if (info.Length() < 2)
    return Nan::ThrowError("cashaddr.is() requires arguments.");

  if (!info[0]->IsString())
    return Nan::ThrowTypeError("First argument must be a string.");

  if (!info[1]->IsString())
    return Nan::ThrowTypeError("Second argument must be a string.");

  Nan::Utf8String addr_(info[0]);
  const char *addr = (const char *)*addr_;

  Nan::Utf8String default_prefix_(info[1]);
  const char *default_prefix = (const char *)*default_prefix_;

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  bool result = bcrypto_cashaddr_is(&err, default_prefix, addr);

  info.GetReturnValue().Set(Nan::New<v8::Boolean>(result));
}

NAN_METHOD(BCashAddr::ConvertBits) {
  if (info.Length() < 4)
    return Nan::ThrowError("cashaddr.convertBits() requires arguments.");

  v8::Local<v8::Object> dbuf = info[0].As<v8::Object>();

  if (!node::Buffer::HasInstance(dbuf))
    return Nan::ThrowTypeError("First argument must be a buffer.");

  if (!info[1]->IsNumber())
    return Nan::ThrowTypeError("Second argument must be a number.");

  if (!info[2]->IsNumber())
    return Nan::ThrowTypeError("Third argument must be a number.");

  if (!info[3]->IsBoolean())
    return Nan::ThrowTypeError("Fourth argument must be a boolean.");

  const uint8_t *data = (uint8_t *)node::Buffer::Data(dbuf);
  size_t data_len = node::Buffer::Length(dbuf);
  int frombits = (int)Nan::To<int32_t>(info[1]).FromJust();
  int tobits = (int)Nan::To<int32_t>(info[2]).FromJust();
  int pad = (int)Nan::To<bool>(info[3]).FromJust();

  if (!(frombits == 8 && tobits == 5 && pad == 1)
      && !(frombits == 5 && tobits == 8 && pad == 0)) {
    return Nan::ThrowRangeError("Parameters out of range.");
  }

  size_t size = (data_len * frombits + (tobits - 1)) / tobits;

  if (pad)
    size += 1;

  uint8_t *out = (uint8_t *)malloc(size);
  size_t out_len = 0;

  if (!out)
    return Nan::ThrowError("Could not allocate.");

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  if (!bcrypto_cashaddr_convert_bits(&err, out, &out_len, tobits,
                                     data, data_len, frombits, pad)) {
    return Nan::ThrowError(bcrypto_cashaddr_strerror(err));
  }

  info.GetReturnValue().Set(
    Nan::NewBuffer((char *)out, out_len).ToLocalChecked());
}

NAN_METHOD(BCashAddr::Encode) {
  if (info.Length() < 3)
    return Nan::ThrowError("cashaddr.encode() requires arguments.");

  if (!info[0]->IsString())
    return Nan::ThrowTypeError("First argument must be a string.");

  Nan::Utf8String prefix_str(info[0]);

  if (!info[1]->IsNumber())
    return Nan::ThrowTypeError("Invalid cashaddr type.");

  v8::Local<v8::Object> hashbuf = info[2].As<v8::Object>();

  if (!node::Buffer::HasInstance(hashbuf))
    return Nan::ThrowTypeError("Third argument must be a buffer.");

  const char *prefix = (const char *)*prefix_str;
  int type = (int)Nan::To<int32_t>(info[1]).FromJust();
  double dtype = (double)Nan::To<double>(info[1]).FromJust();

  if (type < 0 || (type & 0x0f) != type || (double)type != dtype)
    return Nan::ThrowError("Invalid cashaddr type.");

  const uint8_t *hash = (uint8_t *)node::Buffer::Data(hashbuf);
  size_t hash_len = node::Buffer::Length(hashbuf);

  char output[197];
  memset(output, 0, 197);
  size_t olen = 0;

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  if (!bcrypto_cashaddr_encode(&err, output, prefix, type, hash, hash_len))
    return Nan::ThrowError(bcrypto_cashaddr_strerror(err));

  olen = strlen((char *)&output[0]);

  info.GetReturnValue().Set(
    Nan::New<v8::String>((char *)output, olen).ToLocalChecked());
}

NAN_METHOD(BCashAddr::Decode) {
  if (info.Length() < 2)
    return Nan::ThrowError("cashaddr.decode() requires arguments.");

  if (!info[0]->IsString())
    return Nan::ThrowTypeError("First argument must be a string.");

  if (!info[1]->IsString())
    return Nan::ThrowTypeError("Second argument must be a string.");

  Nan::Utf8String addr_(info[0]);
  const char *addr = (const char *)*addr_;

  Nan::Utf8String default_prefix_(info[1]);
  const char *default_prefix = (const char *)*default_prefix_;

  uint8_t hash[65];
  memset(hash, 0, 65);
  size_t hash_len;
  int type;
  char prefix[84];
  memset(prefix, 0, 84);
  size_t prefix_len;

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  if (!bcrypto_cashaddr_decode(&err, &type, hash, &hash_len,
                               prefix, default_prefix, addr)) {
    return Nan::ThrowError(bcrypto_cashaddr_strerror(err));
  }

  prefix_len = strlen((char *)&prefix[0]);

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();

  Nan::Set(ret, 0,
    Nan::New<v8::String>((char *)&prefix[0], prefix_len).ToLocalChecked());

  Nan::Set(ret, 1, Nan::New<v8::Number>(type));

  Nan::Set(ret, 2,
    Nan::CopyBuffer((char *)&hash[0], hash_len).ToLocalChecked());

  info.GetReturnValue().Set(ret);
}

NAN_METHOD(BCashAddr::Test) {
  if (info.Length() < 2)
    return Nan::ThrowError("cashaddr.test() requires arguments.");

  if (!info[0]->IsString())
    return Nan::ThrowTypeError("First argument must be a string.");

  if (!info[1]->IsString())
    return Nan::ThrowTypeError("Second argument must be a string.");

  Nan::Utf8String addr_(info[0]);
  const char *addr = (const char *)*addr_;

  Nan::Utf8String default_prefix_(info[1]);
  const char *default_prefix = (const char *)*default_prefix_;

  bcrypto_cashaddr_error err = BCRYPTO_CASHADDR_ERR_NULL;

  bool result = bcrypto_cashaddr_test(&err, default_prefix, addr);

  info.GetReturnValue().Set(Nan::New<v8::Boolean>(result));
}
