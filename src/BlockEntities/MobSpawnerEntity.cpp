
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "MobSpawnerEntity.h"
#include "json/json.h"

#include "../World.h"
#include "../FastRandom.h"
#include "../MobSpawner.h"
#include "../Items/ItemSpawnEgg.h"





cMobSpawnerEntity::cMobSpawnerEntity(int a_BlockX, int a_BlockY, int a_BlockZ, cWorld * a_World)
	: super(E_BLOCK_MOB_SPAWNER, a_BlockX, a_BlockY, a_BlockZ, a_World)
	, m_Entity(mtPig)
	, m_SpawnDelay(100)
	, m_IsActive(false)
{
}





void cMobSpawnerEntity::SendTo(cClientHandle & a_Client)
{
	a_Client.SendUpdateBlockEntity(*this);
}





void cMobSpawnerEntity::UsedBy(cPlayer * a_Player)
{
	if (a_Player->GetEquippedItem().m_ItemType == E_ITEM_SPAWN_EGG)
	{
		cMonster::eType MonsterType = cItemSpawnEggHandler::ItemDamageToMonsterType(a_Player->GetEquippedItem().m_ItemDamage);
		if (MonsterType == mtInvalidType)
		{
			return;
		}

		m_Entity = MonsterType;
		ResetTimer();
		if (!a_Player->IsGameModeCreative())
		{
			a_Player->GetInventory().RemoveOneEquippedItem();
		}
		LOGD("Changed monster spawner entity to %s!", GetEntityName().c_str());
	}
}





void cMobSpawnerEntity::UpdateActiveState(void)
{
	if (GetNearbyPlayersNum() > 0)
	{
		m_IsActive = true;
	}
	else
	{
		m_IsActive = false;
	}
}





bool cMobSpawnerEntity::Tick(float a_Dt, cChunk & a_Chunk)
{
	// Update the active flag every 5 seconds
	if ((m_World->GetWorldAge() % 100) == 0)
	{
		UpdateActiveState();
	}

	if (!m_IsActive)
	{
		return false;
	}

	if (m_SpawnDelay <= 0)
	{
		SpawnEntity();
		return true;
	}
	else
	{
		m_SpawnDelay--;
	}
	return false;
}





void cMobSpawnerEntity::ResetTimer(void)
{
	m_SpawnDelay = 200 + m_World->GetTickRandomNumber(600);
	m_World->BroadcastBlockEntity(m_PosX, m_PosY, m_PosZ);
}





void cMobSpawnerEntity::SpawnEntity(void)
{
	int NearbyEntities = GetNearbyMonsterNum(m_Entity);
	if (NearbyEntities >= 6)
	{
		ResetTimer();
		return;
	}

	class cCallback : public cChunkCallback
	{
	public:
		cCallback(int a_RelX, int a_RelY, int a_RelZ, eMonsterType a_MobType, int a_NearbyEntitiesNum) :
			m_RelX(a_RelX),
			m_RelY(a_RelY),
			m_RelZ(a_RelZ),
			m_MobType(a_MobType),
			m_NearbyEntitiesNum(a_NearbyEntitiesNum)
		{
		}

		virtual bool Item(cChunk * a_Chunk)
		{
			cFastRandom Random;

			bool EntitiesSpawned = false;
			for (size_t i = 0; i < 4; i++)
			{
				if (m_NearbyEntitiesNum >= 6)
				{
					break;
				}

				int RelX = (int) (m_RelX + (double)(Random.NextFloat() - Random.NextFloat()) * 4.0);
				int RelY = m_RelY + Random.NextInt(3) - 1;
				int RelZ = (int) (m_RelZ + (double)(Random.NextFloat() - Random.NextFloat()) * 4.0);

				cChunk * Chunk = a_Chunk->GetRelNeighborChunkAdjustCoords(RelX, RelZ);
				if ((Chunk == NULL) || !Chunk->IsValid())
				{
					continue;
				}
				EMCSBiome Biome = Chunk->GetBiomeAt(RelX, RelZ);

				if (cMobSpawner::CanSpawnHere(Chunk, RelX, RelY, RelZ, m_MobType, Biome))
				{
					double PosX = Chunk->GetPosX() * cChunkDef::Width + RelX;
					double PosZ = Chunk->GetPosZ() * cChunkDef::Width + RelZ;

					cMonster * Monster = cMonster::NewMonsterFromType(m_MobType);
					if (Monster == NULL)
					{
						continue;
					}

					Monster->SetPosition(PosX, RelY, PosZ);
					Monster->SetYaw(Random.NextFloat() * 360.0f);
					if (Chunk->GetWorld()->SpawnMobFinalize(Monster) != mtInvalidType)
					{
						EntitiesSpawned = true;
						Chunk->BroadcastSoundParticleEffect(2004, (int)(PosX * 8.0), (int)(RelY * 8.0), (int)(PosZ * 8.0), 0);
						m_NearbyEntitiesNum++;
					}
				}
			}
			return EntitiesSpawned;
		}
	protected:
		int m_RelX, m_RelY, m_RelZ;
		eMonsterType m_MobType;
		int m_NearbyEntitiesNum;
	} Callback(m_RelX, m_PosY, m_RelZ, m_Entity, NearbyEntities);

	if (m_World->DoWithChunk(GetChunkX(), GetChunkZ(), Callback))
	{
		ResetTimer();
	}
}





