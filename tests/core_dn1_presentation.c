/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../mods/core-dn1/param_names.c"
const uint8_t fw_params[185 * 60] __attribute__((aligned(4))) = {0};
volatile uint32_t fw_slot_ids[79];
uint32_t core_param_name_call(uint32_t fn, int32_t id, int32_t field)
{ return (uint32_t)(uintptr_t)((const char *(*)(int32_t, int32_t))(uintptr_t)fn)(id, field); }
static int32_t active = 1, previous = 0, stuck, calls;
static const char *legacy_name(int32_t id, int32_t field)
{ return id == 16 && field == CORE_PARAM_SHORT ? "ATK" : 0; }
static const char *other_name(int32_t id, int32_t field)
{ return id == 16 && field == CORE_PARAM_LONG ? "Attack" : 0; }
static int32_t abstain(int32_t id) { (void)id; return -1; }
static int32_t visible(int32_t id) { return active && id == 108 ? 0 : -1; }
static int32_t text(int32_t id, int32_t value, char out[CORE_PARAM_TEXT])
{
    (void)value;
    if (id != 16) return 0;
    memset(out, 'x', CORE_PARAM_TEXT); return 1; /* Core must terminate even this formatter. */
}
static int32_t should_not_run(int32_t id, int32_t v, char out[CORE_PARAM_TEXT])
{ (void)id; (void)v; (void)out; assert(0); return 0; }
static const struct core_param_override legacy = {0, legacy_name}, other = {0, other_name};
static const struct core_param_presenter unknown = {0, should_not_run, 0};
static const struct core_param_presenter first = {&core_param_present_v1, 0, abstain};
static const struct core_param_presenter second = {&core_param_present_v1, text, visible};
const struct core_param_override *const core_param_override[] = {&legacy, &other, 0};
const struct core_param_presenter *const core_param_presenters[] = {&unknown, &first, &second, 0};
static uint32_t ptr(const void *p) { assert((uintptr_t)p <= UINT32_MAX); return (uintptr_t)p; }
static int32_t get(uint32_t owner, int32_t id) { (void)owner; assert(id == 52 || id == 61); return previous; }
static int32_t step(uint32_t owner, int32_t id, int32_t value, int32_t delta)
{ (void)owner; (void)id; calls++; return stuck ? value : value + delta; }
int main(void)
{
    assert(!strcmp((void *)(uintptr_t)core_param_name(16, CORE_PARAM_LONG), "Attack"));
    assert(!strcmp((void *)(uintptr_t)core_param_name(16, CORE_PARAM_SHORT), "ATK"));
    assert(!core_param_name(16, -1) && !core_param_value_text(17, 0));
    const char *out = (void *)(uintptr_t)core_param_value_text(16, 1);
    assert(strlen(out) == 15);
    assert(core_param_dest_visible(107) && !core_param_dest_visible(108));
    static int32_t ids[] = {16, 108, 107};
    static uint32_t vec[3], shared;
    vec[0] = ptr(ids); vec[1] = vec[2] = ptr(ids + 3); shared = ptr(vec);
    core_param_lfo_filter(&shared);
    assert(ids[0] == 16 && ids[1] == 107 && vec[1] == ptr(ids + 2) && vec[2] == ptr(ids + 3));
    static uint32_t vt[11], owner;
    owner = ptr(vt); vt[10] = ptr(get); vt[8] = ptr(step);
    fw_slot_ids[17] = 107; fw_slot_ids[18] = 108; fw_slot_ids[19] = 109;
    assert(core_param_lfo_step(ptr(&owner), 52, 18 * 256, 256) == 19 * 256 && calls == 1);
    previous = 17 * 256; stuck = 1;
    assert(core_param_lfo_step(ptr(&owner), 61, 18 * 256, -256) == previous);
    active = 0; int32_t before = calls;
    assert(core_param_lfo_step(ptr(&owner), 52, 18 * 256, 256) == 18 * 256 && calls == before);
    puts("PASS core presentation without machines: legacy ABI, independent providers, marker refusal, formatter termination, vector IDs/capacity and relative edits");
}
