/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DYNAMICVISIBILITY_H
#define __DYNAMICVISIBILITY_H

#include "Define.h"

struct VisibilitySettingData
{
    uint32 visibilityNotifyDelay;
    uint32 aiNotifyDelay;
    float requiredMoveDistanceSq;
};

// pussywizard: dynamic visibility settings
// 11 player intervals: 0-499, 500-999, 1000-1499, 1500-1999, 2000-2499, 2500-2999, 3000-3499,
//                      3500-3999, 4000-4499, 4500-4999, 5000+
// 5 map types: common, instance, raid, bg, arena
// feel free to add more intervals, change existing ones or move to conf file :P
// The 3500+ rows keep bg scaling mild (queued bots still need responsive fights) and leave arena
// untouched — only continents/instances/raids absorb the extra notify throttling.
#define VISIBILITY_SETTINGS_PLAYER_INTERVAL 500
#define VISIBILITY_SETTINGS_MAX_INTERVAL_NUM 11
const VisibilitySettingData VisibilitySettings[VISIBILITY_SETTINGS_MAX_INTERVAL_NUM][5] =
{
    { {300, 150, 1.0f}, {300, 150, 1.0f}, {300, 150, 1.0f}, {300, 150, 1.0f}, {300, 150, 1.0f} }, // 0-499
    { {400, 200, 2.25f}, {400, 200, 2.25f}, {400, 200, 2.25f}, {300, 150, 1.0f}, {300, 150, 1.0f} }, // 500-999
    { {500, 250, 4.0f}, {500, 250, 4.0f}, {500, 250, 4.0f}, {400, 200, 2.25f}, {300, 150, 1.0f} }, // 1000-1499
    { {700, 350, 6.25f}, {700, 350, 6.25f}, {700, 350, 6.25f}, {600, 300, 6.25f}, {300, 200, 1.0f} }, // 1500-1999
    { {1000, 500, 16.0f}, {1000, 500, 16.0f}, {1000, 500, 16.0f}, {1000, 500, 16.0f}, {300, 250, 1.0f} }, // 2000-2499
    { {1000, 500, 16.0f}, {1000, 500, 16.0f}, {1000, 500, 16.0f}, {1000, 500, 16.0f}, {300, 350, 1.0f} }, // 2500-2999
    { {1200, 550, 20.0f}, {1200, 550, 25.0f}, {1200, 550, 25.0f}, {1100, 550, 16.0f}, {300, 350, 1.0f} }, // 3000-3499
    { {1400, 600, 25.0f}, {1400, 600, 25.0f}, {1400, 600, 25.0f}, {1100, 550, 16.0f}, {300, 350, 1.0f} }, // 3500-3999
    { {1600, 650, 30.25f}, {1600, 650, 30.25f}, {1600, 650, 30.25f}, {1200, 600, 20.25f}, {300, 350, 1.0f} }, // 4000-4499
    { {1800, 700, 36.0f}, {1800, 700, 36.0f}, {1800, 700, 36.0f}, {1200, 600, 20.25f}, {300, 350, 1.0f} }, // 4500-4999
    { {2000, 750, 42.25f}, {2000, 750, 42.25f}, {2000, 750, 42.25f}, {1300, 650, 25.0f}, {300, 350, 1.0f} } // 5000+
};

class DynamicVisibilityMgr
{
public:
    static void Update(uint32 sessionCount);
    static uint32 GetVisibilityNotifyDelay(uint32 map_type) { return VisibilitySettings[visibilitySettingsIndex][map_type].visibilityNotifyDelay; }
    static uint32 GetAINotifyDelay(uint32 map_type) { return VisibilitySettings[visibilitySettingsIndex][map_type].aiNotifyDelay; }
    static float GetReqMoveDistSq(uint32 map_type) { return VisibilitySettings[visibilitySettingsIndex][map_type].requiredMoveDistanceSq; }
protected:
    static uint8 visibilitySettingsIndex;
};

#endif
