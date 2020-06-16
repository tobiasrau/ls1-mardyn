/**
 * @file AutoPasContainer.cpp
 * @author seckler
 * @date 19.09.18
 */
#include "AutoPasContainer.h"
#include <particleContainer/adapter/LegacyCellProcessor.h>
#include <particleContainer/adapter/VectorizedCellProcessor.h>
#include <exception>
#include "Domain.h"
#include "Simulation.h"
#include "autopas/utils/Logger.h"
#include "autopas/utils/StringUtils.h"
#include "parallel/DomainDecompBase.h"

AutoPasContainer::AutoPasContainer(double cutoff)
	: _cutoff(cutoff),
	  _traversalChoices(autopas::TraversalOption::getAllOptions()),
	  _containerChoices(autopas::ContainerOption::getAllOptions()),
	  _selectorStrategy(autopas::SelectorStrategyOption::fastestMedian),
	  _tuningStrategyOption(autopas::TuningStrategyOption::fullSearch),
	  _tuningAcquisitionFunction(autopas::AcquisitionFunctionOption::upperConfidenceBound),
	  _dataLayoutChoices{autopas::DataLayoutOption::soa},
	  _newton3Choices{autopas::Newton3Option::enabled},
	  _particlePropertiesLibrary(cutoff) {
#ifdef ENABLE_MPI
	std::stringstream logFileName;

	auto timeNow = chrono::system_clock::now();
	auto time_tNow = std::chrono::system_clock::to_time_t(timeNow);

	auto maxRank = global_simulation->domainDecomposition().getNumProcs();
	auto numDigitsMaxRank = std::to_string(maxRank).length();

	logFileName << "AutoPas_Rank" << setfill('0') << setw(numDigitsMaxRank)
				<< global_simulation->domainDecomposition().getRank() << "_"
				<< std::put_time(std::localtime(&time_tNow), "%Y-%m-%d_%H-%M-%S") << ".log";

	_logFile.open(logFileName.str());
	_autopasContainer = decltype(_autopasContainer)(_logFile);
#endif
}

/**
 * Safe method to handle autopas options.
 * It checks for exceptions, prints the exceptions and all possible options.
 */
template <typename OptionType>
auto parseAutoPasOption(XMLfileUnits &xmlconfig, const std::string &xmlString, const std::string &defaultValue) {
	try {
		return OptionType::parseOptions(
			string_utils::toLowercase(xmlconfig.getNodeValue_string(xmlString, defaultValue)));
	} catch (const std::exception &e) {
		global_log->error() << "AutoPasContainer: error when parsing " << xmlString << ":" << std::endl;
		global_log->error() << e.what() << std::endl;
		global_log->error() << "Possible options: "
							<< autopas::utils::ArrayUtils::to_string(OptionType::getAllOptions()) << std::endl;
		Simulation::exit(4432);
		// dummy return
		return decltype(OptionType::parseOptions(""))();
	}
}

