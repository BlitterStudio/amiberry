#ifndef AMIBERRY_ZZ9000_NET_PROTOCOL_H
#define AMIBERRY_ZZ9000_NET_PROTOCOL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>

namespace zz9000_net {

// ZZ9000 firmware presents a four-byte size/serial header followed by one
// complete Ethernet frame. The serial 0 means empty and 1 is reserved for
// legacy bare-advance acknowledgements.
class RxQueue {
public:
    static constexpr size_t max_frame = 1518;
    static constexpr size_t capacity = 128;

    bool enqueue(const uint8_t *frame, size_t length)
    {
        if (!frame || length < 14 || length > max_frame)
            return false;
        if (frames_.size() == capacity) {
            record_drop();
            return false;
        }
        Frame entry{};
        entry.length = static_cast<uint16_t>(length);
        entry.serial = next_serial_;
        std::memcpy(entry.bytes.data(), frame, length);
        frames_.push_back(entry);
        advance_serial();
        return true;
    }

    bool accept(uint16_t serial)
    {
        if (frames_.empty() || !serial ||
            (serial != 1 && serial != frames_.front().serial))
            return false;
        frames_.pop_front();
        return true;
    }

    uint8_t read(size_t offset) const
    {
        if (frames_.empty())
            return 0;
        const Frame &frame = frames_.front();
        switch (offset) {
            case 0: return static_cast<uint8_t>(frame.length >> 8);
            case 1: return static_cast<uint8_t>(frame.length);
            case 2: return static_cast<uint8_t>(frame.serial >> 8);
            case 3: return static_cast<uint8_t>(frame.serial);
            default:
                offset -= 4;
                return offset < frame.length ? frame.bytes[offset] : 0;
        }
    }

    size_t ready() const { return frames_.size(); }
    uint32_t dropped() const { return dropped_; }
    void record_drop(uint32_t count = 1)
    {
        dropped_ += count;
        for (uint32_t i = 0; i < count; ++i)
            advance_serial();
    }
    void reset()
    {
        frames_.clear();
        dropped_ = 0;
        next_serial_ = 2;
    }

private:
    struct Frame {
        uint16_t length = 0;
        uint16_t serial = 0;
        std::array<uint8_t, max_frame> bytes{};
    };

    void advance_serial()
    {
        next_serial_ = static_cast<uint16_t>(next_serial_ + 1);
        if (next_serial_ < 2)
            next_serial_ = 2;
    }

    std::deque<Frame> frames_;
    uint32_t dropped_ = 0;
    uint16_t next_serial_ = 2;
};

} // namespace zz9000_net

#endif
