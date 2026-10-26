// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <stddef.h>
#define RS_SLOTS 8
#define RS_TEXT 64
#define RS_SIZE 168
typedef struct { uint8_t b[RS_SIZE]; } rs_record;
typedef int (*rs_persist)(unsigned slot, unsigned bank, const rs_record *record, void *ctx);
typedef struct { rs_record records[RS_SLOTS]; uint8_t banks[RS_SLOTS]; } rs_store;
uint32_t rs_u32(const uint8_t *p);
uint16_t rs_u16(const uint8_t *p);
uint32_t rs_crc(const uint8_t *p, size_t n);
void rs_empty(rs_record *r);
void rs_init(rs_store *s);
int rs_valid(const rs_record *r, int allow_empty);
int rs_load(rs_store *s, unsigned slot, unsigned bank, const rs_record *r);
int rs_commit(rs_store *s, unsigned slot, uint32_t expected, const rs_record *next, rs_persist persist, void *ctx);
uint32_t rs_key(uint8_t ascii, uint8_t layout);
