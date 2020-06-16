/*
 * DomainDecompBaseMPI.cpp
 *
 *  Created on: Nov 15, 2015
 *      Author: tchipevn
 */
#include <memory>
#include <algorithm>

#include "DomainDecompMPIBase.h"
#include "molecules/Molecule.h"
#include "particleContainer/ParticleContainer.h"
#include "Simulation.h"
#include "parallel/NeighbourCommunicationScheme.h"
#include "ParticleData.h"

#include "parallel/ZonalMethods/FullShell.h"
#include "parallel/ZonalMethods/EighthShell.h"
#include "parallel/ZonalMethods/HalfShell.h"
#include "parallel/ZonalMethods/Midpoint.h"
#include "parallel/ZonalMethods/NeutralTerritory.h"
#include "parallel/CollectiveCommunication.h" // new
#include "parallel/CollectiveCommunicationNonBlocking.h" // new

using Log::global_log;
using std::endl;

DomainDecompMPIBase::DomainDecompMPIBase() :
		_comm(MPI_COMM_WORLD) {
#ifndef MARDYN_AUTOPAS
	_neighbourCommunicationScheme = new IndirectNeighbourCommunicationScheme(new FullShell());
#else
	// direct push-pull
	_neighbourCommunicationScheme = new DirectNeighbourCommunicationScheme(new FullShell(), true);
#endif
	//_neighbourCommunicationScheme = new DirectNeighbourCommunicationScheme(new FullShell());

	MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &_rank));

	MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &_numProcs));

	ParticleData::getMPIType(_mpiParticleType);

	_collCommunication = std::unique_ptr<CollectiveCommunicationInterface>(new CollectiveCommunication());
	//_collCommunication = std::unique_ptr<CollectiveCommunicationInterface>(new CollectiveCommunicationNonBlocking());
}

DomainDecompMPIBase::~DomainDecompMPIBase() {

	delete _neighbourCommunicationScheme;
	_neighbourCommunicationScheme = nullptr; // do you need both?

	MPI_Type_free(&_mpiParticleType);

	// MPI_COMM_WORLD doesn't need to be freed, so
	// if a derived class does something with the communicator
	// then the derived class should also free it
}

void DomainDecompMPIBase::readXML(XMLfileUnits& xmlconfig) {
	// store current path
	string oldPath(xmlconfig.getcurrentnodepath());

#ifndef MARDYN_AUTOPAS
	std::string neighbourCommunicationScheme = "indirect";
	xmlconfig.getNodeValue("CommunicationScheme", neighbourCommunicationScheme);
#else
	std::string neighbourCommunicationScheme = "direct-pp";
#endif

	std::string zonalMethod = "fs";
	std::string traversal = "c08"; // currently useless, as traversal is set elsewhere
	
	xmlconfig.changecurrentnode("../datastructure");
	xmlconfig.getNodeValue("traversalSelector", traversal);
	transform(traversal.begin(),
	          traversal.end(),
	          traversal.begin(),
	          ::tolower);
	// currently only checks, if traversal is valid - should check, if zonal method/traversal is valid
	if(traversal.find("hs") != string::npos || traversal.find("mp") != string::npos /* || traversal == "nt" */) {
		zonalMethod = traversal;
	} else if(traversal.find("es") != string::npos){
		zonalMethod = "es";
	}
	else{
		global_log->info() << "Defaulting to fs/c08" << std::endl;

		zonalMethod = "fs";
		traversal = "c08";
	}

	
	global_log->info() << "variable zonalMethod is: " << zonalMethod << std::endl;
	setCommunicationScheme(neighbourCommunicationScheme, zonalMethod);

	// reset path
	xmlconfig.changecurrentnode(oldPath);

	bool overlappingCollectives = false;
	xmlconfig.getNodeValue("overlappingCollectives", overlappingCollectives);
	if(overlappingCollectives) {
		global_log->info() << "DomainDecompMPIBase: Using Overlapping Collectives" << endl;
#if MPI_VERSION >= 3
		_collCommunication = std::unique_ptr<CollectiveCommunicationInterface>(new CollectiveCommunicationNonBlocking());
#else
		global_log->warning() << "DomainDecompMPIBase: Can not use overlapping collectives, as the MPI version is less than MPI 3." << endl;
#endif
	} else {
		global_log->info() << "DomainDecompMPIBase: NOT Using Overlapping Collectives" << endl;
	}
}

int DomainDecompMPIBase::getNonBlockingStageCount() {
	return _neighbourCommunicationScheme->getCommDims();
}

