/*
 * LeafNodesContainer.h
 *
 *  Created on: Feb 6, 2015
 *      Author: tchipev
 *
 *  A bare-bones secondary Linked-Cells structure to allow
 *  carrying out P2P, P2M, L2P operations when the tree is deeper
 *  than what would be allowed by the Lennard-Jones cutoff radius.
 *
 *  Assumption: there is an LJ Linked-Cells structure with cells bigger or equal than these.
 */

#ifndef LEAFNODESCONTAINER_H_
#define LEAFNODESCONTAINER_H_

#include "particleContainer/ParticleCell.h"
#include <vector>

class CellProcessor;

namespace bhfmm {

class LeafNodesContainer {
public:
	LeafNodesContainer(double bBoxMin[3], double bBoxMax[3],
		double LJCellLength[3], unsigned subdivisionFactor,
		bool periodic = true);

	~LeafNodesContainer();

	double* getCellLength() {
		return _cellLength;
	}

	void addParticle(Molecule& particle);
	void clearParticles();
	void traverseCellPairs(CellProcessor& cellProcessor);

private:
	void initializeCells();
	long int cellIndexOf3DIndex(int xIndex, int yIndex, int zIndex) const;
	void calculateNeighbourIndices();
	unsigned long int getCellIndexOfMolecule(Molecule* molecule) const;

	bool _periodicBC;

	double _boundingBoxMin[3];
	double _boundingBoxMax[3];
	double _haloBoundingBoxMin[3];
	double _haloBoundingBoxMax[3];
	double _cellLength[3];
	int _numInnerCellsPerDimension[3];
	int _numCellsPerDimension[3];
	std::vector<ParticleCell> _cells;
	std::vector<unsigned long> _forwardNeighbourOffsets;
	std::vector<unsigned long> _backwardNeighbourOffsets;
	unsigned _maxNeighbourOffset;
	unsigned _minNeighbourOffset;

};

} /* namespace bhfmm */

#endif /* LEAFNODESCONTAINER_H_ */