/* Dynamic PLL for *UAE
*
* Copyright (C) 2021-2024 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This file is multi-licensed under the terms of the Mozilla Public
* License Version 2.0 as published by Mozilla Corporation and the
* GNU General Public License, version 2 or later, as published by the
* Free Software Foundation.
*
* MPL2: https://www.mozilla.org/en-US/MPL/2.0/
* GPL2: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

/*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

// This is an improved PLL designed, which is more like a real drive.
// This is roughly based on PLL design in scp.cpp (UAE) by Keir Fraser
// This also can record the data supplied and re-play it with a small amount
// of jitter.  This allows more than revolution of data to be instantly available
// which may help with unformatted areas and weak-bits, although I have switched it
// off as its quite experimental



#include "pll.h"

using namespace PLL;

#define CLOCK_CENTRE  2000   /* 2000ns = 2us */
#define CLOCK_MAX_ADJ 10     /* +/- 10% adjustment */
#define CLOCK_MIN ((CLOCK_CENTRE * (100 - CLOCK_MAX_ADJ)) / 100)
#define CLOCK_MAX ((CLOCK_CENTRE * (100 + CLOCK_MAX_ADJ)) / 100)

// Constructor
BridgePLL::BridgePLL(bool enabled, bool enableReplay) : m_enabled(enabled)
#ifdef ENABLE_REPLY
, m_useReplay(enableReplay)
#endif 
{
    reset();
}

// Reset the PLL
void BridgePLL::reset() {
    m_clock = CLOCK_CENTRE;
    m_nFluxSoFar = 0; 
    m_indexFound = false;
    m_latency = 0;
    m_totalRealFlux = 0;
    m_prevLatency = 0;
#ifdef ENABLE_REPLY
    m_fluxReplayData.clear();
#endif
}

// Prepare this to be used, by preparing the rotation extractor
void BridgePLL::prepareExtractor(bool isHD, const RotationExtractor::IndexSequenceMarker& indexSequence) {
#ifdef ENABLE_REPLY
    m_fluxReplayData.clear();
#endif
    m_extractor->reset(isHD);
    m_extractor->setIndexSequence(indexSequence);
}

// Re-plays the data back into the rotation extractor
void BridgePLL::rePlayData(const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
        std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation) {
#ifdef ENABLE_REPLY
    if (!m_useReplay) return;
    m_useReplay = false;

    m_clock = CLOCK_CENTRE;
    m_nFluxSoFar = 0;
    m_indexFound = false;
    m_latency = 0;
    m_totalRealFlux = 0;
    m_prevLatency = 0;

    m_extractor->reset(m_extractor->isHD());
    m_extractor->setIndexSequence(indexMarker);

    for (const ReplayData& data : m_fluxReplayData) {

        if (data.fluxTime>250) 
            submitFlux(data.fluxTime + (rand() % 100) - 50, data.isIndex);
        else submitFlux(data.fluxTime, data.isIndex);

        // Is it ready to extract?
        if (canExtract()) {
            unsigned int bits = 0;
            // Go!
            if (extractRotation(buffer, bits, maxBufferSize)) {
                if (!onRotation(buffer, bits)) {
                    // And if the callback says so we stop.                    
                    break;
                }
            }
        }        
    }
    m_useReplay = true;
#endif
}

// Submit flux to the PLL
void BridgePLL::submitFlux(uint32_t timeInNanoSeconds, bool isAtIndex) {
#ifdef ENABLE_REPLY
    if (m_useReplay) {
        m_fluxReplayData.push_back({ (uint32_t)timeInNanoSeconds, isAtIndex });
    }
#endif

    m_indexFound |= isAtIndex;

    // Add on the next flux
    m_nFluxSoFar += (int32_t)timeInNanoSeconds;
    m_totalRealFlux += timeInNanoSeconds;
    if (m_nFluxSoFar < (m_clock / 2)) return;

    // Work out how many zeros, and remaining flux
    const int clockedZeros = (m_nFluxSoFar - (m_clock / 2)) / m_clock;
    m_nFluxSoFar -= ((clockedZeros + 1) * m_clock);
    
    if (m_enabled) {
        m_latency += ((clockedZeros + 1) * m_clock);

        // PLL: Adjust clock frequency according to phase mismatch.
        if ((clockedZeros >= 1) && (clockedZeros <= 3)) {
            // In sync: adjust base clock by 10% of phase mismatch.
            m_clock += (m_nFluxSoFar / (int)(clockedZeros + 1)) / 10;
        }
        else {
            // Out of sync: adjust base clock towards centre.
            m_clock += (CLOCK_CENTRE - m_clock) / 10;
        }

        // Clamp the clock's adjustment range.
        m_clock = std::max(CLOCK_MIN, std::min(CLOCK_MAX, m_clock));

        // Authentic PLL: Do not snap the timing window to each flux transition.
        const uint32_t new_flux = m_nFluxSoFar / 2;
        m_latency += m_nFluxSoFar - new_flux;
        m_nFluxSoFar = new_flux;
        // This actually works ok if m_totalRealFlux is used instead of m_latency - m_prevLatency but we'll leave it there for good measure
        addToExtractor(clockedZeros, m_latency - m_prevLatency, m_totalRealFlux);
        m_prevLatency = m_latency;
    }
    else {
        m_nFluxSoFar = 0;
        // This actually works ok if m_totalRealFlux is used instead of m_latency - m_prevLatency but we'll leave it there for good measure
        addToExtractor(clockedZeros, m_totalRealFlux, m_totalRealFlux);
    }

    m_totalRealFlux = 0;
}

// Add data to the Rotation Extractor
void BridgePLL::addToExtractor(unsigned int numZeros, unsigned int pllTimeInNS, unsigned int realTimeInNS) {
    if (numZeros < 0) numZeros = 0;

    // More than 3 zeros.  This is not normal MFM, but is allowed
    if (numZeros >= 4) {
        unsigned int realTimePerBitcell = realTimeInNS / (numZeros + 1);
        unsigned int pllTimePerBitcell = pllTimeInNS / (numZeros + 1);

        // Based on the rules we can't output a sequence this big and times must be accurate so we output as many 000's as possible
        while (numZeros > 3) {
            RotationExtractor::MFMSequenceInfo sample;
            sample.mfm = RotationExtractor::MFMSequence::mfm000;
            sample.timeNS = realTimePerBitcell * 3;
            sample.pllTimeNS = pllTimePerBitcell * 3;
            realTimeInNS -= sample.timeNS;
            pllTimeInNS -= sample.pllTimeNS;
            m_extractor->submitSequence(sample, m_indexFound);
            m_indexFound = false;
            numZeros -= 3;
        }
    }

    RotationExtractor::MFMSequenceInfo sample;
    sample.mfm = (RotationExtractor::MFMSequence)numZeros;
    sample.timeNS = realTimeInNS;
    sample.pllTimeNS = pllTimeInNS;

    m_extractor->submitSequence(sample, m_indexFound);
    m_indexFound = false;
}
