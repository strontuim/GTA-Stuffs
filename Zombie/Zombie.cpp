
#include "..\Common\script.h"
#include "Zombie.h"
#include "..\Common\ZombieModels.h"
#include "..\Common\Animations.h"
#include "..\Common\Zones.h"
#include "..\Common\ZombieModels.h"
#include "..\Common\Groups.h"
#include <fstream>

namespace Zombie
{
	struct managed
	{
		Ped		zombie;
		DWORD	lastUpdate;
	};
	int		get_free_seat(Vehicle veh)
	{
		int i = -1;
		while (i < VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(veh))
		{
			if (VEHICLE::IS_VEHICLE_SEAT_FREE(veh, i))
			{
				return (i);
			}
			i++;
		}
	}

	Ped		is_headtracking_which_player_ped(Ped ped) // return the player targeted, we only care about many ped attacking another ped if that ped is player, normal ped can just get their 1 or 2 zombies
	{
		if (PLAYER::IS_PLAYER_ONLINE())
		{
			for (Player i = 0; i < NETWORK::NETWORK_GET_NUM_CONNECTED_PLAYERS(); i++)
			{
				if (PED::IS_PED_HEADTRACKING_PED(ped, PLAYER::GET_PLAYER_PED(i)))
					return (PLAYER::GET_PLAYER_PED(i));
			}
			return (-1);
		}
		else
		{
			if (PED::IS_PED_HEADTRACKING_PED(ped, PLAYER::PLAYER_PED_ID()))
				return (PLAYER::PLAYER_PED_ID());
			else
				return (-1);
		}
	}

	Ped	get_closest_pped(Ped ped)
	{
		if (NETWORK::NETWORK_IS_SESSION_STARTED()) // unimplemented
		{
			return (-1);
		}
		else
		{
			return (PLAYER::PLAYER_PED_ID());
		}
	}

	static std::vector<managed>	zombies;
	void		zombify_nearby_ped(Ped id) // shitfunction
	{
		int		skin_hash = Utilities::get_hash("u_m_y_zombie_01");
		int		peds[1000] = { 0 }; // the array for getnearbypeds is a int + a int of padding, then a element = 2 int, and the first element is the size of the array
		peds[0] = 499;
		int count = PED::GET_PED_NEARBY_PEDS(id, peds, -1);
		for (int i = 0; i < count; i++)
		{
			int realpos = i * 2 + 2;
			if (ENTITY::IS_AN_ENTITY(peds[realpos]))
				if (!PED::IS_PED_A_PLAYER(peds[realpos]) && ENTITY::GET_ENTITY_MODEL(peds[realpos]) != skin_hash)
				{
					if (PED::IS_PED_IN_ANY_VEHICLE(peds[realpos], true))
						PED::KNOCK_PED_OFF_VEHICLE(peds[realpos]);
					STREAMING::REQUEST_MODEL(skin_hash);
					while (STREAMING::HAS_MODEL_LOADED(skin_hash) == false)
						WAIT(0);
					Vector3 pos = ENTITY::GET_ENTITY_COORDS(peds[realpos], true);
					float heading = ENTITY::GET_ENTITY_HEADING(peds[realpos]);
					PED::CREATE_PED(26, skin_hash, pos.x, pos.y, pos.z, heading, true, true);
					PED::REMOVE_PED_ELEGANTLY(&peds[realpos]);
				}
		}
	}

