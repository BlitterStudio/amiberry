#ifndef FLOPPYBRIDGE_PLL
#define FLOPPYBRIDGE_PLL
/* Dynamic PLL for *UAE
*
* Copyright (C) 2021-2022 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, see http://www.gnu.org/licenses/
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


#ifndef _WIN32
#include <dlfcn.h>
#endif
#include <stdint.h>
#include "RotationExtractor.h"
#include <queue>
#include <functional>



namespace PLL {

	class BridgePLL {
	private:
#ifdef ENABLE_REPLY
		struct ReplayData {
			uint32_t fluxTime;
			bool isIndex;
		};
#endif
		const bool m_enabled;

		// Rotation extractor
		RotationExtractor* m_extractor = nullptr;

		// Clock
		int32_t m_clock = 0;
		int32_t m_latency = 0;
		int32_t m_prevLatency = 0;
		int32_t m_totalRealFlux = 0;

		// Current flux total in nanoseconds
		int32_t m_nFluxSoFar = 0;

#ifdef ENABLE_REPLY
		// If re-play is enabled
		bool m_useReplay;

		// For storing all flux data so far for "reply with jitter"
		std::vector<ReplayData> m_fluxReplayData;
#endif
		// If the index was discovered
		bool m_indexFound = false;

		// Add data to the Rotation Extractor
		void addToExtractor(unsigned int numZeros, unsigned int pllTimeInNS, unsigned int realTimeInNS);

	public:
		// Make me - if disabled this behaves very basic which might be useful for extraction of flux to SCP
		BridgePLL(bool enabled, bool enableReplay);

		// Submit flux to the PLL
		void submitFlux(uint32_t timeInNanoSeconds, bool isAtIndex);

		// Reset the PLL
		void reset();

		// Prepare this to be used, by preparing the rotation extractor
		void prepareExtractor(bool isHD, const RotationExtractor::IndexSequenceMarker& indexSequence);

		// Change the rotation extractor
		void setRotationExtractor(RotationExtractor* extractor) { m_extractor = extractor; }

		// Re-plays the data back into the rotation extractor but with (random) +/- 64ns of jitter
		void rePlayData(const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
			std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation);

		// Return the active rotation extractor
		RotationExtractor* rotationExtractor() { return m_extractor; }

		// Pass on some functions from the extractor
		bool canExtract() { return m_extractor->canExtract(); }
		bool extractRotation(RotationExtractor::MFMSample* output, unsigned int& outputBits, const unsigned int maxBufferSizeBytes, const bool usePLLTime = false) { return m_extractor->extractRotation(output, outputBits, maxBufferSizeBytes, usePLLTime); }
		void getIndexSequence(RotationExtractor::IndexSequenceMarker& sequence) const { m_extractor->getIndexSequence(sequence); }
		unsigned int totalTimeReceived() const { return m_extractor->totalTimeReceived(); }
	};
};


#endif