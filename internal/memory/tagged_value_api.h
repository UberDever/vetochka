#ifndef __TAGGED_VALUE_API_H__
#define __TAGGED_VALUE_API_H__

#include "internal/domain/domain_api.h"

static inline u8 _tv_get_tag(uint tagged_value) {
  return (u8)(tagged_value & 0xF);
}

static inline sint _tv_get_payload_signed(uint tagged_value) {
  return (sint)(tagged_value & ~0xFULL) >> 4;
}

static inline uint _tv_get_payload_unsigned(uint tagged_value) {
  return tagged_value >> 4;
}

static inline uint _tv_set_tag(uint tagged_value, u8 new_tag) {
  return (tagged_value & ~0xFULL) | (new_tag & 0xF);
}

static inline uint _tv_set_payload_signed(uint tagged_value, sint new_payload) {
  uint tag_bits = tagged_value & 0xF;
  return ((uint)new_payload << 4) | tag_bits;
}

static inline uint _tv_set_payload_unsigned(uint tagged_value, uint new_payload) {
  uint tag_bits = tagged_value & 0xF;
  return (new_payload << 4) | tag_bits;
}

static inline uint _tv_new_tagged_value_signed(u8 tag, sint payload) {
  return ((uint)payload << 4) | (tag & 0xF);
}

static inline uint _tv_new_tagged_value_unsigned(u8 tag, uint payload) {
  return (payload << 4) | (tag & 0xF);
}

#endif
