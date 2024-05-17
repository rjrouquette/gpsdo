//
// Created by robert on 3/26/24.
//

#pragma once

#include <cstdint>

#include "../chrony/candm.h"

namespace ntp {
    class Source {
    protected:
        // maximum number of polling samples
        static constexpr int MAX_HISTORY = 16;

        // ring buffer index modulo mask
        static constexpr int RING_MASK = MAX_HISTORY - 1;

        // minimum number of smaples required for linear regression
        static constexpr int MIN_REGRESSION_SAMPLES = 5;

        static constexpr int MAX_STRATUM = 3;

        static constexpr float MAX_DELAY = 50e-3f;

        struct Sample {
            uint64_t taiLocal;
            uint64_t taiRemote;
            float delay;

            [[nodiscard]]
            int64_t getOffset() const {
                return static_cast<int64_t>(taiRemote - taiLocal);
            }
        };

    private:
        // maximum frequency skew tolerance
        static constexpr float MAX_FREQ_SKEW = 50e-6f;

        // filter samples
        Sample ringSamples[MAX_HISTORY];
        uint8_t ringPtr;

    protected:
        uint8_t sampleCount;

        uint8_t usedOffset;
        uint8_t usedDrift;
        // time span
        int span;

        int64_t filteredOffset;
        uint64_t lastUpdate;
        uint64_t refTime;
        const uint32_t id;
        uint32_t refId;
        uint32_t rootDelay;
        uint32_t rootDispersion;
        uint32_t rxCount;
        uint32_t rxValid;
        uint32_t txCount;
        float responseTime;
        const uint16_t mode;
        uint16_t state;
        uint16_t reach;
        uint16_t stratum;
        int16_t poll;
        uint16_t pollCounter;
        int16_t minPoll;
        int16_t maxPoll;
        int8_t precision;
        uint8_t leap;
        uint8_t version;
        uint8_t ntpMode;

        // last sample offset
        float lastOffset;
        float lastOffsetOrig;
        float lastDelay;
        // offset stats
        float offsetMean;
        float offsetStdDev;
        // delay stats
        float delayMean;
        float delayStdDev;
        // frequency stats
        float freqDrift;
        float freqSkew;
        // overall score
        float score;

        // status flags
        bool xleave;
        bool prune;
        bool lost;
        bool unstable;

        Sample& advanceFilter();

        void updateFilter();

        void updateDelay();

    public:
        Source(uint32_t id_, uint16_t mode_);

        ~Source();

        /**
         * Determine if the source instance is properly allocated
         * @return true if the source instance is allocated
         */
        [[nodiscard]]
        bool isAllocated() const {
            return id != 0;
        }

        /**
         * Determine if the current source instance is a local reference
         * @return true if this instance is tracking a local reference
         */
        [[nodiscard]]
        bool isReference() const {
            return mode == RPY_SD_MD_REF;
        }

        /**
         * Determine if this source instance is a remote time server
         * @return true if this instance is tracking a remote time server
         */
        [[nodiscard]]
        bool isRemote() const {
            return mode == RPY_SD_MD_CLIENT;
        }

        /**
         * Determine if this source instance is the designated recipient for a specific IP address
         * @param srcAddr the remote address to macth
         * @return true if the remote address matches this instance
         */
        [[nodiscard]]
        bool isRecipient(const uint32_t srcAddr) const {
            return isRemote() && id == srcAddr;
        }

        /**
         * Determine if this source instance should be pruned.
         * @return true if this instance should be pruned
         */
        [[nodiscard]]
        bool isPruneable() const {
            return !isReference() && prune;
        }

        /**
         * Determine if this source instance is selectable for synchronization.
         * @return true if this instance is selectable for synchronization
         */
        [[nodiscard]]
        bool isSelectable();

        /**
         * Mark this source instance as selected.
         */
        void select() {
            state = RPY_SD_ST_SELECTED;
        }

        /**
         * Apply offset adjustment to all samples from source.
         * @param offset the offset to apply in (32.32 fixed point)
         */
        void applyOffset(int64_t offset);

        [[nodiscard]]
        auto getMode() const {
            return mode;
        }

        [[nodiscard]]
        auto getId() const {
            return id;
        }

        [[nodiscard]]
        auto getStratum() const {
            return stratum;
        }

        [[nodiscard]]
        auto getLeapIndicator() const {
            return leap;
        }

        [[nodiscard]]
        auto getScore() const {
            return score;
        }

        [[nodiscard]]
        auto getPollingInterval() const {
            return poll;
        }

        [[nodiscard]]
        auto getFrequencyDrift() const {
            return freqDrift;
        }

        [[nodiscard]]
        auto getFilteredOffset() const {
            return filteredOffset;
        }

        [[nodiscard]]
        auto getLastUpdate() const {
            return lastUpdate;
        }

        [[nodiscard]]
        auto getDelayMean() const {
            return delayMean;
        }

        [[nodiscard]]
        auto getDelayStdDev() const {
            return delayStdDev;
        }

        [[nodiscard]]
        auto getRootDelay() const {
            return rootDelay;
        }

        [[nodiscard]]
        auto getRootDispersion() const {
            return rootDispersion;
        }

        void getNtpData(RPY_NTPData &rpyNtpData) const;

        void getSourceData(RPY_Source_Data &rpySourceData) const;

        void getSourceStats(RPY_Sourcestats &rpySourceStats) const;
    };
}