bool cMobSpawnerEntity::LoadFromJson(const Json::Value & a_Value)
{
	m_PosX = a_Value.get("x", 0).asInt();
	m_PosY = a_Value.get("y", 0).asInt();
	m_PosZ = a_Value.get("z", 0).asInt();

	return true;
}





void cMobSpawnerEntity::SaveToJson(Json::Value & a_Value)
{
	a_Value["x"] = m_PosX;
	a_Value["y"] = m_PosY;
	a_Value["z"] = m_PosZ;
}





AString cMobSpawnerEntity::GetEntityName() const
{
	switch (m_Entity)
	{
		case mtBat:          return "Bat";
		case mtBlaze:        return "Blaze";
		case mtCaveSpider:   return "CaveSpider";
		case mtChicken:      return "Chicken";
		case mtCow:          return "Cow";
		case mtCreeper:      return "Creeper";
		case mtEnderDragon:  return "EnderDragon";
		case mtEnderman:     return "Enderman";
		case mtGhast:        return "Ghast";
		case mtGiant:        return "Giant";
		case mtHorse:        return "EntityHorse";
		case mtIronGolem:    return "VillagerGolem";
		case mtMagmaCube:    return "LavaSlime";
		case mtMooshroom:    return "MushroomCow";
		case mtOcelot:       return "Ozelot";
		case mtPig:          return "Pig";
		case mtSheep:        return "Sheep";
		case mtSilverfish:   return "Silverfish";
		case mtSkeleton:     return "Skeleton";
		case mtSlime:        return "Slime";
		case mtSnowGolem:    return "SnowMan";
		case mtSpider:       return "Spider";
		case mtSquid:        return "Squid";
		case mtVillager:     return "Villager";
		case mtWitch:        return "Witch";
		case mtWither:       return "WitherBoss";
		case mtWolf:         return "Wolf";
		case mtZombie:       return "Zombie";
		case mtZombiePigman: return "PigZombie";
		default:
		{
			ASSERT(!"Unknown monster type!");
			return "Pig";
		}
	}
}





int cMobSpawnerEntity::GetNearbyPlayersNum(void)
{
	Vector3d SpawnerPos(m_PosX + 0.5, m_PosY + 0.5, m_PosZ + 0.5);
	int NumPlayers = 0;

	class cCallback : public cChunkDataCallback
	{
	public:
		cCallback(Vector3d a_SpawnerPos, int & a_NumPlayers) :
			m_SpawnerPos(a_SpawnerPos),
			m_NumPlayers(a_NumPlayers)
		{
		}

		virtual void Entity(cEntity * a_Entity) override
		{
			if (!a_Entity->IsPlayer())
			{
				return;
			}

			if ((m_SpawnerPos - a_Entity->GetPosition()).Length() <= 16)
			{
				m_NumPlayers++;
			}
		}

	protected:
		Vector3d m_SpawnerPos;
		int & m_NumPlayers;
	} Callback(SpawnerPos, NumPlayers);

	int ChunkX = GetChunkX();
	int ChunkZ = GetChunkZ();
	m_World->ForEachChunkInRect(ChunkX - 1, ChunkX + 1, ChunkZ - 1, ChunkZ + 1, Callback);

	return NumPlayers;
}





int cMobSpawnerEntity::GetNearbyMonsterNum(eMonsterType a_EntityType)
{
	Vector3d SpawnerPos(m_PosX + 0.5, m_PosY + 0.5, m_PosZ + 0.5);
	int NumEntities = 0;

	class cCallback : public cChunkDataCallback
	{
	public:
		cCallback(Vector3d a_SpawnerPos, eMonsterType a_EntityType, int & a_NumEntities) :
			m_SpawnerPos(a_SpawnerPos),
			m_EntityType(a_EntityType),
			m_NumEntities(a_NumEntities)
		{
		}

		virtual void Entity(cEntity * a_Entity) override
		{
			if (!a_Entity->IsMob())
			{
				return;
			}

			cMonster * Mob = (cMonster *)a_Entity;
			if (Mob->GetMobType() != m_EntityType)
			{
				return;
			}

			if (((m_SpawnerPos - a_Entity->GetPosition()).Length() <= 8) && (Diff(m_SpawnerPos.y, a_Entity->GetPosY()) <= 4.0))
			{
				m_NumEntities++;
			}
		}

	protected:
		Vector3d m_SpawnerPos;
		eMonsterType m_EntityType;
		int & m_NumEntities;
	} Callback(SpawnerPos, a_EntityType, NumEntities);

	int ChunkX = GetChunkX();
	int ChunkZ = GetChunkZ();
	m_World->ForEachChunkInRect(ChunkX - 1, ChunkX + 1, ChunkZ - 1, ChunkZ + 1, Callback);

	return NumEntities;
}




