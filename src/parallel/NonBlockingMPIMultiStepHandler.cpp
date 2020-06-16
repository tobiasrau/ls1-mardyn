/*
 * NonBlockingMPIMultiStepHandler.cpp
 *
 *  Created on: Apr 28, 2016
 *      Author: seckler
 */

#include "Simulation.h"
#include "NonBlockingMPIMultiStepHandler.h"
#include <assert.h>
#include "utils/Logger.h"
#include "DomainDecompMPIBase.h"
#include "particleContainer/ParticleContainer.h"
#include "particleContainer/adapter/CellProcessor.h"

using Log::global_log;
using namespace std;

NonBlockingMPIMultiStepHandler::NonBlockingMPIMultiStepHandler(DomainDecompMPIBase* domainDecomposition,
		ParticleContainer* moleculeContainer, Domain* domain, CellProcessor* cellProcessor) :
				NonBlockingMPIHandlerBase(domainDecomposition, moleculeContainer, domain, cellProcessor) {
	// just calling the constructor of the base class is enough
}

NonBlockingMPIMultiStepHandler::~NonBlockingMPIMultiStepHandler() {
	// nothing to be done
}

void NonBlockingMPIMultiStepHandler::performComputation() {
	int stageCount = _domainDecomposition->getNonBlockingStageCount();

	mardyn_assert(stageCount > 0);

	global_simulation->timers()->start("SIMULATION_COMPUTATION");
	global_simulation->timers()->start("SIMULATION_FORCE_CALCULATION");
	_cellProcessor->initTraversal();
	global_simulation->timers()->stop("SIMULATION_FORCE_CALCULATION");
	global_simulation->timers()->stop("SIMULATION_COMPUTATION");
	for (unsigned int i = 0; i < static_cast<unsigned int>(stageCount); ++i) {
#ifndef ADVANCED_OVERLAPPING
		global_simulation->timers()->start("SIMULATION_DECOMPOSITION");
		_domainDecomposition->prepareNonBlockingStage(false, _moleculeContainer, _domain, i);
		global_simulation->timers()->stop("SIMULATION_DECOMPOSITION");
		// Force calculation and other pair interaction related computations
		global_log->debug() << "Traversing innermost cells" << std::endl;
		global_simulation->timers()->start("SIMULATION_COMPUTATION");
		global_simulation->timers()->start("SIMULATION_FORCE_CALCULATION");
		_moleculeContainer->traversePartialInnermostCells(*_cellProcessor, i, stageCount);
		global_simulation->timers()->stop("SIMULATION_FORCE_CALCULATION");
		global_simulation->timers()->stop("SIMULATION_COMPUTATION");

		global_simulation->timers()->start("SIMULATION_DECOMPOSITION");
		_domainDecomposition->finishNonBlockingStage(false, _moleculeContainer, _domain, i);
		global_simulation->timers()->stop("SIMULATION_DECOMPOSITION");
#else
		omp_set_dynamic(0);
		omp_set_nested(1);
		int max_threads = omp_get_max_threads();
		#pragma omp parallel num_threads(2)
		{
			#pragma omp master
			{
				omp_set_num_threads(1);
				global_simulation->timers()->start("SIMULATION_DECOMPOSITION");
				_domainDecomposition->prepareNonBlockingStage(false, _moleculeContainer, _domain, i);
				_domainDecomposition->finishNonBlockingStage(false, _moleculeContainer, _domain, i);
				global_simulation->timers()->stop("SIMULATION_DECOMPOSITION");
			}
			#pragma omp single
			{
				omp_set_num_threads(max_threads - 1);
				global_simulation->timers()->start("SIMULATION_COMPUTATION");
				global_simulation->timers()->start("SIMULATION_FORCE_CALCULATION");
				_moleculeContainer->traversePartialInnermostCells(*_cellProcessor, i, stageCount);
				global_simulation->timers()->stop("SIMULATION_FORCE_CALCULATION");
				global_simulation->timers()->stop("SIMULATION_COMPUTATION");
			}
		}
#endif

	}

	global_simulation->timers()->start("SIMULATION_DECOMPOSITION");
	_moleculeContainer->updateBoundaryAndHaloMoleculeCaches();  // update the caches of the other molecules (non-inner cells)
	global_simulation->timers()->stop("SIMULATION_DECOMPOSITION");

	// remaining force calculation and other pair interaction related computations
	global_log->debug() << "Traversing non-innermost cells" << std::endl;
	global_simulation->timers()->start("SIMULATION_COMPUTATION");
	global_simulation->timers()->start("SIMULATION_FORCE_CALCULATION");
	_moleculeContainer->traverseNonInnermostCells(*_cellProcessor);
	_cellProcessor->endTraversal();

	// siteWiseForces Plugin Call
	global_log -> debug() << "[SITEWISE FORCES] Performing siteWiseForces plugin (nonBlocking) call" << endl;
	for (PluginBase* plugin : *global_simulation->getPluginList()) {
		global_log -> debug() << "[SITEWISE FORCES] Plugin: " << plugin->getPluginName() << endl;
		plugin->siteWiseForces(_moleculeContainer, _domainDecomposition, global_simulation->getSimulationStep());
	}

	// Update forces in molecules so they can be exchanged - new - begin

	for (auto i = _moleculeContainer->iterator(ParticleIterator::ALL_CELLS); i.isValid(); ++i){
		i->calcFM();
	}
	// - new - end
	
	global_simulation->timers()->stop("SIMULATION_FORCE_CALCULATION");
	global_simulation->timers()->stop("SIMULATION_COMPUTATION");
	

	// - new - begin
	global_simulation->timers()->start("SIMULATION_DECOMPOSITION");
	// Exchange forces if it's required by the cell container.
	if(_moleculeContainer->requiresForceExchange()){
		_domainDecomposition->exchangeForces(_moleculeContainer, _domain);
	}
	global_simulation->timers()->stop("SIMULATION_DECOMPOSITION");
	// - new - end
}

void NonBlockingMPIMultiStepHandler::initBalanceAndExchange(bool forceRebalancing, double etime) {

	mardyn_assert(!forceRebalancing);

	global_simulation->timers()->start("SIMULATION_DECOMPOSITION");
	_domainDecomposition->balanceAndExchangeInitNonBlocking(forceRebalancing, _moleculeContainer, _domain);

	// The cache of the molecules must be updated/build after the exchange process,
	// as the cache itself isn't transferred
	_moleculeContainer->updateInnerMoleculeCaches(); // only the caches of the innermost molecules have to be updated.

	global_simulation->timers()->stop("SIMULATION_DECOMPOSITION");
}

