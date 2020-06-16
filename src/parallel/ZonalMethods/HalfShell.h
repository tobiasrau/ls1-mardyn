/*
 * CommunicationScheme.h
 *
 *  Created on: 14.07.2017
 *      Author: sauermann
 */

#pragma once

#include "ZonalMethod.h"

/**
 * This class implements the HalfShell Method. Halo is only imported, if it lies within the half shell.
 * The half shell is defined using the same traversal scheme as the VectorizedCellProcessor uses for determining,
 * whether or whether not to calculate the macroscopic values, i.e: cellIndex1 < cellIndex2.
 * As LinkedCells defines the cellIndex using z as the major array index (then y, then x), we will do the same here:
 * if z<z_domain, we don't import the region. if z>z_domain we import it.
 * if z within z_domain: y determines the procedure.
 * The HalfShell method requires forces to be exchanged.
 */
class HalfShell: public ZonalMethod {
public:
	HalfShell() = default;
	~HalfShell() override = default;

	std::vector<HaloRegion> getHaloImportForceExportRegions(HaloRegion& initialRegion, double cutoffRadius, double /*skin*/,
			bool coversWholeDomain[3], double cellLength[3]) override {
		auto condition = [](const int d[3])->bool {
			// TODO reenable this check and fix halfshelltraversal
			//int pseudoCellIndex = ((d[2] * 2) + d[1]) * 2 + d[0];
			//return pseudoCellIndex > 0;
			return true;
		};
		return getHaloRegionsConditional(initialRegion, cutoffRadius, 0., coversWholeDomain, condition);
	}

	std::vector<HaloRegion> getHaloExportForceImportRegions(HaloRegion& initialRegion, double cutoffRadius, double /*skin*/,
			bool coversWholeDomain[3], double cellLength[3]) override {
		auto condition = [](const int d[3])->bool {
			// TODO reenable this check and fix halfshelltraversal
			//int pseudoCellIndex = ((d[2] * 2) + d[1]) * 2 + d[0];
			//return pseudoCellIndex < 0;
			return true;
		};
		return getHaloRegionsConditionalInside(initialRegion, cutoffRadius, 0., coversWholeDomain, condition);
	}
};

