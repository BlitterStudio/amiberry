/* ArduinoFloppyReader (and writer) - Rotation Extractor
*
* Copyright (C) 2017-2021 Robert Smith (@RobSmithDev)
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

////////////////////////////////////////////////////////////////////////////////////////
// Class to manage finding *exact* disk revolutions                                   //
////////////////////////////////////////////////////////////////////////////////////////
//
// Purpose:
// The class attempts to guess where an exact disk revolution occurs, and then re-aligns
// it such that it starts at the index pulse.  This means we dont need to wait for an index
// pulse to work out a revolution of the disk.  The first time a disk is used we calculate
// the time of a single revolution and then use that as a guide to how long a revolution will
// take in the future.
//
// This isn't 100% perfect but does seem to work.

#include "RotationExtractor.h"

// Finds the overlap between the start of the data and where we currently are.  The returned position is where the NEXT revolution starts
unsigned int RotationExtractor::getOverlapPosition() const {
	int bestScore = OVERLAP_SEQUENCE_MATCHES / 4;     // must have *some* kind of match to be worthy
	unsigned int bestScoreIndex = m_revolutionReadyAt;

	// Working back from the mid-point
	for (unsigned int midPoint = 0; midPoint < OVERLAP_SEQUENCE_MATCHES * (OVERLAP_EXTRA_BUFFER - 1); midPoint++) {

		// Count the number of matching sequences
		int scoreL = 0;
		int scoreR = 0;
		const int startPositionR = m_revolutionReadyAt + midPoint;
		const int startPositionL = m_revolutionReadyAt - midPoint;

		if (startPositionL >= 0) {
			for (unsigned int pos = 0; pos < OVERLAP_SEQUENCE_MATCHES; pos++) {
				if (m_sequences[pos].mfm == m_sequences[startPositionR + pos].mfm) scoreR++;
				if (m_sequences[pos].mfm == m_sequences[startPositionL + pos].mfm) scoreL++;
			}
		}
		else {
			for (unsigned int pos = 0; pos < OVERLAP_SEQUENCE_MATCHES; pos++) {
				if (m_sequences[pos].mfm == m_sequences[startPositionR + pos].mfm) scoreR++;
			}
		}

		// A perfect score short-circuits the rest of the loop		
		if (scoreL > bestScore) {
			bestScore = scoreL;
			bestScoreIndex = startPositionL;
		}
		if (scoreR > bestScore) {
			bestScore = scoreR;
			bestScoreIndex = startPositionR;
		}

		if (bestScore == OVERLAP_SEQUENCE_MATCHES) return bestScoreIndex;
	}

	// If we got here then there wasnt a perfect match.  This would only happen if:
	// 1. The drive speed is broken!
	// 2. The overlap is unformatted, in which case it doesn't really matter anyway
	// 3. The disk/head is damaged or dirty, so then there's no hope anyway
	return bestScoreIndex;
}

// Finds the true start position of the INDEX pulse based on previous revolutions.  This is a lot like the above function
const unsigned int RotationExtractor::getTrueIndexPosition(const unsigned int nextRevolutionStart, const unsigned int startingPoint) {
	// Where to start from
	const int firstPoint = (startingPoint == INDEX_NOT_FOUND) ? m_sequenceIndex : startingPoint;

	// First. Do we actually have a 'index marker' buffer?
	if (!m_indexSequence.valid) {
		// Not valid means we make it, and take our index as "true"
		m_indexSequence.valid = true;
		for (unsigned int pos = 0; pos < OVERLAP_SEQUENCE_MATCHES; pos++)
			m_indexSequence.sequences[pos] = m_sequences[(firstPoint + pos) % nextRevolutionStart].mfm;

		return firstPoint;
	}

	int bestScore = OVERLAP_SEQUENCE_MATCHES / 4;     // must have *some* kind of match to be worthy
	unsigned int bestScoreIndex = firstPoint;

	// Working back from the mid-point
	for (unsigned int midPoint = 0; midPoint < OVERLAP_SEQUENCE_MATCHES * (OVERLAP_EXTRA_BUFFER - 1); midPoint++) {

		// Count the number of matching sequences
		int scoreL = 0;
		int scoreR = 0;
		const int startPositionR = firstPoint + midPoint;
		const int startPositionL = firstPoint - midPoint;

		if (startPositionL >= 0) {
			for (unsigned int pos = 0; pos < OVERLAP_SEQUENCE_MATCHES; pos++) {
				if (m_indexSequence.sequences[pos] == m_sequences[(startPositionR + pos) % nextRevolutionStart].mfm) scoreR++;
				if (m_indexSequence.sequences[pos] == m_sequences[(startPositionL + pos) % nextRevolutionStart].mfm) scoreL++;
			}
		}
		else {
			for (unsigned int pos = 0; pos < OVERLAP_SEQUENCE_MATCHES; pos++) {
				if (m_indexSequence.sequences[pos] == m_sequences[(startPositionR + pos) % nextRevolutionStart].mfm) scoreR++;
			}
		}

		// A perfect score short-circuits the rest of the loop		
		if (scoreL > bestScore) {
			bestScore = scoreL;
			bestScoreIndex = startPositionL;
		}
		if (scoreR > bestScore) {
			bestScore = scoreR;
			bestScoreIndex = startPositionR;
		}

		if (bestScore == OVERLAP_SEQUENCE_MATCHES) return bestScoreIndex;
	}

	// If we got here then there wasnt a perfect match.  This would only happen if:
	// 1. The drive speed is broken!
	// 2. The overlap is unformatted, in which case it doesn't really matter anyway
	// 3. The disk/head is damaged or dirty, so then there's no hope anyway
	return bestScoreIndex;
}

// Write a bit into the stream
inline void writeStreamBit(RotationExtractor::MFMSample* output, unsigned int& pos, unsigned int& bit, bool value, const unsigned short valuespeed, const unsigned int maxLength) {
	if (pos >= maxLength) return;

	output[pos].mfmData <<= 1;
	if (value) output[pos].mfmData |= 1;

#ifdef OUTPUT_TIME_IN_NS
	output[pos].bittime[7 - bit] = valuespeed;
#else
#ifdef HIGH_RESOLUTION_MODE
	output[pos].speed[7 - bit] = valuespeed;
#else
	if (bit == 0) output[pos].speed = valuespeed; else  output[pos].speed += valuespeed;
#endif
#endif

	bit++;
	if (bit >= 8) {
#ifndef OUTPUT_TIME_IN_NS
#ifndef HIGH_RESOLUTION_MODE
		output[pos].speed /= 8;
#endif
#endif
		pos++;
		bit = 0;
	}
}

// Submit a single sequence to the list
void RotationExtractor::submitSequence(const MFMSequenceInfo& sequence, const bool isIndex) {
	// we reject the first 20uSec of data.  Makes things so much more stable
	m_timeReceived += sequence.timeNS;
	if (m_timeReceived < 20000) return;

	// And stop if we have too much.  Shouldn't happen
	if (m_sequencePos >= MAX_REVOLUTION_SEQUENCES) return;

	// See if we can calculate the time it takes to get a single revolution from the disk
	if ((m_revolutionTime == 0) && (!m_useIndex)) {
		if (isIndex) {
			if (m_sequenceIndex == INDEX_NOT_FOUND) m_sequenceIndex = 0; else m_nextSequenceIndex = m_sequencePos;
		}

		if (m_sequenceIndex != INDEX_NOT_FOUND) {
			// Store the sequence only if we found the first index
			m_sequences[m_sequencePos++] = sequence;
			m_currentTime += sequence.timeNS;

			if (m_nextSequenceIndex == INDEX_NOT_FOUND)
				m_revolutionTimeCounting += sequence.timeNS;
			else {
				m_revolutionTime = m_revolutionTimeCounting;

				// Set the nearly complete marker to be at 90%.  That would only need approx another 20ms to complete
				m_revolutionTimeNearlyComplete = (unsigned int)(m_revolutionTime * 0.9f);
			}
		}
		return;
	}

	// Handle index search
	if (isIndex) {
		if (m_sequenceIndex == INDEX_NOT_FOUND) m_sequenceIndex = m_sequencePos; else m_nextSequenceIndex = m_sequencePos;
	}

	// Do different things depending on the mode in use
	if (m_useIndex) {
		if (m_sequenceIndex == INDEX_NOT_FOUND) {
			// Store the data in the circular buffer
			m_initialSequences[m_initialSequencesWritePos] = sequence;
			m_initialSequencesWritePos = (m_initialSequencesWritePos + 1) % (OVERLAP_SEQUENCE_MATCHES * OVERLAP_EXTRA_BUFFER);
			if (m_initialSequencesLength < OVERLAP_SEQUENCE_MATCHES * OVERLAP_EXTRA_BUFFER) m_initialSequencesLength++;
			m_revolutionReady = false;
		}
		else {
			// Was the FIRST index detected?
			if ((isIndex) && (m_nextSequenceIndex == INDEX_NOT_FOUND) && (m_initialSequencesLength)) {
				// Ok, shunt the buffer we have onto the output
				m_sequencePos = m_initialSequencesLength;
				m_sequenceIndex = m_initialSequencesLength;

				// Go through all of them
				while (m_initialSequencesLength) {
					// Wind back one
					m_initialSequencesLength--;
					// Handle wrap-around buffer
					if (m_initialSequencesWritePos == 0) m_initialSequencesWritePos = (OVERLAP_SEQUENCE_MATCHES * OVERLAP_EXTRA_BUFFER) - 1; else m_initialSequencesWritePos--;
					// Save it
					m_sequences[m_initialSequencesLength] = m_initialSequences[m_initialSequencesWritePos];
				}
				m_initialSequencesLength = 0;
			}

			// Store as normal
			m_sequences[m_sequencePos++] = sequence;

			// Check if ready
			if (m_nextSequenceIndex != INDEX_NOT_FOUND) {
				if (m_sequencePos > m_nextSequenceIndex + (OVERLAP_SEQUENCE_MATCHES * OVERLAP_EXTRA_BUFFER)) {
					m_revolutionReadyAt = m_nextSequenceIndex;
					m_revolutionReady = true;
				}
			}
		}
	}
	else {
		m_sequences[m_sequencePos++] = sequence;
		m_currentTime += (unsigned int)sequence.timeNS;

		// This is a sneaky check-ahead for a full rotation, like a virtual index marker
		if (m_currentTime >= m_revolutionTime) {
			if (m_revolutionReadyAt == INDEX_NOT_FOUND) m_revolutionReadyAt = m_sequencePos; else
				if (m_sequencePos > m_revolutionReadyAt + (OVERLAP_SEQUENCE_MATCHES * OVERLAP_EXTRA_BUFFER))
					m_revolutionReady = true;
		}
	}
}

// Extracts a single rotation and updates the buffer to remove it.  Returns FALSE if no rotation is available
bool RotationExtractor::extractRotation(MFMSample* output, unsigned int& outputBits, const unsigned int maxBufferSizeBytes) {
	// Step 0: check if we're possibly ready
	if (!canExtract()) return false;

	if (m_useIndex) {
		if (m_sequenceIndex == INDEX_NOT_FOUND) return false;
		if (m_nextSequenceIndex == INDEX_NOT_FOUND) return false;
		if (m_sequenceIndex > m_nextSequenceIndex) {
			unsigned int s = m_sequenceIndex;
			m_sequenceIndex = m_nextSequenceIndex;
			m_nextSequenceIndex = s;
		}

		// Step 1: Find where the first index position is
		const unsigned int revolutionStart = getTrueIndexPosition(INDEX_NOT_FOUND, m_sequenceIndex);

		// Step 2: find out where the next one is
		const unsigned int nextRevolutionStart = getTrueIndexPosition(INDEX_NOT_FOUND, m_nextSequenceIndex);

		// Markers
		unsigned int outputStreamPos = 0;
		unsigned int outputStreamBit = 0;
		unsigned int totalSamples = nextRevolutionStart - revolutionStart;
		// Work out revolution time anyway
		unsigned int rTime = 0;

		// Step 3: output.  Data goes from 0 to nextRevolutionStart-1, but we need to output from indexPosition
		for (unsigned int pos = 0; pos < totalSamples; pos++) {
			const MFMSequenceInfo& sequence = m_sequences[pos + revolutionStart];
			rTime += sequence.timeNS;
			
#ifdef OUTPUT_TIME_IN_NS
			const unsigned int bitTime = sequence.timeNS / ((unsigned int)sequence.mfm + 2);

			// And write the output stream
			for (unsigned int s = 0; s <= (unsigned int)sequence.mfm; s++)
				writeStreamBit(output, outputStreamPos, outputStreamBit, false, bitTime, maxBufferSizeBytes);
			writeStreamBit(output, outputStreamPos, outputStreamBit, (sequence.mfm != MFMSequence::mfm0000), bitTime, maxBufferSizeBytes);
#else
			const unsigned int speed = ((unsigned int)sequence.timeNS * 100) / (((unsigned int)sequence.mfm + 2) * 2000);

			// And write the output stream
			for (unsigned int s = 0; s <= (unsigned int)sequence.mfm; s++)
				writeStreamBit(output, outputStreamPos, outputStreamBit, false, speed, maxBufferSizeBytes);
			writeStreamBit(output, outputStreamPos, outputStreamBit, (sequence.mfm != MFMSequence::mfm0000), speed, maxBufferSizeBytes);
#endif
		}
		// Need to shift the last ones onto place
		if (outputStreamBit) {
			output[outputStreamPos].mfmData <<= (8 - outputStreamBit);
#ifndef OUTPUT_TIME_IN_NS
#ifndef HIGH_RESOLUTION_MODE
			output[outputStreamPos].speed /= outputStreamBit;
#endif
#endif
		}
		if (m_revolutionTime == 0) {
			m_revolutionTime = rTime;
			m_revolutionTimeNearlyComplete = (unsigned int)(m_revolutionTime * 0.9f);
		}

		// Calculate how much we wrote
		outputBits = (outputStreamPos * 8) + outputStreamBit;

		// Now shift the remaining data so that the next revolution starts at 0
		for (unsigned int pos = 0; pos < m_sequencePos - nextRevolutionStart; pos++)
			m_sequences[pos] = m_sequences[pos + nextRevolutionStart];

		// And mark it
		if (m_nextSequenceIndex > nextRevolutionStart) m_sequenceIndex = m_nextSequenceIndex - nextRevolutionStart; else m_sequenceIndex = 0;
		m_nextSequenceIndex = INDEX_NOT_FOUND;

		// And account for the shift
		m_sequencePos -= nextRevolutionStart;		
	}
	else {

		// Step 1: Find the overlap between the start of the data and the end of the data
		const unsigned int nextRevolutionStart = getOverlapPosition();
		if (nextRevolutionStart < 1) return false;

		// Step 2: wind it back to the INDEX position
		const unsigned int indexPosition = getTrueIndexPosition(nextRevolutionStart);

		// Markers
		unsigned int outputStreamPos = 0;
		unsigned int outputStreamBit = 0;

		// Step 3: output.  Data goes from 0 to nextRevolutionStart-1, but we need to output from indexPosition
		for (unsigned int pos = 0; pos < nextRevolutionStart; pos++) {
			const MFMSequenceInfo& sequence = m_sequences[(pos + indexPosition) % nextRevolutionStart];
			m_currentTime -= (unsigned int)sequence.timeNS;

#ifdef OUTPUT_TIME_IN_NS
			const unsigned int bitTime = sequence.timeNS / ((unsigned int)sequence.mfm + 2);

			// And write the output stream
			for (unsigned int s = 0; s <= (unsigned int)sequence.mfm; s++)
				writeStreamBit(output, outputStreamPos, outputStreamBit, false, bitTime, maxBufferSizeBytes);
			writeStreamBit(output, outputStreamPos, outputStreamBit, (sequence.mfm != MFMSequence::mfm0000), bitTime, maxBufferSizeBytes);
#else
			const unsigned int speed = ((unsigned int)sequence.timeNS * 100) / (((unsigned int)sequence.mfm + 2) * 2000);

			// And write the output stream
			for (unsigned int s = 0; s <= (unsigned int)sequence.mfm; s++)
				writeStreamBit(output, outputStreamPos, outputStreamBit, false, speed, maxBufferSizeBytes);
			writeStreamBit(output, outputStreamPos, outputStreamBit, (sequence.mfm != MFMSequence::mfm0000), speed, maxBufferSizeBytes);
#endif
		}
		// Need to shift the last ones onto place
		if (outputStreamBit) {
			output[outputStreamPos].mfmData <<= (8 - outputStreamBit);
#ifndef OUTPUT_TIME_IN_NS
#ifndef HIGH_RESOLUTION_MODE
			output[outputStreamPos].speed /= outputStreamBit;
#endif
#endif
		}

		// Calculate how much we wrote
		outputBits = (outputStreamPos * 8) + outputStreamBit;

		// Now it's extracted we need to remove it.  First. Shift or reset the index marker
		if ((m_nextSequenceIndex != INDEX_NOT_FOUND) && (m_nextSequenceIndex >= nextRevolutionStart)) m_sequenceIndex = m_nextSequenceIndex - nextRevolutionStart; else m_sequenceIndex = INDEX_NOT_FOUND;
		m_nextSequenceIndex = INDEX_NOT_FOUND;

		// Now shift the remaining data
		for (unsigned int pos = 0; pos < m_sequencePos - nextRevolutionStart; pos++)
			m_sequences[pos] = m_sequences[pos + nextRevolutionStart];

		// And account for the shift
		m_sequencePos -= nextRevolutionStart;
	}

	m_revolutionReadyAt = INDEX_NOT_FOUND;
	m_revolutionReady = false;
	m_initialSequencesLength = 0;
	m_initialSequencesWritePos = 0;

	// and success!
	return true;
}