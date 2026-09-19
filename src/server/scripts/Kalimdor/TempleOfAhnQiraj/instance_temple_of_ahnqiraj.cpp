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

#include "CreatureGroups.h"
#include "GameTime.h"
#include "InstanceMapScript.h"
#include "InstanceScript.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "TaskScheduler.h"
#include "temple_of_ahnqiraj.h"
#include <set>
#include <vector>

namespace
{
    constexpr uint32 NPC_OURO_DIRT_MOUND = 15712;
}

ObjectData const creatureData[] =
{
    { NPC_VEM,          DATA_VEM          },
    { NPC_KRI,          DATA_KRI          },
    { NPC_YAUJ,         DATA_YAUJ         },
    { NPC_SARTURA,      DATA_SARTURA      },
    { NPC_CTHUN,        DATA_CTHUN        },
    { NPC_EYE_OF_CTHUN, DATA_EYE_OF_CTHUN },
    { NPC_OURO,         DATA_OURO         },
    { NPC_OURO_SPAWNER, DATA_OURO_SPAWNER },
    { NPC_MASTERS_EYE,  DATA_MASTERS_EYE  },
    { NPC_VEKLOR,       DATA_VEKLOR       },
    { NPC_VEKNILASH,    DATA_VEKNILASH    },
    { NPC_VISCIDUS,     DATA_VISCIDUS     },
    { 0,                0                 }
};

DoorData const doorData[] =
{
    { AQ40_DOOR_SKERAM,      DATA_SKERAM,        DOOR_TYPE_PASSAGE },
    { AQ40_DOOR_TE_ENTRANCE, DATA_TWIN_EMPERORS, DOOR_TYPE_ROOM },
    { AQ40_DOOR_TE_EXIT,     DATA_TWIN_EMPERORS, DOOR_TYPE_PASSAGE },
    { 0,                     0,                  DOOR_TYPE_ROOM}
};

