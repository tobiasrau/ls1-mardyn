/*
 * DomainDecompBaseMPI.cpp
 *
 *  Created on: Nov 15, 2015
 *      Author: tchipevn
 */

#include "DomainDecompBaseMPI.h"
#include "molecules/Molecule.h"

using Log::global_log;

DomainDecompBaseMPI::DomainDecompBaseMPI() : _comm(MPI_COMM_WORLD) {
	// TODO Auto-generated constructor stub

	// Initialize MPI Dataype for the particle exchange once at the beginning.
	ParticleData::setMPIType(_mpiParticleType);
}

DomainDecompBaseMPI::~DomainDecompBaseMPI() {
	MPI_Type_free(&_mpiParticleType);

	// MPI_COMM_WORLD doesn't need to be freed
	// if a derived class does something with the communicator
	// then the derived class should also free it
}

unsigned DomainDecompBaseMPI::Ndistribution(unsigned localN, float* minrnd, float* maxrnd) {
	unsigned* moldistribution = new unsigned[_numProcs];
	MPI_CHECK( MPI_Allgather(&localN, 1, MPI_UNSIGNED, moldistribution, 1, MPI_UNSIGNED, _comm) );
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

void DomainDecompBaseMPI::assertIntIdentity(int IX) {
	if (_rank)
		MPI_CHECK( MPI_Send(&IX, 1, MPI_INT, 0, 2 * _rank + 17, _comm) );
	else {
		int recv;
		MPI_Status s;
		for (int i = 1; i < _numProcs; i++) {
			MPI_CHECK( MPI_Recv(&recv, 1, MPI_INT, i, 2 * i + 17, _comm, &s) );
			if (recv != IX) {
				global_log->error() << "IX is " << IX << " for rank 0, but " << recv << " for rank " << i << ".\n";
				MPI_Abort(MPI_COMM_WORLD, 911);
			}
		}
		global_log->info() << "IX = " << recv << " for all " << _numProcs << " ranks.\n";
	}
}

void DomainDecompBaseMPI::assertDisjunctivity(TMoleculeContainer* mm) {
	using std::map;
	using std::endl;

	Molecule* m;

	if (_rank) {
		int num_molecules = mm->getNumberOfParticles();
		unsigned long *tids;
		tids = new unsigned long[num_molecules];

		int i = 0;
		for (m = mm->begin(); m != mm->end(); m = mm->next()) {
			tids[i] = m->id();
			i++;
		}
		MPI_CHECK( MPI_Send(tids, num_molecules, MPI_UNSIGNED_LONG, 0, 2674 + _rank, _comm) );
		delete[] tids;
		global_log->info() << "Data consistency checked: for results see rank 0." << endl;
	}
	else {
		map<unsigned long, int> check;

		for (m = mm->begin(); m != mm->end(); m = mm->next())
			check[m->id()] = 0;

		MPI_Status status;
		for (int i = 1; i < _numProcs; i++) {
			int num_recv = 0;
			unsigned long *recv;
			MPI_CHECK( MPI_Probe(i, 2674 + i, _comm, &status) );
			MPI_CHECK( MPI_Get_count(&status, MPI_UNSIGNED_LONG, &num_recv) );
			recv = new unsigned long[num_recv];

			MPI_CHECK( MPI_Recv(recv, num_recv, MPI_UNSIGNED_LONG, i, 2674 + i, _comm, &status) );
			for (int j = 0; j < num_recv; j++) {
				if (check.find(recv[j]) != check.end()) {
					global_log->error() << "Ranks " << check[recv[j]] << " and " << i << " both propagate ID " << recv[j] << endl;
					MPI_Abort(MPI_COMM_WORLD, 1);
				}
				else
					check[recv[j]] = i;
			}
			delete[] recv;
		}
		global_log->info() << "Data consistency checked: No duplicate IDs detected among " << check.size() << " entries." << endl;
	}
}

void DomainDecompBaseMPI::exchangeMolecules(ParticleContainer* moleculeContainer, Domain* domain) {
	using std::vector;

	for (unsigned short d = 0; d < DIM; d++) {
		if (_coversWholeDomain[d]) {
			// use the sequential version
			DomainDecompBase::handleDomainLeavingParticles(d, moleculeContainer);
			DomainDecompBase::populateHaloLayerWithCopies(d, moleculeContainer);
			continue;
		}

		const int numNeighbours = _neighbours[d].size();

		for (int i = 0; i < numNeighbours; ++i) {
			_neighbours[d][i].initCommunication(d, moleculeContainer, _comm, _mpiParticleType);
		}

		// the following implements a non-blocking recv scheme, which overlaps unpacking of
		// messages with waiting for other messages to arrive
		bool allDone = false;

		while (not allDone) {
			allDone = true;

			// "kickstart" processing of all Isend requests
			for(int i = 0; i < numNeighbours; ++i) {
				allDone &= _neighbours[d][i].testSend();
			}

			// get the counts and issue the Irecv-s
			for(int i = 0; i < numNeighbours; ++i) {
				allDone &= _neighbours[d][i].iprobeCount(_comm, _mpiParticleType);
			}

			// unpack molecules
			for(int i = 0; i < numNeighbours; ++i) {
				allDone &= _neighbours[d][i].testRecv(moleculeContainer);
			}

			// TODO: we can catch deadlocks here due to dying MPI processes (or bugs)

		} // while not allDone
	} // for d
}

void DomainDecompBaseMPI::balanceAndExchange(bool forceRebalancing, ParticleContainer* moleculeContainer, Domain* domain) {
	rebalance(forceRebalancing, moleculeContainer, domain);
	exchangeMolecules(moleculeContainer, domain);
}

