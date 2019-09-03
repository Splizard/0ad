/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_CCMPPATHFINDER_COMMON
#define INCLUDED_CCMPPATHFINDER_COMMON

/**
 * @file
 * Declares CCmpPathfinder. Its implementation is mainly done in CCmpPathfinder.cpp,
 * but the short-range (vertex) pathfinding is done in CCmpPathfinder_Vertex.cpp.
 * This file provides common code needed for both files.
 *
 * The long-range pathfinding is done by a LongPathfinder object.
 */

#include "simulation2/system/Component.h"

#include "ICmpPathfinder.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/components/ICmpObstructionManager.h"


class HierarchicalPathfinder;
class LongPathfinder;
class VertexPathfinder;

class SceneCollector;
class AtlasOverlay;

#ifdef NDEBUG
#define PATHFIND_DEBUG 0
#else
#define PATHFIND_DEBUG 1
#endif

/**
 * Implementation of ICmpPathfinder
 */
class CCmpPathfinder final : public ICmpPathfinder
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
		componentManager.SubscribeToMessageType(MT_WaterChanged);
		componentManager.SubscribeToMessageType(MT_ObstructionMapShapeChanged);
		componentManager.SubscribeToMessageType(MT_TurnStart);
	}

	~CCmpPathfinder();

	DEFAULT_COMPONENT_ALLOCATOR(Pathfinder)

	// Template state:

	std::map<std::string, pass_class_t> m_PassClassMasks;
	std::vector<PathfinderPassability> m_PassClasses;

	// Dynamic state:

	std::vector<LongPathRequest> m_LongPathRequests;
	std::vector<ShortPathRequest> m_ShortPathRequests;
	u32 m_NextAsyncTicket; // unique IDs for asynchronous path requests
	u16 m_SameTurnMovesCount; // current number of same turn moves we have processed this turn

	// Lazily-constructed dynamic state (not serialized):

	u16 m_MapSize; // tiles per side
	Grid<NavcellData>* m_Grid; // terrain/passability information
	Grid<NavcellData>* m_TerrainOnlyGrid; // same as m_Grid, but only with terrain, to avoid some recomputations

	// Keep clever updates in memory to avoid memory fragmentation from the grid.
	// This should be used only in UpdateGrid(), there is no guarantee the data is properly initialized anywhere else.
	GridUpdateInformation m_DirtinessInformation;
	// The data from clever updates is stored for the AI manager
	GridUpdateInformation m_AIPathfinderDirtinessInformation;
	bool m_TerrainDirty;

	std::unique_ptr<VertexPathfinder> m_VertexPathfinder;
	std::unique_ptr<HierarchicalPathfinder> m_PathfinderHier;
	std::unique_ptr<LongPathfinder> m_LongPathfinder;

	// For responsiveness we will process some moves in the same turn they were generated in

	u16 m_MaxSameTurnMoves; // max number of moves that can be created and processed in the same turn

	AtlasOverlay* m_AtlasOverlay;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& paramNode);

	virtual void Deinit();

	template<typename S>
	void SerializeCommon(S& serialize);

	virtual void Serialize(ISerializer& serialize);

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize);

	virtual void HandleMessage(const CMessage& msg, bool global);

	virtual pass_class_t GetPassabilityClass(const std::string& name) const;

	virtual void GetPassabilityClasses(std::map<std::string, pass_class_t>& passClasses) const;
	virtual void GetPassabilityClasses(
		std::map<std::string, pass_class_t>& nonPathfindingPassClasses,
		std::map<std::string, pass_class_t>& pathfindingPassClasses) const;

	const PathfinderPassability* GetPassabilityFromMask(pass_class_t passClass) const;

	virtual entity_pos_t GetClearance(pass_class_t passClass) const
	{
		const PathfinderPassability* passability = GetPassabilityFromMask(passClass);
		if (!passability)
			return fixed::Zero();

		return passability->m_Clearance;
	}

	virtual entity_pos_t GetMaximumClearance() const
	{
		entity_pos_t max = fixed::Zero();

		for (const PathfinderPassability& passability : m_PassClasses)
			if (passability.m_Clearance > max)
				max = passability.m_Clearance;

		return max + Pathfinding::CLEARANCE_EXTENSION_RADIUS;
	}

	virtual const Grid<NavcellData>& GetPassabilityGrid();

	virtual const GridUpdateInformation& GetAIPathfinderDirtinessInformation() const
	{
		return m_AIPathfinderDirtinessInformation;
	}

	virtual void FlushAIPathfinderDirtinessInformation()
	{
		m_AIPathfinderDirtinessInformation.Clean();
	}

	virtual Grid<u16> ComputeShoreGrid(bool expandOnWater = false);

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, WaypointPath& ret) const;

	virtual u32 ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, entity_id_t notify);

	virtual WaypointPath ComputeShortPath(const ShortPathRequest& request) const;

	virtual u32 ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t clearance, entity_pos_t range, const PathGoal& goal, pass_class_t passClass, bool avoidMovingUnits, entity_id_t controller, entity_id_t notify);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass);

	virtual void SetDebugOverlay(bool enabled);

	virtual void SetHierDebugOverlay(bool enabled);

	virtual void GetDebugData(u32& steps, double& time, Grid<u8>& grid) const;

	virtual void SetAtlasOverlay(bool enable, pass_class_t passClass = 0);

	virtual bool CheckMovement(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, pass_class_t passClass) const;

	virtual ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, pass_class_t passClass, bool onlyCenterPoint) const;

	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass) const;

	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass, bool onlyCenterPoint) const;

	virtual void FinishAsyncRequests();

	void ProcessLongRequests(const std::vector<LongPathRequest>& longRequests);

	void ProcessShortRequests(const std::vector<ShortPathRequest>& shortRequests);

	virtual void ProcessSameTurnMoves();

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	virtual void UpdateGrid();

	/**
	 * Updates the terrain-only grid without updating the dirtiness informations.
	 * Useful for fast passability updates in Atlas.
	 */
	void MinimalTerrainUpdate();

	/**
	 * Regenerates the terrain-only grid.
	 * Atlas doesn't need to have passability cells expanded.
	 */
	void TerrainUpdateHelper(bool expandPassability = true);

	void RenderSubmit(SceneCollector& collector);
};

class AtlasOverlay : public TerrainTextureOverlay
{
public:
	const CCmpPathfinder* m_Pathfinder;
	pass_class_t m_PassClass;

	AtlasOverlay(const CCmpPathfinder* pathfinder, pass_class_t passClass) :
		TerrainTextureOverlay(Pathfinding::NAVCELLS_PER_TILE), m_Pathfinder(pathfinder), m_PassClass(passClass)
	{
	}

	virtual void BuildTextureRGBA(u8* data, size_t w, size_t h)
	{
		// Render navcell passability, based on the terrain-only grid
		u8* p = data;
		for (size_t j = 0; j < h; ++j)
		{
			for (size_t i = 0; i < w; ++i)
			{
				SColor4ub color(0, 0, 0, 0);
				if (!IS_PASSABLE(m_Pathfinder->m_TerrainOnlyGrid->get((int)i, (int)j), m_PassClass))
					color = SColor4ub(255, 0, 0, 127);

				*p++ = color.R;
				*p++ = color.G;
				*p++ = color.B;
				*p++ = color.A;
			}
		}
	}
};

#endif // INCLUDED_CCMPPATHFINDER_COMMON