void AutoPasContainer::readXML(XMLfileUnits &xmlconfig) {
	string oldPath(xmlconfig.getcurrentnodepath());

	// set default values here!

	_traversalChoices = parseAutoPasOption<autopas::TraversalOption>(xmlconfig, "allowedTraversals", "c08");

	_containerChoices = parseAutoPasOption<autopas::ContainerOption>(xmlconfig, "allowedContainers", "linked-cell");

	_selectorStrategy =
		*parseAutoPasOption<autopas::SelectorStrategyOption>(xmlconfig, "selectorStrategy", "median").begin();

	_tuningStrategyOption =
		*parseAutoPasOption<autopas::TuningStrategyOption>(xmlconfig, "tuningStrategy", "fullSearch").begin();

	_dataLayoutChoices = parseAutoPasOption<autopas::DataLayoutOption>(xmlconfig, "dataLayouts", "soa");

	_newton3Choices = parseAutoPasOption<autopas::Newton3Option>(xmlconfig, "newton3", "enabled");

	_tuningAcquisitionFunction = *parseAutoPasOption<autopas::AcquisitionFunctionOption>(
									  xmlconfig, "tuningAcquisitionFunction", "lower-confidence-bound")
									  .begin();

	_maxEvidence = (unsigned int)xmlconfig.getNodeValue_int("maxEvidence", 20);
	_tuningSamples = (unsigned int)xmlconfig.getNodeValue_int("tuningSamples", 3);
	_tuningFrequency = (unsigned int)xmlconfig.getNodeValue_int("tuningInterval", 500);

	xmlconfig.getNodeValue("rebuildFrequency", _verletRebuildFrequency);
	xmlconfig.getNodeValue("skin", _verletSkin);

	std::stringstream dataLayoutChoicesStream;
	for_each(_dataLayoutChoices.begin(), _dataLayoutChoices.end(),
			 [&](auto &choice) { dataLayoutChoicesStream << choice.to_string() << " "; });
	std::stringstream containerChoicesStream;
	for_each(_containerChoices.begin(), _containerChoices.end(),
			 [&](auto &choice) { containerChoicesStream << choice.to_string() << " "; });
	std::stringstream traversalChoicesStream;
	for_each(_traversalChoices.begin(), _traversalChoices.end(),
			 [&](auto &choice) { traversalChoicesStream << choice.to_string() << " "; });
	std::stringstream newton3ChoicesStream;
	for_each(_newton3Choices.begin(), _newton3Choices.end(),
			 [&](auto &choice) { newton3ChoicesStream << choice.to_string() << " "; });

	int valueOffset = 20;
	global_log->info() << "AutoPas configuration:" << endl
					   << setw(valueOffset) << left << "Data Layout "
					   << ": " << dataLayoutChoicesStream.str() << endl
					   << setw(valueOffset) << left << "Container "
					   << ": " << containerChoicesStream.str() << endl
					   << setw(valueOffset) << left << "Traversals "
					   << ": " << traversalChoicesStream.str() << endl
					   << setw(valueOffset) << left << "Newton3"
					   << ": " << newton3ChoicesStream.str() << endl
					   << setw(valueOffset) << left << "Tuning strategy "
					   << ": " << _tuningStrategyOption.to_string() << endl
					   << setw(valueOffset) << left << "Selector strategy "
					   << ": " << _selectorStrategy.to_string() << endl
					   << setw(valueOffset) << left << "Tuning frequency"
					   << ": " << _tuningFrequency << endl
					   << setw(valueOffset) << left << "Number of samples "
					   << ": " << _tuningSamples << endl
					   << setw(valueOffset) << left << "Tuning Acquisition Function"
					   << ": " << _tuningAcquisitionFunction.to_string() << endl
					   << setw(valueOffset) << left << "Number of evidence "
					   << ": " << _maxEvidence << endl;

	xmlconfig.changecurrentnode(oldPath);
}

bool AutoPasContainer::rebuild(double *bBoxMin, double *bBoxMax) {
	mardyn_assert(_cutoff > 0.);
	std::array<double, 3> boxMin{bBoxMin[0], bBoxMin[1], bBoxMin[2]};
	std::array<double, 3> boxMax{bBoxMax[0], bBoxMax[1], bBoxMax[2]};

	_autopasContainer.setBoxMin(boxMin);
	_autopasContainer.setBoxMax(boxMax);
	_autopasContainer.setCutoff(_cutoff);
	_autopasContainer.setVerletSkin(_verletSkin);
	_autopasContainer.setVerletRebuildFrequency(_verletRebuildFrequency);
	_autopasContainer.setTuningInterval(_tuningFrequency);
	_autopasContainer.setNumSamples(_tuningSamples);
	_autopasContainer.setSelectorStrategy(_selectorStrategy);
	_autopasContainer.setAllowedContainers(_containerChoices);
	_autopasContainer.setAllowedTraversals(_traversalChoices);
	_autopasContainer.setAllowedDataLayouts(_dataLayoutChoices);
	_autopasContainer.setAllowedNewton3Options(_newton3Choices);
	_autopasContainer.setTuningStrategyOption(_tuningStrategyOption);
	_autopasContainer.setAcquisitionFunction(_tuningAcquisitionFunction);
	_autopasContainer.setMaxEvidence(_maxEvidence);
	_autopasContainer.init();
	autopas::Logger::get()->set_level(autopas::Logger::LogLevel::debug);

	memcpy(_boundingBoxMin, bBoxMin, 3 * sizeof(double));
	memcpy(_boundingBoxMax, bBoxMax, 3 * sizeof(double));
	/// @todo return sendHaloAndLeavingTogether, (always false) for simplicity.
	return false;
}

void AutoPasContainer::update() {
	// in case we update the container before handling the invalid particles, this might lead to lost particles.
	if (not _invalidParticles.empty()) {
		global_log->error() << "AutoPasContainer: trying to update container, even though invalidParticles still "
							   "exist. This would lead to lost particle => ERROR!"
							<< std::endl;
		Simulation::exit(434);
	}

	std::tie(_invalidParticles, _hasInvalidParticles) = _autopasContainer.updateContainer();
}

