// Mailbox discovery shared by the ZZ9000 SDK tests.
#ifndef AMIBERRY_ZZ9000_SDK_TEST_MAILBOX_H
#define AMIBERRY_ZZ9000_SDK_TEST_MAILBOX_H

#include "zz9000_sdk.h"

#include <cassert>
#include <cstdint>

// The ARM address the card publishes in its SDK_MAILBOX_HI/LO registers.
inline uint32_t zz9000_test_mailbox_arm(const zz9000_sdk::Engine &sdk)
{
	return (static_cast<uint32_t>(sdk.read_register(0x104)) << 16) |
		sdk.read_register(0x106);
}

// Board offset of a main-aperture ARM address, mapped as zz9k.library does.
inline uint32_t zz9000_test_arm_to_board(uint32_t arm)
{
	assert(arm >= 0x200000);
	return arm - 0x200000 + 0x10000;
}

inline uint8_t *zz9000_test_mailbox(const zz9000_sdk::Engine &sdk, uint8_t *board)
{
	return board + zz9000_test_arm_to_board(zz9000_test_mailbox_arm(sdk));
}

#endif
