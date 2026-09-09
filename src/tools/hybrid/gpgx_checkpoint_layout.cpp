#include "tools/hybrid/gpgx_checkpoint_layout.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace oasis::hybrid {
namespace {

using Byte = std::uint8_t;
using Word = std::uint16_t;
using Dword = std::uint32_t;
using HostPointer = void*;
using FunctionPointer = int (*)(int);

struct FmSlot {
    HostPointer dt;
    Byte ksr_capital;
    Dword ar;
    Dword d1r;
    Dword d2r;
    Dword rr;
    Byte ksr;
    Dword mul;
    Dword phase;
    std::int32_t incr;
    Byte state;
    Dword tl;
    std::int32_t volume;
    Dword sl;
    Dword vol_out;
    Byte eg_sh_ar;
    Byte eg_sel_ar;
    Byte eg_sh_d1r;
    Byte eg_sel_d1r;
    Byte eg_sh_d2r;
    Byte eg_sel_d2r;
    Byte eg_sh_rr;
    Byte eg_sel_rr;
    Byte ssg;
    Byte ssgn;
    Byte key;
    Dword am_mask;
};

struct FmChannel {
    FmSlot slot[4];
    Byte algorithm;
    Byte feedback;
    std::int32_t op1_out[2];
    std::int32_t* connect1;
    std::int32_t* connect3;
    std::int32_t* connect2;
    std::int32_t* connect4;
    std::int32_t* mem_connect;
    std::int32_t mem_value;
    std::int32_t pms;
    Byte ams;
    Dword fc;
    Byte kcode;
    Dword block_fnum;
};

struct FmState {
    Word address;
    Byte status;
    Dword mode;
    Byte fn_h;
    std::int32_t ta;
    std::int32_t tal;
    std::int32_t tac;
    std::int32_t tb;
    std::int32_t tbl;
    std::int32_t tbc;
    std::int32_t dt_tab[8][32];
};

struct FmThreeSlot {
    Dword fc[3];
    Byte fn_h;
    Byte kcode[3];
    Dword block_fnum[3];
    Byte key_csm;
};

struct FmOpn {
    FmState st;
    FmThreeSlot sl3;
    unsigned int pan[12];
    Dword eg_cnt;
    Dword eg_timer;
    Byte lfo_cnt;
    Dword lfo_timer;
    Dword lfo_timer_overflow;
    Dword lfo_am;
    Dword lfo_pm;
};

struct Ym2612 {
    FmChannel ch[6];
    Byte dac_en;
    std::int32_t dac_out;
    FmOpn opn;
};

struct Z80Regs {
    struct Pair { Dword d; } pc, sp, af, bc, de, hl, ix, iy, wz;
    Pair af2, bc2, de2, hl2;
    Byte r, r2, iff1, iff2, halt, im, i;
    Byte nmi_state, nmi_pending, irq_state, after_ei;
    Dword cycles;
    const void* daisy;
    FunctionPointer irq_callback;
};

static_assert(sizeof(void*) == 8);
static_assert(sizeof(FmSlot) == 80);
static_assert(sizeof(FmChannel) == 400);
static_assert(sizeof(FmState) == 1060);
static_assert(sizeof(FmThreeSlot) == 32);
static_assert(sizeof(FmOpn) == 1168);
static_assert(sizeof(Ym2612) == 3576);
static_assert(sizeof(Z80Regs) == 88);
static_assert(offsetof(FmSlot, dt) == 0);
static_assert(offsetof(FmChannel, connect1) == 336);
static_assert(offsetof(FmChannel, connect3) == 344);
static_assert(offsetof(FmChannel, connect2) == 352);
static_assert(offsetof(FmChannel, connect4) == 360);
static_assert(offsetof(FmChannel, mem_connect) == 368);
static_assert(offsetof(Z80Regs, daisy) == 72);
static_assert(offsetof(Z80Regs, irq_callback) == 80);

constexpr std::size_t kStateSize = 0xfd000;
constexpr std::size_t kYm2612Offset = 140652;
constexpr std::size_t kZ80Offset = 144504;
constexpr std::string_view kStateVersion = "GENPLUS-GX 1.7.6";

struct MemberRange { std::size_t offset; std::size_t size; };

class SpanBuilder {
public:
    void pointer(std::size_t offset, std::string_view name) {
        spans_.push_back({offset, sizeof(HostPointer),
                          CheckpointRepresentationKind::HostPointer, name});
    }

