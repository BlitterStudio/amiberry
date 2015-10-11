static uae_u32 blit_func_0x0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return 0;
}
static uae_u32 blit_func_0x1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | srcb | srcc);
}
static uae_u32 blit_func_0x2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & ~(srca | srcb));
}
static uae_u32 blit_func_0x3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | srcb);
}
static uae_u32 blit_func_0x4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & ~(srca | srcc));
}
static uae_u32 blit_func_0x5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | srcc);
}
static uae_u32 blit_func_0x6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca & (srcb ^ srcc));
}
static uae_u32 blit_func_0x7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | (srcb & srcc));
}
static uae_u32 blit_func_0x8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca & srcb & srcc);
}
static uae_u32 blit_func_0x9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | (srcb ^ srcc));
}
static uae_u32 blit_func_0xa(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca & srcc);
}
static uae_u32 blit_func_0xb(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | (srcb & ~srcc));
}
static uae_u32 blit_func_0xc(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca & srcb);
}
static uae_u32 blit_func_0xd(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca | (~srcb & srcc));
}
static uae_u32 blit_func_0xe(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca & (srcb | srcc));
}
static uae_u32 blit_func_0xf(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~srca;
}
static uae_u32 blit_func_0x10(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & ~(srcb | srcc));
}
static uae_u32 blit_func_0x11(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb | srcc);
}
static uae_u32 blit_func_0x12(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcb & (srca ^ srcc));
}
static uae_u32 blit_func_0x13(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb | (srca & srcc));
}
static uae_u32 blit_func_0x14(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcc & (srca ^ srcb));
}
static uae_u32 blit_func_0x15(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc | (srca & srcb));
}
static uae_u32 blit_func_0x16(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ ((srca & srcb) | (srcb ^ srcc)));
}
static uae_u32 blit_func_0x17(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ ((srca ^ srcb) & (srca ^ srcc)));
}
static uae_u32 blit_func_0x18(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcb) & (srca ^ srcc));
}
static uae_u32 blit_func_0x19(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srcc | (srca & srcb)));
}
static uae_u32 blit_func_0x1a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc | (srca & srcb)));
}
static uae_u32 blit_func_0x1b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc | ~(srca ^ srcb)));
}
static uae_u32 blit_func_0x1c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | (srca & srcc)));
}
static uae_u32 blit_func_0x1d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | ~(srca ^ srcc)));
}
static uae_u32 blit_func_0x1e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | srcc));
}
static uae_u32 blit_func_0x1f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & (srcb | srcc));
}
static uae_u32 blit_func_0x20(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & ~srcb & srcc);
}
static uae_u32 blit_func_0x21(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb | (srca ^ srcc));
}
static uae_u32 blit_func_0x22(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcb & srcc);
}
static uae_u32 blit_func_0x23(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb | (srca & ~srcc));
}
static uae_u32 blit_func_0x24(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcb) & (srcb ^ srcc));
}
static uae_u32 blit_func_0x25(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcc | (srca & srcb)));
}
static uae_u32 blit_func_0x26(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srcc | (srca & srcb)));
}
static uae_u32 blit_func_0x27(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcc & (srca ^ srcb)));
}
static uae_u32 blit_func_0x28(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & (srca ^ srcb));
}
static uae_u32 blit_func_0x29(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcb ^ (srcc | (srca & srcb)));
}
static uae_u32 blit_func_0x2a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & ~(srca & srcb));
}
static uae_u32 blit_func_0x2b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ ((srca ^ srcb) & (srcb ^ srcc)));
}
static uae_u32 blit_func_0x2c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca & (srcb | srcc)));
}
static uae_u32 blit_func_0x2d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | ~srcc));
}
static uae_u32 blit_func_0x2e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | (srca ^ srcc)));
}
static uae_u32 blit_func_0x2f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & (srcb | ~srcc));
}
static uae_u32 blit_func_0x30(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & ~srcb);
}
static uae_u32 blit_func_0x31(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb | (~srca & srcc));
}
static uae_u32 blit_func_0x32(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcb & (srca | srcc));
}
static uae_u32 blit_func_0x33(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~srcb;
}
static uae_u32 blit_func_0x34(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca | (srcb & srcc)));
}
static uae_u32 blit_func_0x35(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca | ~(srcb ^ srcc)));
}
static uae_u32 blit_func_0x36(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca | srcc));
}
static uae_u32 blit_func_0x37(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb & (srca | srcc));
}
static uae_u32 blit_func_0x38(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb & (srca | srcc)));
}
static uae_u32 blit_func_0x39(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca | ~srcc));
}
static uae_u32 blit_func_0x3a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca | (srcb ^ srcc)));
}
static uae_u32 blit_func_0x3b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb & (srca | ~srcc));
}
static uae_u32 blit_func_0x3c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcb);
}
static uae_u32 blit_func_0x3d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | ~(srca | srcc)));
}
static uae_u32 blit_func_0x3e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb | (srca ^ (srca | srcc))));
}
static uae_u32 blit_func_0x3f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & srcb);
}
static uae_u32 blit_func_0x40(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & srcb & ~srcc);
}
static uae_u32 blit_func_0x41(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc | (srca ^ srcb));
}
static uae_u32 blit_func_0x42(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcc) & (srcb ^ srcc));
}
static uae_u32 blit_func_0x43(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb | (srca & srcc)));
}
static uae_u32 blit_func_0x44(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & ~srcc);
}
static uae_u32 blit_func_0x45(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc | (srca & ~srcb));
}
static uae_u32 blit_func_0x46(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srcb | (srca & srcc)));
}
static uae_u32 blit_func_0x47(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcb & (srca ^ srcc)));
}
static uae_u32 blit_func_0x48(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & (srca ^ srcc));
}
static uae_u32 blit_func_0x49(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcc ^ (srcb | (srca & srcc)));
}
static uae_u32 blit_func_0x4a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca & (srcb | srcc)));
}
static uae_u32 blit_func_0x4b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb | srcc));
}
static uae_u32 blit_func_0x4c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & ~(srca & srcc));
}
static uae_u32 blit_func_0x4d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ ((srca ^ srcb) | ~(srca ^ srcc)));
}
static uae_u32 blit_func_0x4e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc | (srca ^ srcb)));
}
static uae_u32 blit_func_0x4f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & (~srcb | srcc));
}
static uae_u32 blit_func_0x50(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & ~srcc);
}
static uae_u32 blit_func_0x51(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc | (~srca & srcb));
}
static uae_u32 blit_func_0x52(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca | (srcb & srcc)));
}
static uae_u32 blit_func_0x53(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srca & (srcb ^ srcc)));
}
static uae_u32 blit_func_0x54(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcc & (srca | srcb));
}
static uae_u32 blit_func_0x55(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~srcc;
}
static uae_u32 blit_func_0x56(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca | srcb));
}
static uae_u32 blit_func_0x57(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc & (srca | srcb));
}
static uae_u32 blit_func_0x58(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x59(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca | ~srcb));
}
static uae_u32 blit_func_0x5a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcc);
}
static uae_u32 blit_func_0x5b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc | ~(srca | srcb)));
}
static uae_u32 blit_func_0x5c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca | (srcb ^ srcc)));
}
static uae_u32 blit_func_0x5d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc & (srca | ~srcb));
}
static uae_u32 blit_func_0x5e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc | (srca ^ (srca | srcb))));
}
static uae_u32 blit_func_0x5f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & srcc);
}
static uae_u32 blit_func_0x60(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & (srcb ^ srcc));
}
static uae_u32 blit_func_0x61(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ srcc ^ (srca | (srcb & srcc)));
}
static uae_u32 blit_func_0x62(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srcb & (srca | srcc)));
}
static uae_u32 blit_func_0x63(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srca | srcc));
}
static uae_u32 blit_func_0x64(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x65(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (~srca | srcb));
}
static uae_u32 blit_func_0x66(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ srcc);
}
static uae_u32 blit_func_0x67(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srcc | ~(srca | srcb)));
}
static uae_u32 blit_func_0x68(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca & srcb) ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x69(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcb ^ srcc);
}
static uae_u32 blit_func_0x6a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca & srcb));
}
static uae_u32 blit_func_0x6b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcb ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x6c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca & srcc));
}
static uae_u32 blit_func_0x6d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcc ^ (srcb & (srca | srcc)));
}
static uae_u32 blit_func_0x6e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((~srca & srcb) | (srcb ^ srcc));
}
static uae_u32 blit_func_0x6f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca | (srcb ^ srcc));
}
static uae_u32 blit_func_0x70(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & ~(srcb & srcc));
}
static uae_u32 blit_func_0x71(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ ((srca ^ srcb) | (srca ^ srcc)));
}
static uae_u32 blit_func_0x72(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srcc | (srca ^ srcb)));
}
static uae_u32 blit_func_0x73(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb & (~srca | srcc));
}
static uae_u32 blit_func_0x74(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srcb | (srca ^ srcc)));
}
static uae_u32 blit_func_0x75(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc & (~srca | srcb));
}
static uae_u32 blit_func_0x76(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srcc | (srca ^ (srca & srcb))));
}
static uae_u32 blit_func_0x77(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb & srcc);
}
static uae_u32 blit_func_0x78(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb & srcc));
}
static uae_u32 blit_func_0x79(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ srcc ^ (srca & (srcb | srcc)));
}
static uae_u32 blit_func_0x7a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca & ~srcb) | (srca ^ srcc));
}
static uae_u32 blit_func_0x7b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcb | (srca ^ srcc));
}
static uae_u32 blit_func_0x7c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcb) | (srca & ~srcc));
}
static uae_u32 blit_func_0x7d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcc | (srca ^ srcb));
}
static uae_u32 blit_func_0x7e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcb) | (srca ^ srcc));
}
static uae_u32 blit_func_0x7f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & srcb & srcc);
}
static uae_u32 blit_func_0x80(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & srcb & srcc);
}
static uae_u32 blit_func_0x81(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~((srca ^ srcb) | (srca ^ srcc));
}
static uae_u32 blit_func_0x82(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & ~(srca ^ srcb));
}
static uae_u32 blit_func_0x83(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb | (srca & ~srcc)));
}
static uae_u32 blit_func_0x84(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & ~(srca ^ srcc));
}
static uae_u32 blit_func_0x85(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcc | (srca & ~srcb)));
}
static uae_u32 blit_func_0x86(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ srcc ^ (srca & (srcb | srcc)));
}
static uae_u32 blit_func_0x87(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcb & srcc));
}
static uae_u32 blit_func_0x88(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & srcc);
}
static uae_u32 blit_func_0x89(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srcc & (~srca | srcb)));
}
static uae_u32 blit_func_0x8a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & (~srca | srcb));
}
static uae_u32 blit_func_0x8b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb | (srca ^ srcc)));
}
static uae_u32 blit_func_0x8c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & (~srca | srcc));
}
static uae_u32 blit_func_0x8d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcc | (srca ^ srcb)));
}
static uae_u32 blit_func_0x8e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ ((srca ^ srcb) | (srca ^ srcc)));
}
static uae_u32 blit_func_0x8f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca | (srcb & srcc));
}
static uae_u32 blit_func_0x90(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & ~(srcb ^ srcc));
}
static uae_u32 blit_func_0x91(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srcc | (~srca & srcb)));
}
static uae_u32 blit_func_0x92(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcc ^ (srcb & (srca | srcc)));
}
static uae_u32 blit_func_0x93(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srca & srcc));
}
static uae_u32 blit_func_0x94(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcb ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x95(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc ^ (srca & srcb));
}
static uae_u32 blit_func_0x96(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcb ^ srcc);
}
static uae_u32 blit_func_0x97(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcb ^ (srcc | ~(srca | srcb)));
}
static uae_u32 blit_func_0x98(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x99(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ srcc);
}
static uae_u32 blit_func_0x9a(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca & ~srcb));
}
static uae_u32 blit_func_0x9b(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0x9c(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca & ~srcc));
}
static uae_u32 blit_func_0x9d(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc ^ (srcb & (srca | srcc)));
}
static uae_u32 blit_func_0x9e(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ srcc ^ (srca | (srcb & srcc)));
}
static uae_u32 blit_func_0x9f(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca & (srcb ^ srcc));
}
static uae_u32 blit_func_0xa0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & srcc);
}
static uae_u32 blit_func_0xa1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcc & (srca | ~srcb)));
}
static uae_u32 blit_func_0xa2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & (srca | ~srcb));
}
static uae_u32 blit_func_0xa3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srca | (srcb ^ srcc)));
}
static uae_u32 blit_func_0xa4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcc & (srca | srcb)));
}
static uae_u32 blit_func_0xa5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcc);
}
static uae_u32 blit_func_0xa6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (~srca & srcb));
}
static uae_u32 blit_func_0xa7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcc & (srca | srcb)));
}
static uae_u32 blit_func_0xa8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc & (srca | srcb));
}
static uae_u32 blit_func_0xa9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc ^ (srca | srcb));
}
static uae_u32 blit_func_0xaa(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return srcc;
}
static uae_u32 blit_func_0xab(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | ~(srca | srcb));
}
static uae_u32 blit_func_0xac(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srca & (srcb ^ srcc)));
}
static uae_u32 blit_func_0xad(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc ^ (srca | (srcb & srcc)));
}
static uae_u32 blit_func_0xae(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | (~srca & srcb));
}
static uae_u32 blit_func_0xaf(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca | srcc);
}
static uae_u32 blit_func_0xb0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & (~srcb | srcc));
}
static uae_u32 blit_func_0xb1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcc | (srca ^ srcb)));
}
static uae_u32 blit_func_0xb2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ ((srca ^ srcc) & (srcb ^ srcc)));
}
static uae_u32 blit_func_0xb3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcb | (srca & srcc));
}
static uae_u32 blit_func_0xb4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb & ~srcc));
}
static uae_u32 blit_func_0xb5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc ^ (srca & (srcb | srcc)));
}
static uae_u32 blit_func_0xb6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcc ^ (srcb | (srca & srcc)));
}
static uae_u32 blit_func_0xb7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb & (srca ^ srcc));
}
static uae_u32 blit_func_0xb8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcb & (srca ^ srcc)));
}
static uae_u32 blit_func_0xb9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc ^ (srcb | (srca & srcc)));
}
static uae_u32 blit_func_0xba(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | (srca & ~srcb));
}
static uae_u32 blit_func_0xbb(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcb | srcc);
}
static uae_u32 blit_func_0xbc(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcb) | (srca & srcc));
}
static uae_u32 blit_func_0xbd(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca ^ srcb) | ~(srca ^ srcc));
}
static uae_u32 blit_func_0xbe(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | (srca ^ srcb));
}
static uae_u32 blit_func_0xbf(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | ~(srca & srcb));
}
static uae_u32 blit_func_0xc0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & srcb);
}
static uae_u32 blit_func_0xc1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb & (srca | ~srcc)));
}
static uae_u32 blit_func_0xc2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb & (srca | srcc)));
}
static uae_u32 blit_func_0xc3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ srcb);
}
static uae_u32 blit_func_0xc4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & (srca | ~srcc));
}
static uae_u32 blit_func_0xc5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srca | (srcb ^ srcc)));
}
static uae_u32 blit_func_0xc6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (~srca & srcc));
}
static uae_u32 blit_func_0xc7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcb & (srca | srcc)));
}
static uae_u32 blit_func_0xc8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb & (srca | srcc));
}
static uae_u32 blit_func_0xc9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srca | srcc));
}
static uae_u32 blit_func_0xca(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srca & (srcb ^ srcc)));
}
static uae_u32 blit_func_0xcb(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srca | (srcb & srcc)));
}
static uae_u32 blit_func_0xcc(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return srcb;
}
static uae_u32 blit_func_0xcd(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | ~(srca | srcc));
}
static uae_u32 blit_func_0xce(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | (~srca & srcc));
}
static uae_u32 blit_func_0xcf(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca | srcb);
}
static uae_u32 blit_func_0xd0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & (srcb | ~srcc));
}
static uae_u32 blit_func_0xd1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcb | (srca ^ srcc)));
}
static uae_u32 blit_func_0xd2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (~srcb & srcc));
}
static uae_u32 blit_func_0xd3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srca & (srcb | srcc)));
}
static uae_u32 blit_func_0xd4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ ((srca ^ srcb) & (srcb ^ srcc)));
}
static uae_u32 blit_func_0xd5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srcc | (srca & srcb));
}
static uae_u32 blit_func_0xd6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcb ^ (srcc | (srca & srcb)));
}
static uae_u32 blit_func_0xd7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcc & (srca ^ srcb));
}
static uae_u32 blit_func_0xd8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ (srcc & (srca ^ srcb)));
}
static uae_u32 blit_func_0xd9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srcb ^ (srcc | (srca & srcb)));
}
static uae_u32 blit_func_0xda(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca & srcb) | (srca ^ srcc));
}
static uae_u32 blit_func_0xdb(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~((srca ^ srcb) & (srcb ^ srcc));
}
static uae_u32 blit_func_0xdc(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | (srca & ~srcc));
}
static uae_u32 blit_func_0xdd(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | ~srcc);
}
static uae_u32 blit_func_0xde(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | (srca ^ srcc));
}
static uae_u32 blit_func_0xdf(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | ~(srca & srcc));
}
static uae_u32 blit_func_0xe0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca & (srcb | srcc));
}
static uae_u32 blit_func_0xe1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcb | srcc));
}
static uae_u32 blit_func_0xe2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc ^ (srcb & (srca ^ srcc)));
}
static uae_u32 blit_func_0xe3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcb | (srca & srcc)));
}
static uae_u32 blit_func_0xe4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb ^ (srcc & (srca ^ srcb)));
}
static uae_u32 blit_func_0xe5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~(srca ^ (srcc | (srca & srcb)));
}
static uae_u32 blit_func_0xe6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ((srca & srcb) | (srcb ^ srcc));
}
static uae_u32 blit_func_0xe7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return ~((srca ^ srcb) & (srca ^ srcc));
}
static uae_u32 blit_func_0xe8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ ((srca ^ srcb) & (srca ^ srcc)));
}
static uae_u32 blit_func_0xe9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca ^ srcb ^ (~srcc | (srca & srcb)));
}
static uae_u32 blit_func_0xea(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | (srca & srcb));
}
static uae_u32 blit_func_0xeb(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcc | ~(srca ^ srcb));
}
static uae_u32 blit_func_0xec(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | (srca & srcc));
}
static uae_u32 blit_func_0xed(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | ~(srca ^ srcc));
}
static uae_u32 blit_func_0xee(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srcb | srcc);
}
static uae_u32 blit_func_0xef(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (~srca | srcb | srcc);
}
static uae_u32 blit_func_0xf0(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return srca;
}
static uae_u32 blit_func_0xf1(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | ~(srcb | srcc));
}
static uae_u32 blit_func_0xf2(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | (~srcb & srcc));
}
static uae_u32 blit_func_0xf3(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | ~srcb);
}
static uae_u32 blit_func_0xf4(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | (srcb & ~srcc));
}
static uae_u32 blit_func_0xf5(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | ~srcc);
}
static uae_u32 blit_func_0xf6(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | (srcb ^ srcc));
}
static uae_u32 blit_func_0xf7(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | ~(srcb & srcc));
}
static uae_u32 blit_func_0xf8(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | (srcb & srcc));
}
static uae_u32 blit_func_0xf9(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | ~(srcb ^ srcc));
}
static uae_u32 blit_func_0xfa(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | srcc);
}
static uae_u32 blit_func_0xfb(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | ~srcb | srcc);
}
static uae_u32 blit_func_0xfc(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | srcb);
}
static uae_u32 blit_func_0xfd(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | srcb | ~srcc);
}
static uae_u32 blit_func_0xfe(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return (srca | srcb | srcc);
}
static uae_u32 blit_func_0xff(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc) {
	return 0xFFFFFFFF;
}

