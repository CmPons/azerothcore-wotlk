/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef AC_PROGRESSION_RAID_RESET_H
#define AC_PROGRESSION_RAID_RESET_H

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

// Pure policy: independent of maps, players, databases and the wall clock.
namespace ProgressionRaidReset
{
    constexpr int64_t Day = 86400;
    enum class Stage : uint8_t { Unmanaged = 0, Fresh = 1, Progression = 2, Cleared = 3 };

    struct Layout
    {
        std::string_view header;
        uint32_t slots;
        uint32_t encounterMask;
        uint32_t requiredMask;
    };

    inline std::optional<Layout> GetLayout(uint32_t mapId)
    {
        // These are InstanceScript boss indexes, NOT DungeonEncounter.dbc indexes.
        switch (mapId)
        {
            case 249: return Layout{"OL", 1, 0x1, 0x1};
            // ZG: eight regular bosses. Fishing/Edge of Madness may start the clock,
            // but are not required; Thekal's zealots and Ohgan are not bosses.
            case 309: return Layout{"ZG", 13, 0x3ff, 0xff};
            case 409: return Layout{"MC", 10, 0x3ff, 0x3ff};
            case 469: return Layout{"BWL", 8, 0xff, 0xff};
            case 509: return Layout{"RA", 6, 0x3f, 0x3f};
            // AQ40 index zero is unused. Include Trio, Viscidus and Ouro.
            case 531: return Layout{"AQT", 10, 0x3fe, 0x3fe};
            default: return std::nullopt;
        }
    }

    struct Progress
    {
        bool started = false;
        bool cleared = false;
    };

    inline std::optional<Progress> ReadProgress(uint32_t mapId, std::string const& data)
    {
        auto layout = GetLayout(mapId);
        if (!layout)
            return std::nullopt;

        std::istringstream input(data);
        for (char expected : layout->header)
        {
            char actual;
            if (!(input >> actual) || actual != expected)
                return std::nullopt;
        }

        uint32_t done = 0;
        for (uint32_t index = 0; index < layout->slots; ++index)
        {
            uint32_t state;
            // TO_BE_DECIDED (5) is valid in unused/uninitialized slots.
            if (!(input >> state) || state > 5)
                return std::nullopt;
            if (state == 3) // DONE, not kill-credit bits (Razorgore phase-one failures).
                done |= uint32_t(1) << index;
        }
        return Progress{(done & layout->encounterMask) != 0,
            (done & layout->requiredMask) == layout->requiredMask};
    }

    struct State
    {
        Stage stage = Stage::Unmanaged;
        int64_t deadline = 0;
        bool operator==(State const&) const = default;
    };

    inline int64_t NextDailyReset(int64_t now, uint32_t hour)
    {
        // Match the core's epoch-day arithmetic, not the host's local timezone/DST.
        int64_t reset = now / Day * Day + hour * 3600;
        return reset > now ? reset : reset + Day;
    }

    inline State Advance(State previous, std::optional<Progress> progress, int64_t now,
        uint32_t days, uint32_t hour)
    {
        if (previous.stage == Stage::Unmanaged)
        {
            // Existing malformed/unknown saves get a conservative progression window.
            if (!progress || progress->started)
                previous = {Stage::Progression, now + days * Day};
            else
                previous = {Stage::Fresh, NextDailyReset(now, hour)};
        }
        if (progress)
        {
            if (progress->cleared && previous.stage != Stage::Cleared)
                return {Stage::Cleared, NextDailyReset(now, hour)};
            if (progress->started && previous.stage == Stage::Fresh)
                return {Stage::Progression, now + days * Day};
        }
        // Later kills, reloads, logins and repeated saves never refresh the clock.
        return previous;
    }

    inline int64_t ExtendedDeadline(State state, uint32_t days, uint32_t hour)
    {
        return state.stage == Stage::Progression ? state.deadline + days * Day :
            NextDailyReset(state.deadline, hour);
    }

    inline uint8_t WarningStage(int64_t secondsLeft)
    {
        return secondsLeft <= 60 ? 4 : secondsLeft <= 300 ? 3 : secondsLeft <= 900 ? 2 :
            secondsLeft <= 3600 ? 1 : 0;
    }
}

#endif