    void function_pointer(std::size_t offset, std::string_view name) {
        spans_.push_back({offset, sizeof(FunctionPointer),
                          CheckpointRepresentationKind::FunctionPointer, name});
    }

    void padding(std::size_t offset, std::size_t size, std::string_view name) {
        if (size != 0) {
            spans_.push_back({offset, size, CheckpointRepresentationKind::AbiPadding, name});
        }
    }

    void finish_struct(std::size_t base, std::size_t size,
                       std::vector<MemberRange> members, std::string_view name) {
        std::sort(members.begin(), members.end(),
                  [](const auto& left, const auto& right) { return left.offset < right.offset; });
        std::size_t cursor = 0;
        for (const auto& member : members) {
            if (member.offset < cursor || member.offset + member.size > size) {
                throw std::logic_error("invalid GPGX checkpoint layout member");
            }
            padding(base + cursor, member.offset - cursor, name);
            cursor = member.offset + member.size;
        }
        padding(base + cursor, size - cursor, name);
    }

    [[nodiscard]] std::vector<CheckpointRepresentationSpan> take() && {
        std::sort(spans_.begin(), spans_.end(), [](const auto& left, const auto& right) {
            return left.offset < right.offset;
        });
        for (std::size_t i = 1; i < spans_.size(); ++i) {
            if (spans_[i - 1].offset + spans_[i - 1].size > spans_[i].offset) {
                throw std::logic_error("overlapping GPGX checkpoint representation spans");
            }
        }
        return std::move(spans_);
    }

private:
    std::vector<CheckpointRepresentationSpan> spans_;
};

void add_slot(SpanBuilder& builder, std::size_t base, std::string_view name) {
    builder.pointer(base + offsetof(FmSlot, dt), name);
    builder.finish_struct(base, sizeof(FmSlot), {
        {offsetof(FmSlot, dt), sizeof(FmSlot::dt)},
        {offsetof(FmSlot, ksr_capital), sizeof(FmSlot::ksr_capital)},
        {offsetof(FmSlot, ar), sizeof(FmSlot::ar)}, {offsetof(FmSlot, d1r), sizeof(FmSlot::d1r)},
        {offsetof(FmSlot, d2r), sizeof(FmSlot::d2r)}, {offsetof(FmSlot, rr), sizeof(FmSlot::rr)},
        {offsetof(FmSlot, ksr), sizeof(FmSlot::ksr)}, {offsetof(FmSlot, mul), sizeof(FmSlot::mul)},
        {offsetof(FmSlot, phase), sizeof(FmSlot::phase)}, {offsetof(FmSlot, incr), sizeof(FmSlot::incr)},
        {offsetof(FmSlot, state), sizeof(FmSlot::state)}, {offsetof(FmSlot, tl), sizeof(FmSlot::tl)},
        {offsetof(FmSlot, volume), sizeof(FmSlot::volume)}, {offsetof(FmSlot, sl), sizeof(FmSlot::sl)},
        {offsetof(FmSlot, vol_out), sizeof(FmSlot::vol_out)},
        {offsetof(FmSlot, eg_sh_ar), 8}, {offsetof(FmSlot, ssg), 3},
        {offsetof(FmSlot, am_mask), sizeof(FmSlot::am_mask)}} , name);
}

void add_channel(SpanBuilder& builder, std::size_t base) {
    for (std::size_t slot = 0; slot < 4; ++slot) {
        add_slot(builder, base + slot * sizeof(FmSlot), "YM2612.CH[].SLOT[].representation");
    }
    builder.pointer(base + offsetof(FmChannel, connect1), "YM2612.CH[].connect1");
    builder.pointer(base + offsetof(FmChannel, connect3), "YM2612.CH[].connect3");
    builder.pointer(base + offsetof(FmChannel, connect2), "YM2612.CH[].connect2");
    builder.pointer(base + offsetof(FmChannel, connect4), "YM2612.CH[].connect4");
    builder.pointer(base + offsetof(FmChannel, mem_connect), "YM2612.CH[].mem_connect");
    builder.finish_struct(base, sizeof(FmChannel), {
        {offsetof(FmChannel, slot), sizeof(FmChannel::slot)},
        {offsetof(FmChannel, algorithm), sizeof(FmChannel::algorithm)},
        {offsetof(FmChannel, feedback), sizeof(FmChannel::feedback)},
        {offsetof(FmChannel, op1_out), sizeof(FmChannel::op1_out)},
        {offsetof(FmChannel, connect1), sizeof(FmChannel::connect1)},
        {offsetof(FmChannel, connect3), sizeof(FmChannel::connect3)},
        {offsetof(FmChannel, connect2), sizeof(FmChannel::connect2)},
        {offsetof(FmChannel, connect4), sizeof(FmChannel::connect4)},
        {offsetof(FmChannel, mem_connect), sizeof(FmChannel::mem_connect)},
        {offsetof(FmChannel, mem_value), sizeof(FmChannel::mem_value)},
        {offsetof(FmChannel, pms), sizeof(FmChannel::pms)}, {offsetof(FmChannel, ams), sizeof(FmChannel::ams)},
        {offsetof(FmChannel, fc), sizeof(FmChannel::fc)}, {offsetof(FmChannel, kcode), sizeof(FmChannel::kcode)},
        {offsetof(FmChannel, block_fnum), sizeof(FmChannel::block_fnum)}} , "YM2612.CH[].representation");
}

void add_ym2612(SpanBuilder& builder) {
    for (std::size_t channel = 0; channel < 6; ++channel) {
        add_channel(builder, kYm2612Offset + channel * sizeof(FmChannel));
    }
    builder.finish_struct(kYm2612Offset, sizeof(Ym2612), {
        {offsetof(Ym2612, ch), sizeof(Ym2612::ch)},
        {offsetof(Ym2612, dac_en), sizeof(Ym2612::dac_en)},
        {offsetof(Ym2612, dac_out), sizeof(Ym2612::dac_out)},
        {offsetof(Ym2612, opn), sizeof(Ym2612::opn)}} , "YM2612.representation");
}

void add_z80(SpanBuilder& builder) {
    builder.pointer(kZ80Offset + offsetof(Z80Regs, daisy), "Z80_Regs.daisy");
    builder.function_pointer(kZ80Offset + offsetof(Z80Regs, irq_callback), "Z80_Regs.irq_callback");
    builder.finish_struct(kZ80Offset, sizeof(Z80Regs), {
        {offsetof(Z80Regs, pc), offsetof(Z80Regs, r)},
        {offsetof(Z80Regs, r), 11}, {offsetof(Z80Regs, cycles), sizeof(Z80Regs::cycles)},
        {offsetof(Z80Regs, daisy), sizeof(Z80Regs::daisy)},
        {offsetof(Z80Regs, irq_callback), sizeof(Z80Regs::irq_callback)}} , "Z80_Regs.representation");
}

const GpgxCheckpointLayout& make_layout() {
    static const auto spans = [] {
        SpanBuilder builder;
        add_ym2612(builder);
        add_z80(builder);
        return std::move(builder).take();
    }();
    static const GpgxCheckpointLayout layout{
        kStateSize, kYm2612Offset, sizeof(Ym2612), kZ80Offset, sizeof(Z80Regs),
        kStateVersion, spans};
    return layout;
}

} // namespace

const GpgxCheckpointLayout& gpgx_checkpoint_layout() {
    return make_layout();
}

} // namespace oasis::hybrid