void AutoPasContainer::forcedUpdate() {
	// in case we update the container before handling the invalid particles, this might lead to lost particles.
	if (not _invalidParticles.empty()) {
		global_log->error() << "AutoPasContainer: trying to force update container, even though invalidParticles still "
							   "exist. This would lead to lost particle => ERROR!"
							<< std::endl;
		Simulation::exit(435);
	}
	_hasInvalidParticles = true;
	_invalidParticles = _autopasContainer.updateContainerForced();
}

bool AutoPasContainer::addParticle(Molecule &particle, bool inBoxCheckedAlready, bool checkWhetherDuplicate,
								   const bool &rebuildCaches) {
	if (particle.inBox(_boundingBoxMin, _boundingBoxMax)) {
		_autopasContainer.addParticle(particle);
	} else {
		_autopasContainer.addOrUpdateHaloParticle(particle);
	}
	return true;
}

bool AutoPasContainer::addHaloParticle(Molecule &particle, bool inBoxCheckedAlready, bool checkWhetherDuplicate,
									   const bool &rebuildCaches) {
	_autopasContainer.addOrUpdateHaloParticle(particle);
	return true;
}

void AutoPasContainer::addParticles(std::vector<Molecule> &particles, bool checkWhetherDuplicate) {
	for (auto &particle : particles) {
		addParticle(particle, true, checkWhetherDuplicate);
	}
}

template <bool shifting>
void AutoPasContainer::traverseTemplateHelper() {
	// Generate the functor. Should be regenerated every iteration to wipe internally saved globals.
	autopas::LJFunctor<Molecule, CellType, /*applyShift*/ shifting, /*mixing*/ true, autopas::FunctorN3Modes::Both,
					   /*calculateGlobals*/ true>
		functor(_cutoff, _particlePropertiesLibrary, /*duplicatedCalculation*/ true);
#if defined(_OPENMP)
#pragma omp parallel
#endif
	for (auto iter = iterator(ParticleIterator::ALL_CELLS); iter.isValid(); ++iter) {
		iter->clearFM();
	}

	// here we call the actual autopas' iteratePairwise method to compute the forces.
	_autopasContainer.iteratePairwise(&functor);
	double upot = functor.getUpot();
	double virial = functor.getVirial();

	// _myRF is always zero for lj only!
	global_simulation->getDomain()->setLocalVirial(virial /*+ 3.0 * _myRF*/);
	// _upotXpoles is zero as we do not have any dipoles or quadrupoles
	global_simulation->getDomain()->setLocalUpot(upot /* _upotXpoles + _myRF*/);
}

void AutoPasContainer::traverseCells(CellProcessor &cellProcessor) {
	if (dynamic_cast<VectorizedCellProcessor *>(&cellProcessor) or
		dynamic_cast<LegacyCellProcessor *>(&cellProcessor)) {
		// only initialize ppl if it is empty
		bool hasShift = false;
		bool hasNoShift = false;
		size_t numComponentsAdded = 0;
		if (_particlePropertiesLibrary.getTypes().empty()) {
			auto components = global_simulation->getEnsemble()->getComponents();
			for (auto &c : *components) {
				_particlePropertiesLibrary.addType(c.getLookUpId(), c.ljcenter(0).eps(), c.ljcenter(0).sigma(),
												   c.ljcenter(0).m());
				if (c.ljcenter(0).shift6() != 0.) {
					hasShift = true;
					double autoPasShift6 =
						_particlePropertiesLibrary.mixingShift6(numComponentsAdded, numComponentsAdded);
					double ls1Shift6 = c.ljcenter(0).shift6();
					if (std::fabs((autoPasShift6 - ls1Shift6) / ls1Shift6) > 1.e-10) {
						// warn if shift differs relatively by more than 1.e-10
						global_log->warning() << "Dangerous shift6 detected: AutoPas will use: " << autoPasShift6
											  << ", while normal ls1 mode uses: " << ls1Shift6 << std::endl
											  << "Please check that your shifts are calculated correctly." << std::endl;
					}
					++numComponentsAdded;
				} else {
					hasNoShift = true;
				}
			}
		}
		if (hasShift and hasNoShift) {
			// if some particles require shifting and some don't:
			// throw an error, as AutoPas does not support this, yet.
			throw std::runtime_error("AutoPas does not support mixed shifting state!");
		}
		if (hasShift) {
			traverseTemplateHelper</*shifting*/ true>();
		} else {
			traverseTemplateHelper</*shifting*/ false>();
		}

	} else {
		global_log->warning() << "only lj functors are supported for traversals." << std::endl;
	}
}