class instance_temple_of_ahnqiraj : public InstanceMapScript
{
public:
    instance_temple_of_ahnqiraj() : InstanceMapScript("instance_temple_of_ahnqiraj", MAP_AHN_QIRAJ_TEMPLE) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_temple_of_ahnqiraj_InstanceMapScript(map);
    }

    struct instance_temple_of_ahnqiraj_InstanceMapScript : public InstanceScript
    {
        instance_temple_of_ahnqiraj_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_BOSS_NUMBER);
            LoadObjectData(creatureData, nullptr);
            LoadDoorData(doorData);
        }

        void Initialize() override
        {
            BugTrioDeathCount = 0;
            BugTrioConsumeTarget = 0;
        }

        void OnPlayerEnter(Player* player) override
        {
            InstanceScript::OnPlayerEnter(player);
            // Also recover a failed pull saved before this fix: its original spawn timer survives
            // a map unload/restart, while the old creature GUID does not.
            ScheduleOuroRecovery();
        }

        void OnCreatureRemove(Creature* creature) override
        {
            _ouroMounds.erase(creature->GetGUID());
            InstanceScript::OnCreatureRemove(creature);
            if (creature->GetEntry() == NPC_OURO || creature->GetEntry() == NPC_OURO_DIRT_MOUND)
                ScheduleOuroRecovery();
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_OURO_DIRT_MOUND:
                    _ouroMounds.insert(creature->GetGUID());
                    break;
                case NPC_MASTERS_EYE:
                    if (GetBossState(DATA_TWIN_EMPERORS) != DONE && !creature->IsAlive())
                        creature->Respawn(true);
                    break;
                case NPC_CTHUN:
                    if (!creature->IsAlive())
                    {
                        for (ObjectGuid const& guid : CThunGraspGUIDs)
                        {
                            if (GameObject* cthunGrasp = instance->GetGameObject(guid))
                            {
                                cthunGrasp->DespawnOrUnsummon(1s);
                            }
                        }
                    }
                    break;
                default:
                    break;
            }

            InstanceScript::OnCreatureCreate(creature);
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_CTHUN_GRASP:
                    CThunGraspGUIDs.push_back(go->GetGUID());
                    if (Creature* CThun = GetCreature(DATA_CTHUN))
                    {
                        if (!CThun->IsAlive())
                        {
                            go->DespawnOrUnsummon(1s);
                        }
                    }
                    break;
                default:
                    break;
            }

            InstanceScript::OnGameObjectCreate(go);
        }

        void OnUnitDeath(Unit* unit) override
        {
            switch (unit->GetEntry())
            {
                case NPC_QIRAJI_SLAYER:
                case NPC_QIRAJI_MINDSLAYER:
                    if (Creature* creature = unit->ToCreature())
                    {
                        if (CreatureGroup* formation = creature->GetFormation())
                        {
                            scheduler.Schedule(100ms, [formation](TaskContext /*context*/)
                            {
                                if (!formation->IsAnyMemberAlive(true))
                                {
                                    if (Creature* leader = formation->GetLeader())
                                    {
                                        if (leader->IsAlive())
                                        {
                                            leader->AI()->SetData(0, 1);
                                        }
                                    }
                                }
                            });
                        }
                    }
                    break;
                case NPC_CTHUN:
                    for (ObjectGuid const& guid : CThunGraspGUIDs)
                    {
                        if (GameObject* cthunGrasp = instance->GetGameObject(guid))
                        {
                            cthunGrasp->DespawnOrUnsummon(1s);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_BUG_TRIO_DEATH:
                    return BugTrioDeathCount;
                case DATA_BUG_TRIO_CONSUME_TARGET:
                    return BugTrioConsumeTarget;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_BUG_TRIO_DEATH:
                    if (data != 0)
                        ++BugTrioDeathCount;
                    else
                        BugTrioDeathCount = 0;
                    break;
                case DATA_BUG_TRIO_CONSUME_TARGET:
                    BugTrioConsumeTarget = data;
                    break;
                default:
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            // Late mound evades must not reopen an already completed Ouro encounter.
            if (type == DATA_OURO && GetBossState(DATA_OURO) == DONE && state != DONE)
                return false;
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case DATA_OURO:
                    if (state == FAIL)
                        ScheduleOuroRecovery();
                    break;
                default:
                    break;
            }

            return true;
        }

    private:
        bool OuroCanRecover() const
        {
            EncounterState const state = GetBossState(DATA_OURO);
            return state == NOT_STARTED || state == FAIL;
        }

        void ScheduleOuroRecovery()
        {
            if (_ouroRecoveryPending || !OuroCanRecover())
                return;
            _ouroRecoveryPending = true;
            scheduler.Schedule(2s, [this](TaskContext context)
            {
                if (!OuroCanRecover())
                {
                    _ouroRecoveryPending = false;
                    return;
                }
                // A submerge replaces the boss with moving mounds. A single mound's evade is
                // not permission to create another boss while its siblings can still re-emerge.
                if (GetCreature(DATA_OURO) || !_ouroMounds.empty())
                {
                    context.Repeat(2s);
                    return;
                }
                _ouroRecoveryPending = false;
                RecoverOuroSpawner();
            });
        }

        void RecoverOuroSpawner()
        {
            if (!OuroCanRecover() || GetCreature(DATA_OURO) || !_ouroMounds.empty())
                return;
            // Compatibility-mode spawns may still be present as dead objects. Do not force a
            // living spawner through a death/respawn cycle, or bypass native respawn conditions.
            if (Creature* spawner = GetCreature(DATA_OURO_SPAWNER))
            {
                if (!spawner->IsAlive())
                    spawner->Respawn();
                return;
            }

            // Non-compat despawn removes the instance GUID entirely. Recover the ORIGINAL DB
            // spawn via this map's native respawn queue, never by summoning a replacement boss.
            std::vector<ObjectGuid::LowType> spawns;
            for (auto const& [spawnId, respawnTime] : instance->GetCreatureRespawnTimes())
            {
                CreatureData const* data = sObjectMgr->GetCreatureData(spawnId);
                if (data && data->mapid == MAP_AHN_QIRAJ_TEMPLE && data->id == NPC_OURO_SPAWNER &&
                    !data->id2 && !data->id3)
                    spawns.push_back(spawnId);
            }
            if (spawns.size() != 1)
                return; // absent or ambiguous metadata: do not fabricate a new spawn

            time_t now = GameTime::GetGameTime().count();
            instance->SaveCreatureRespawnTime(spawns.front(), now);
            LOG_INFO("scripts", "Ouro recovery: queued original spawner {} in instance {}",
                     spawns.front(), instance->GetInstanceId());
        }

        bool _ouroRecoveryPending = false;
        std::set<ObjectGuid> _ouroMounds;
        GuidVector CThunGraspGUIDs;
        uint32 BugTrioDeathCount;
        uint32 BugTrioConsumeTarget;
    };
};

void AddSC_instance_temple_of_ahnqiraj()
{
    new instance_temple_of_ahnqiraj();
}