typedef uae_u32 (*blitter_op_func)(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc);

static blitter_op_func blitter_func_tbl[256]={
  blit_func_0x0, blit_func_0x1, blit_func_0x2, blit_func_0x3, 
  blit_func_0x4, blit_func_0x5, blit_func_0x6, blit_func_0x7, 
  blit_func_0x8, blit_func_0x9, blit_func_0xa, blit_func_0xb, 
  blit_func_0xc, blit_func_0xd, blit_func_0xe, blit_func_0xf, 

  blit_func_0x10, blit_func_0x11, blit_func_0x12, blit_func_0x13, 
  blit_func_0x14, blit_func_0x15, blit_func_0x16, blit_func_0x17, 
  blit_func_0x18, blit_func_0x19, blit_func_0x1a, blit_func_0x1b, 
  blit_func_0x1c, blit_func_0x1d, blit_func_0x1e, blit_func_0x1f, 

  blit_func_0x20, blit_func_0x21, blit_func_0x22, blit_func_0x23, 
  blit_func_0x24, blit_func_0x25, blit_func_0x26, blit_func_0x27, 
  blit_func_0x28, blit_func_0x29, blit_func_0x2a, blit_func_0x2b, 
  blit_func_0x2c, blit_func_0x2d, blit_func_0x2e, blit_func_0x2f, 

  blit_func_0x30, blit_func_0x31, blit_func_0x32, blit_func_0x33, 
  blit_func_0x34, blit_func_0x35, blit_func_0x36, blit_func_0x37, 
  blit_func_0x38, blit_func_0x39, blit_func_0x3a, blit_func_0x3b, 
  blit_func_0x3c, blit_func_0x3d, blit_func_0x3e, blit_func_0x3f, 

  blit_func_0x40, blit_func_0x41, blit_func_0x42, blit_func_0x43, 
  blit_func_0x44, blit_func_0x45, blit_func_0x46, blit_func_0x47, 
  blit_func_0x48, blit_func_0x49, blit_func_0x4a, blit_func_0x4b, 
  blit_func_0x4c, blit_func_0x4d, blit_func_0x4e, blit_func_0x4f, 

  blit_func_0x50, blit_func_0x51, blit_func_0x52, blit_func_0x53, 
  blit_func_0x54, blit_func_0x55, blit_func_0x56, blit_func_0x57, 
  blit_func_0x58, blit_func_0x59, blit_func_0x5a, blit_func_0x5b, 
  blit_func_0x5c, blit_func_0x5d, blit_func_0x5e, blit_func_0x5f, 

  blit_func_0x60, blit_func_0x61, blit_func_0x62, blit_func_0x63, 
  blit_func_0x64, blit_func_0x65, blit_func_0x66, blit_func_0x67, 
  blit_func_0x68, blit_func_0x69, blit_func_0x6a, blit_func_0x6b, 
  blit_func_0x6c, blit_func_0x6d, blit_func_0x6e, blit_func_0x6f, 

  blit_func_0x70, blit_func_0x71, blit_func_0x72, blit_func_0x73, 
  blit_func_0x74, blit_func_0x75, blit_func_0x76, blit_func_0x77, 
  blit_func_0x78, blit_func_0x79, blit_func_0x7a, blit_func_0x7b, 
  blit_func_0x7c, blit_func_0x7d, blit_func_0x7e, blit_func_0x7f, 

  blit_func_0x80, blit_func_0x81, blit_func_0x82, blit_func_0x83, 
  blit_func_0x84, blit_func_0x85, blit_func_0x86, blit_func_0x87, 
  blit_func_0x88, blit_func_0x89, blit_func_0x8a, blit_func_0x8b, 
  blit_func_0x8c, blit_func_0x8d, blit_func_0x8e, blit_func_0x8f, 

  blit_func_0x90, blit_func_0x91, blit_func_0x92, blit_func_0x93, 
  blit_func_0x94, blit_func_0x95, blit_func_0x96, blit_func_0x97, 
  blit_func_0x98, blit_func_0x99, blit_func_0x9a, blit_func_0x9b, 
  blit_func_0x9c, blit_func_0x9d, blit_func_0x9e, blit_func_0x9f, 

  blit_func_0xa0, blit_func_0xa1, blit_func_0xa2, blit_func_0xa3, 
  blit_func_0xa4, blit_func_0xa5, blit_func_0xa6, blit_func_0xa7, 
  blit_func_0xa8, blit_func_0xa9, blit_func_0xaa, blit_func_0xab, 
  blit_func_0xac, blit_func_0xad, blit_func_0xae, blit_func_0xaf, 

  blit_func_0xb0, blit_func_0xb1, blit_func_0xb2, blit_func_0xb3, 
  blit_func_0xb4, blit_func_0xb5, blit_func_0xb6, blit_func_0xb7, 
  blit_func_0xb8, blit_func_0xb9, blit_func_0xba, blit_func_0xbb, 
  blit_func_0xbc, blit_func_0xbd, blit_func_0xbe, blit_func_0xbf, 

  blit_func_0xc0, blit_func_0xc1, blit_func_0xc2, blit_func_0xc3, 
  blit_func_0xc4, blit_func_0xc5, blit_func_0xc6, blit_func_0xc7, 
  blit_func_0xc8, blit_func_0xc9, blit_func_0xca, blit_func_0xcb, 
  blit_func_0xcc, blit_func_0xcd, blit_func_0xce, blit_func_0xcf, 

  blit_func_0xd0, blit_func_0xd1, blit_func_0xd2, blit_func_0xd3, 
  blit_func_0xd4, blit_func_0xd5, blit_func_0xd6, blit_func_0xd7, 
  blit_func_0xd8, blit_func_0xd9, blit_func_0xda, blit_func_0xdb, 
  blit_func_0xdc, blit_func_0xdd, blit_func_0xde, blit_func_0xdf, 

  blit_func_0xe0, blit_func_0xe1, blit_func_0xe2, blit_func_0xe3, 
  blit_func_0xe4, blit_func_0xe5, blit_func_0xe6, blit_func_0xe7, 
  blit_func_0xe8, blit_func_0xe9, blit_func_0xea, blit_func_0xeb, 
  blit_func_0xec, blit_func_0xed, blit_func_0xee, blit_func_0xef, 

  blit_func_0xf0, blit_func_0xf1, blit_func_0xf2, blit_func_0xf3, 
  blit_func_0xf4, blit_func_0xf5, blit_func_0xf6, blit_func_0xf7, 
  blit_func_0xf8, blit_func_0xf9, blit_func_0xfa, blit_func_0xfb, 
  blit_func_0xfc, blit_func_0xfd, blit_func_0xfe, blit_func_0xff 

};
