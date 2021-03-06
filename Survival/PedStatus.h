#pragma once
#include "..\Common\script.h"
#include "Defines.h"
#include "Inventory.h"

class PedStatus
{
public:
	PedStatus(Ped pedId);
	virtual ~PedStatus();
	float			getFood(void);
	float			getWater(void);
	void			addFood(float);
	void			addWater(float);
	Ped				getId(void);
	virtual int		tick(void);
	DWORD			timeBeforeRespawn(void) const { return (respawnTimer == 0 ? 0 : respawnTimer - GetTickCount()); }
	Inventory		*getInventory(void) { return (&(this->inventory)); }
	Vector3			getPos(void) { return (ENTITY::GET_ENTITY_COORDS(pedId, true)); }
protected:
	int				doTick(void);
	float			food;
	float			water;
	Ped				pedId, lastPedId;
	DWORD			time, respawnTimer;
	Inventory		inventory;
	bool			dead, moved;
	Vector3			targetPos;
};