void AutoPasContainer::traverseNonInnermostCells(CellProcessor &cellProcessor) {
	throw std::runtime_error("AutoPasContainer::traverseNonInnermostCells() not yet implemented");
}

void AutoPasContainer::traversePartialInnermostCells(CellProcessor &cellProcessor, unsigned int stage, int stageCount) {
	throw std::runtime_error("AutoPasContainer::traversePartialInnermostCells() not yet implemented");
}

unsigned long AutoPasContainer::getNumberOfParticles() {
	unsigned long count = 0;
	for (auto iter = iterator(ParticleIterator::ONLY_INNER_AND_BOUNDARY); iter.isValid(); ++iter) {
		++count;
	}
	return count;
	// return _autopasContainer.getNumberOfParticles(); // todo: this is currently buggy!, so we use iterators instead.
}

void AutoPasContainer::clear() { _autopasContainer.deleteAllParticles(); }

void AutoPasContainer::deleteOuterParticles() {
	global_log->info() << "deleting outer particles by using forced update" << std::endl;
	auto invalidParticles = _autopasContainer.updateContainerForced();
	if (not invalidParticles.empty()) {
		throw std::runtime_error(
			"AutoPasContainer: Invalid particles ignored in deleteOuterParticles, check that your rebalance rate is a "
			"multiple of the rebuild rate!");
	}
}

double AutoPasContainer::get_halo_L(int /*index*/) const { return _cutoff; }

double AutoPasContainer::getCutoff() const { return _cutoff; }

double AutoPasContainer::getInteractionLength() const { return _cutoff + _verletSkin; }

double AutoPasContainer::getSkin() const { return _verletSkin; }

void AutoPasContainer::deleteMolecule(ParticleIterator &moleculeIter, const bool & /*rebuildCaches*/) {
	_autopasContainer.deleteParticle(moleculeIter);
}

double AutoPasContainer::getEnergy(ParticlePairsHandler *particlePairsHandler, Molecule *m1,
								   CellProcessor &cellProcessor) {
	throw std::runtime_error("AutoPasContainer::getEnergy() not yet implemented");
}

void AutoPasContainer::updateInnerMoleculeCaches() {
	throw std::runtime_error("AutoPasContainer::updateInnerMoleculeCaches() not yet implemented");
}

void AutoPasContainer::updateBoundaryAndHaloMoleculeCaches() {
	throw std::runtime_error("AutoPasContainer::updateBoundaryAndHaloMoleculeCaches() not yet implemented");
}

void AutoPasContainer::updateMoleculeCaches() {
	// nothing needed
}

bool AutoPasContainer::getMoleculeAtPosition(const double *pos, Molecule **result) {
	std::array<double, 3> pos_arr{pos[0], pos[1], pos[2]};
	for (auto iter = this->iterator(ParticleIterator::ALL_CELLS); iter.isValid(); ++iter) {
		if (iter->getR() == pos_arr) {
			*result = &(*iter);
			return true;
		}
	}
	return false;
}

unsigned long AutoPasContainer::initCubicGrid(std::array<unsigned long, 3> numMoleculesPerDimension,
											  std::array<double, 3> simBoxLength) {
	throw std::runtime_error("AutoPasContainer::initCubicGrid() not yet implemented");
}

double *AutoPasContainer::getCellLength() {
	throw std::runtime_error("AutoPasContainer::getCellLength() not yet implemented");
}

double *AutoPasContainer::getHaloSize() {
	static std::array<double, 3> haloLength{_verletSkin + _cutoff};
	return haloLength.data();
}

autopas::IteratorBehavior convertBehaviorToAutoPas(ParticleIterator::Type t) {
	switch (t) {
		case ParticleIterator::Type::ALL_CELLS:
			return autopas::IteratorBehavior::haloAndOwned;
		case ParticleIterator::Type::ONLY_INNER_AND_BOUNDARY:
			return autopas::IteratorBehavior::ownedOnly;
	}
	throw std::runtime_error("Unknown iterator type.");
}

ParticleIterator AutoPasContainer::iterator(ParticleIterator::Type t) {
	return _autopasContainer.begin(convertBehaviorToAutoPas(t));
}

RegionParticleIterator AutoPasContainer::regionIterator(const double *startCorner, const double *endCorner,
														ParticleIterator::Type t) {
	std::array<double, 3> lowCorner{startCorner[0], startCorner[1], startCorner[2]};
	std::array<double, 3> highCorner{endCorner[0], endCorner[1], endCorner[2]};
	return RegionParticleIterator{
		_autopasContainer.getRegionIterator(lowCorner, highCorner, convertBehaviorToAutoPas(t))};
}