void DomainDecompMPIBase::setCommunicationScheme(std::string scheme, std::string zonalMethod) {
	// delete if it exists already
	delete _neighbourCommunicationScheme;
	_neighbourCommunicationScheme = nullptr;

	ZonalMethod* zonalMethodP = nullptr;

	// CommunicationScheme will delete the pointer
	if(zonalMethod=="fs") {
		zonalMethodP = new FullShell();
	} else if(zonalMethod=="es") {
		zonalMethodP = new EighthShell();
	} else if(zonalMethod=="hs") {
		zonalMethodP = new HalfShell();
	} else if(zonalMethod=="mp") {
		zonalMethodP = new Midpoint();
	} else if(zonalMethod=="nt") {
		zonalMethodP = new NeutralTerritory();
	} else {
		global_log->error() << "DomainDecompMPIBase: invalid zonal method specified. Valid values are 'fs', 'hs', 'mp' and 'nt'"
				<< std::endl;
		Simulation::exit(1);
	}
	global_log->info() << "Using zonal method: " << zonalMethod << std::endl;

	if (scheme=="direct") {
		global_log->info() << "DomainDecompMPIBase: Using DirectCommunicationScheme without push-pull neighbors" << std::endl;
		_neighbourCommunicationScheme = new DirectNeighbourCommunicationScheme(zonalMethodP, false);
	} else if(scheme=="direct-pp") {
		global_log->info() << "DomainDecompMPIBase: Using DirectCommunicationScheme with push-pull neighbors" << std::endl;
		_neighbourCommunicationScheme = new DirectNeighbourCommunicationScheme(zonalMethodP, true);
	} else if(scheme=="indirect") {
		global_log->info() << "DomainDecompMPIBase: Using IndirectCommunicationScheme" << std::endl;
		_neighbourCommunicationScheme = new IndirectNeighbourCommunicationScheme(zonalMethodP);
	} else {
		global_log->error() << "DomainDecompMPIBase: invalid NeighbourCommunicationScheme specified. Valid values are 'direct' and 'indirect'"
				<< std::endl;
		Simulation::exit(1);
	}
}

unsigned DomainDecompMPIBase::Ndistribution(unsigned localN, float* minrnd, float* maxrnd) {
	unsigned* moldistribution = new unsigned[_numProcs];
	MPI_CHECK(MPI_Allgather(&localN, 1, MPI_UNSIGNED, moldistribution, 1, MPI_UNSIGNED, _comm));
	unsigned globalN = 0;
	for (int r = 0; r < _rank; r++)
		globalN += moldistribution[r];
	unsigned localNbottom = globalN;
	globalN += moldistribution[_rank];
	unsigned localNtop = globalN;
	for (int r = _rank + 1; r < _numProcs; r++)
		globalN += moldistribution[r];
	delete[] moldistribution;
	*minrnd = (float) localNbottom / globalN;
	*maxrnd = (float) localNtop / globalN;
	return globalN;
}

void DomainDecompMPIBase::assertIntIdentity(int IX) {
	if (_rank)
		MPI_CHECK(MPI_Send(&IX, 1, MPI_INT, 0, 2 * _rank + 17, _comm));
	else {
		int recv;
		MPI_Status s;
		for (int i = 1; i < _numProcs; i++) {
			MPI_CHECK(MPI_Recv(&recv, 1, MPI_INT, i, 2 * i + 17, _comm, &s));
			if (recv != IX) {
				global_log->error() << "IX is " << IX << " for rank 0, but " << recv << " for rank " << i << ".\n";
				MPI_Abort(MPI_COMM_WORLD, 911);
			}
		}
		global_log->info() << "IX = " << recv << " for all " << _numProcs << " ranks.\n";
	}
}