	void		clean_a_zombie(Vector3 point)
	{
		int		zombie_deleted = 0;
		bool	foundDeletable = true;

		while (zombie_deleted < MAX_ZOMBIE_DELETION_PER_FRAME && foundDeletable)
		{
			foundDeletable = false;
			auto it = zombies.begin();
			while (it != zombies.end() && !foundDeletable)
			{
				if (ENTITY::IS_AN_ENTITY((*it).zombie))
				{
					Vector3 pos = ENTITY::GET_ENTITY_COORDS((*it).zombie, true);
					if ((GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pos.x, pos.y, pos.z, point.x, point.y, point.z, true) > ZOMBIE_CLEANING_DISTANCE || PED::_IS_PED_DEAD((*it).zombie, true)) && !CAM::IS_SPHERE_VISIBLE(pos.x, pos.y, pos.z, 1.0))
					{
						PED::DELETE_PED(&((*it).zombie));
						zombies.erase(it);
						foundDeletable = true;
					}
					/*if (GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pos.x, pos.y, pos.z, point.x, point.y, point.z, true) < 10.0f && pos.z < point.z - 1.0f)
					{
						PED::DELETE_PED(&(*it));
						zombies.erase(it);
						foundDeletable = true;
					}*/
				}
				else
				{
					zombies.erase(it);
					foundDeletable = true;
				}
				it++;
			}
			zombie_deleted++;
		}
	}


	int		actualize_attacks_return_near(float nearRange)
	{
		auto	it = zombies.begin();
		int		nb = 0;

		while (it != zombies.end())
		{
			if (ENTITY::IS_AN_ENTITY((*it).zombie))
				if (!PED::_IS_PED_DEAD((*it).zombie, true))
				{
					Any	bone;
					Ped target = get_closest_pped((*it).zombie);
					if (ENTITY::IS_AN_ENTITY(target) ? PED::IS_PED_IN_ANY_VEHICLE(target, false) && (*it).lastUpdate < GetTickCount() : false)
					{
						Vector3 tCoords = ENTITY::GET_ENTITY_COORDS(target, true);
						Vector3	pCoords = ENTITY::GET_ENTITY_COORDS((*it).zombie, true);
						if (GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(tCoords.x, tCoords.y, tCoords.z, pCoords.x, pCoords.y, pCoords.z, true) < 3.0f)
							nb++;
						if (GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(tCoords.x, tCoords.y, tCoords.z, pCoords.x, pCoords.y, pCoords.z, true) < 50.0f)
							AI::TASK_GO_STRAIGHT_TO_COORD((*it).zombie, tCoords.x, tCoords.y, tCoords.z, 2.0f, -1, rand() % 360, 0.0f);
						(*it).lastUpdate = GetTickCount() + 100;
					}
					if (PED::IS_PED_STOPPED((*it).zombie) && (*it).lastUpdate < GetTickCount())
					{
						AI::CLEAR_PED_TASKS((*it).zombie);
						AI::CLEAR_PED_SECONDARY_TASK((*it).zombie);
						PED::SET_PED_KEEP_TASK((*it).zombie, false);
						PED::REGISTER_HATED_TARGETS_AROUND_PED((*it).zombie, HEAR_RANGE);
						if (!PED::IS_PED_IN_COMBAT((*it).zombie, true))
							AI::TASK_WANDER_STANDARD((*it).zombie, 0x471c4000, 0);
						PED::SET_PED_KEEP_TASK((*it).zombie, true);
						(*it).lastUpdate = GetTickCount();
					}
					if (PED::GET_PED_LAST_DAMAGE_BONE((*it).zombie, &bone))
					{
						if (!(bone == IK_Head || bone == SKEL_Head))
							ENTITY::SET_ENTITY_HEALTH((*it).zombie, ENTITY::GET_ENTITY_MAX_HEALTH((*it).zombie));
					}
				}
			it++;
		}
		return (nb);
	}

	float	get_random_mul(void)
	{
		int i = rand() % 3 - 1;

		while (i == 0)
			i = rand() % 3 - 1;
		return ((float)i);
	}

	void				place_random_zombie_near(Vector3 point, float mulZombie, bool fivem)
	{
		zone			here = get_zone_by_name(ZONE::GET_NAME_OF_ZONE(point.x, point.y, point.z));
		unsigned int	max_zombies = (unsigned int)(here.zombieMul * mulZombie * (fivem ? 0.5f : 1.0f));
		if (zombies.size() < max_zombies)
		{
			int zombiesthisframe = 0;
			while (zombies.size() < max_zombies && zombiesthisframe < MAX_ZOMBIES_PER_FRAME)
			{
				float	x = point.x + (float)(get_random_mul() * (rand() % 70 + 10.0f)), y = point.y + (float)(get_random_mul() * (rand() % 70 + 10.0f)), z = 0;
				GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(x, y, point.z, &z);
				if (z <= 0)
					GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(x, y, point.z + 1000.0f, &z);
				if (z <= 0)
					return;
				if (!CAM::IS_SPHERE_VISIBLE(x, y, z, 1.0) && !INTERIOR::GET_INTERIOR_FROM_COLLISION(x, y, z))
				{
					char* animation = animations[rand() % animations.size()];
					/*int		skin_hash = Utilities::get_hash(model);
					STREAMING::REQUEST_MODEL(skin_hash);*/
					/*while (STREAMING::HAS_MODEL_LOADED(skin_hash) == false && timeout < 100)
					{
						WAIT(0);
						timeout++;
					}
					if (timeout >= 100)
						return;*/
					STREAMING::REQUEST_ANIM_SET(animation);
					/*while (STREAMING::HAS_ANIM_SET_LOADED("move_m@drunk@verydrunk") == false && timeout < 100)
					{
						WAIT(0);
						timeout++;
					}
					if (timeout >= 100)
						return;*/
						//Ped i = PED::CREATE_PED(26, skin_hash, x, y, z, (float)(rand() % 360), true, true);
					Ped	i;
					if (NETWORK::NETWORK_IS_SESSION_STARTED())
						i = PED::CREATE_PED(26, Utilities::get_hash(zombieModels[rand() % zombieModels.size()]), x, y, z, (float)(rand() % 360), true, false);
					else
						i = PED::CREATE_RANDOM_PED(x, y, z);
					managed	a;
					a.zombie = i;
					a.lastUpdate = GetTickCount();
					zombies.push_back(a);
					PED::SET_PED_MOVEMENT_CLIPSET(i, animation, 0x3e800000);
					PED::SET_PED_DIES_WHEN_INJURED(i, false);
					//PED::SET_PED_RELATIONSHIP_GROUP_HASH(i, Utilities::get_hash("ZOMBIE"));
					PED::SET_PED_RELATIONSHIP_GROUP_HASH(i, Utilities::get_hash("COUGAR"));
					PED::SET_PED_COMBAT_ABILITY(i, 2);
					PED::SET_PED_COMBAT_MOVEMENT(i, 3);
					AUDIO::SET_AMBIENT_VOICE_NAME(i, "ALIENS");
					AUDIO::DISABLE_PED_PAIN_AUDIO(i, true);
					PED::SET_PED_COMBAT_ATTRIBUTES(i, 46, TRUE);
					PED::APPLY_PED_DAMAGE_PACK(i, "BigHitByVehicle", 0, 1.0f);
					PED::APPLY_PED_DAMAGE_PACK(i, "Explosion_Med", 0, 1.0f);
					PED::SET_PED_RANDOM_PROPS(i);
					AI::CLEAR_PED_TASKS(i);
					AI::CLEAR_PED_SECONDARY_TASK(i);
					PED::SET_PED_KEEP_TASK(i, false);
					PED::SET_PED_HEARING_RANGE(i, HEAR_RANGE);
					PED::SET_PED_COMBAT_RANGE(i, 0);
					AI::TASK_WANDER_STANDARD(i, (float)(rand() % 360), 0);
					AI::SET_PED_PATH_CAN_DROP_FROM_HEIGHT(i, true);
					AI::SET_PED_PATH_AVOID_FIRE(i, false);
					PED::SET_PED_SEEING_RANGE(i, 8.0f);
					PED::SET_PED_KEEP_TASK(i, true);
					PED::SET_PED_AS_ENEMY(i, true);
					PED::SET_PED_CAN_SMASH_GLASS(i, true, true);
					PED::SET_PED_CAN_PLAY_AMBIENT_ANIMS(i, false);
					PED::SET_PED_CAN_EVASIVE_DIVE(i, false);
					PED::SET_PED_ALERTNESS(i, 3);
				}
				zombiesthisframe++;
			}
		}
		else
			clean_a_zombie(point);
	}

	unsigned int		getZombieNumber(void)
	{
		return ((unsigned int)zombies.size());
	}

	void	ZombieMain(void)
	{
		DWORD			longTick = GetTickCount(), longTickb = GetTickCount();
		DWORD			waitforready = GetTickCount() + 2000;
		bool			devMode, fivem;
		float			mulZombie;
		srand(GetTickCount());

		while (!ENTITY::DOES_ENTITY_EXIST(PLAYER::PLAYER_PED_ID()))
			WAIT(0);
		while (!PLAYER::IS_PLAYER_CONTROL_ON(PLAYER::PLAYER_ID()))
			WAIT(0);
		{
			char	buff[100];
			GetPrivateProfileString("Options", "devMode", "0", buff, 99, "./options.ini");
			if (!strcmp("1", buff))
				devMode = true;
			else
				devMode = false;
			GetPrivateProfileString("Options", "mulZombie", "50", buff, 99, "./options.ini");
			mulZombie = (float)atof(buff);
			WritePrivateProfileString("Options", "mulZombie", buff, "./options.ini");
			fivem = Utilities::getFromFile<bool>(".\\Options.ini", "Options", "fivem", false);
			if (fivem == false)
				Utilities::writeToFile<bool>(".\\Options.ini", "Options", "fivem", false);
		}
		if (fivem == true)
			while (!NETWORK::NETWORK_IS_SESSION_STARTED())
				WAIT(0);
		//Groups::init();
		int	nb = 1;
		while (true)
		{
			if (PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false) && nb > 0)
			{
				VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false), 1.0f / ((float)nb / mulZombie * 100.0f));
				VEHICLE::SET_VEHICLE_ENGINE_HEALTH(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false), VEHICLE::GET_VEHICLE_ENGINE_HEALTH(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false)) - (float)nb / mulZombie);
				VEHICLE::SET_VEHICLE_BODY_HEALTH(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false), VEHICLE::GET_VEHICLE_BODY_HEALTH(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false)) - (float)nb / mulZombie);
			}
			PLAYER::SET_PLAYER_NOISE_MULTIPLIER(PLAYER::PLAYER_ID(), 2.0f);
			auto plcoords = ENTITY::GET_ENTITY_COORDS(PLAYER::GET_PLAYER_PED(PLAYER::PLAYER_ID()), true);
			if (GetTickCount() > longTick)
			{
				Zombie::place_random_zombie_near(plcoords, mulZombie, fivem);
				longTick = GetTickCount() + 600;
			}
			if (GetTickCount() > longTickb)
			{
				nb = Zombie::actualize_attacks_return_near(5.0f);
				if (nb > 0 && PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false))
					ENTITY::APPLY_FORCE_TO_ENTITY(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false), 3, (float)(rand() % 700) * (float)nb, (float)(rand() % 700) * (float)nb, 0 , 0,0,0,false, true, true, false, false, true);
				longTickb = GetTickCount() + 1500;
			}
			if (devMode)
			{
				Utilities::putText(Utilities::floatToString((float)Zombie::getZombieNumber()), 0.5f, 0.5f);
				Utilities::putText(ZONE::GET_NAME_OF_ZONE(plcoords.x, plcoords.y, plcoords.z), 0.5f, 0.6f);
			}
			/*if (IsKeyJustUp(VK_F7))
			{
				std::fstream	file("zones.txt", std::ios::app | std::ios::out);
				file << ZONE::GET_NAME_OF_ZONE(plcoords.x, plcoords.y, plcoords.z) << std::endl;
				file.close();
			}*/
			WAIT(0);
		}
	}
}