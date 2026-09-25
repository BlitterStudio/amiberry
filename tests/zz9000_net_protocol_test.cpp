#include "zz9000_net_protocol.h"

#include <cassert>
#include <cstdint>

using zz9000_net::RxQueue;

static void test_frame_layout_and_ack()
{
    RxQueue rx;
    const uint8_t frame[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                             0x68, 0x82, 0xf2, 0x00, 0x01, 0x00,
                             0x08, 0x06};
    assert(rx.enqueue(frame, sizeof frame));
    assert(rx.ready() == 1);
    assert(rx.read(0) == 0 && rx.read(1) == sizeof frame);
    assert(rx.read(2) == 0 && rx.read(3) == 2);
    for (unsigned i = 0; i < sizeof frame; ++i)
        assert(rx.read(i + 4) == frame[i]);
    assert(!rx.accept(0));
    assert(!rx.accept(3));
    assert(rx.ready() == 1);
    assert(rx.accept(2));
    assert(rx.ready() == 0);
    for (unsigned i = 0; i < 32; ++i)
        assert(rx.read(i) == 0);
}

static void test_queue_order_and_legacy_ack()
{
    RxQueue rx;
    const uint8_t frame[14] = {};
    assert(rx.enqueue(frame, sizeof frame));
    assert(rx.enqueue(frame, sizeof frame));
    assert(rx.read(3) == 2);
    assert(rx.accept(1));
    assert(rx.read(3) == 3);
    assert(rx.accept(3));
    assert(rx.ready() == 0);
}

static void test_bounds_and_overflow()
{
    RxQueue rx;
    const uint8_t short_frame[13] = {};
    const uint8_t frame[1518] = {};
    assert(!rx.enqueue(short_frame, sizeof short_frame));
    assert(!rx.enqueue(frame, 1519));
    for (unsigned i = 0; i < RxQueue::capacity; ++i)
        assert(rx.enqueue(frame, sizeof frame));
    assert(!rx.enqueue(frame, sizeof frame));
    assert(rx.ready() == RxQueue::capacity);
    assert(rx.dropped() == 1);
    assert(rx.read(4 + sizeof frame) == 0);
    assert(rx.accept(2));
    assert(rx.ready() == RxQueue::capacity - 1);
    for (uint16_t serial = 3; serial <= RxQueue::capacity + 1; ++serial)
        assert(rx.accept(serial));
    assert(rx.enqueue(frame, sizeof frame));
    assert(rx.read(2) == 0 && rx.read(3) == RxQueue::capacity + 3);
}

static void test_serial_wrap()
{
    RxQueue rx;
    const uint8_t frame[14] = {};
    for (uint32_t serial = 2; serial <= 0xffff; ++serial) {
        assert(rx.enqueue(frame, sizeof frame));
        assert(rx.accept(static_cast<uint16_t>(serial)));
    }
    assert(rx.enqueue(frame, sizeof frame));
    assert(rx.read(2) == 0 && rx.read(3) == 2);
}

int main()
{
    test_frame_layout_and_ack();
    test_queue_order_and_legacy_ack();
    test_bounds_and_overflow();
    test_serial_wrap();
}