void DomainDecompMPIBase::assertDisjunctivity(ParticleContainer* moleculeContainer) const {
	using std::map;
	using std::endl;

	if (_rank) {
		unsigned long num_molecules = moleculeContainer->getNumberOfParticles();
		std::vector<unsigned long> tids(num_molecules);

		int i = 0;
		for (auto m = moleculeContainer->iterator(ParticleIterator::ONLY_INNER_AND_BOUNDARY); m.isValid(); ++m) {
			tids[i] = m->getID();
			i++;
		}
		MPI_CHECK(MPI_Send(tids.data(), num_molecules, MPI_UNSIGNED_LONG, 0, 2674 + _rank, _comm));
		global_log->info() << "Data consistency checked: for results see rank 0." << endl;
	} else {
		/** @todo FIXME: This implementation does not scale. */
		map<unsigned long, int> check;

		for (auto m = moleculeContainer->iterator(ParticleIterator::ONLY_INNER_AND_BOUNDARY); m.isValid(); ++m) {
			if(check.find(m->getID()) != check.end()){
				global_log->error() << "Rank 0 contains a duplicated particle with id " << m->getID() << std::endl;
				MPI_Abort(MPI_COMM_WORLD, 1);
			}
			check[m->getID()] = 0;
		}
		MPI_Status status;
		for (int i = 1; i < _numProcs; i++) {
			int num_recv = 0;
			MPI_CHECK(MPI_Probe(i, 2674 + i, _comm, &status));
			MPI_CHECK(MPI_Get_count(&status, MPI_UNSIGNED_LONG, &num_recv));
			std::vector<unsigned long> recv(num_recv);

			MPI_CHECK(MPI_Recv(recv.data(), num_recv, MPI_UNSIGNED_LONG, i, 2674 + i, _comm, &status));
			for (int j = 0; j < num_recv; j++) {
				if (check.find(recv[j]) != check.end()) {
					global_log->error() << "Ranks " << check[recv[j]] << " and " << i << " both propagate ID "
							<< recv[j] << endl;
					MPI_Abort(MPI_COMM_WORLD, 1);
				} else
					check[recv[j]] = i;
			}
		}
		global_log->info() << "Data consistency checked: No duplicate IDs detected among " << check.size()
				<< " entries." << endl;
	}
}

void DomainDecompMPIBase::balanceAndExchangeInitNonBlocking(bool /*forceRebalancing*/,
		ParticleContainer* /*moleculeContainer*/, Domain* /*domain*/) {
	// for now, nothing to be done here
	// later switch between different communication schemes might go in here.
}

void DomainDecompMPIBase::prepareNonBlockingStageImpl(ParticleContainer* moleculeContainer, Domain* domain,
		unsigned int stageNumber, MessageType msgType, bool removeRecvDuplicates) {
	mardyn_assert(stageNumber < _neighbourCommunicationScheme->getCommDims());
	_neighbourCommunicationScheme->prepareNonBlockingStageImpl(moleculeContainer, domain, stageNumber, msgType,
			removeRecvDuplicates, this);
}

void DomainDecompMPIBase::finishNonBlockingStageImpl(ParticleContainer* moleculeContainer, Domain* domain,
		unsigned int stageNumber, MessageType msgType, bool removeRecvDuplicates) {
	_neighbourCommunicationScheme->finishNonBlockingStageImpl(moleculeContainer, domain, stageNumber, msgType,
			removeRecvDuplicates, this);
}

void DomainDecompMPIBase::exchangeMoleculesMPI(ParticleContainer* moleculeContainer, Domain* domain,
		MessageType msgType, bool doHaloPositionCheck, bool removeRecvDuplicates) {

	global_log->set_mpi_output_all();

	_neighbourCommunicationScheme->exchangeMoleculesMPI(moleculeContainer, domain, msgType, removeRecvDuplicates,
														this, doHaloPositionCheck);

	global_log->set_mpi_output_root(0);
}



void DomainDecompMPIBase::exchangeForces(ParticleContainer* moleculeContainer, Domain* domain) { 
	global_log->set_mpi_output_all();

	// Using molecule exchange method with the force message type
	_neighbourCommunicationScheme->exchangeMoleculesMPI(moleculeContainer, domain, FORCES, false, this);

	global_log->set_mpi_output_root(0);
}

size_t DomainDecompMPIBase::getTotalSize() { 
	return DomainDecompBase::getTotalSize() + _neighbourCommunicationScheme->getDynamicSize()
			+ _collCommunication->getTotalSize();
}

void DomainDecompMPIBase::printSubInfo(int offset) { 
	std::stringstream offsetstream;
	for (int i = 0; i < offset; i++) {
		offsetstream << "\t";
	}
	global_log->info() << offsetstream.str() << "own datastructures:\t" << sizeof(DomainDecompMPIBase) / 1.e6 << " MB" << std::endl;
	global_log->info() << offsetstream.str() << "neighbourCommunicationScheme:\t\t" << _neighbourCommunicationScheme->getDynamicSize() / 1.e6 << " MB" << std::endl;
	global_log->info() << offsetstream.str() << "collective Communication:\t\t" << _collCommunication->getTotalSize() / 1.e6 << " MB" << std::endl;

}

void DomainDecompMPIBase::printCommunicationPartners(std::string filename) const{
	_neighbourCommunicationScheme->printCommunicationPartners(filename);
}
