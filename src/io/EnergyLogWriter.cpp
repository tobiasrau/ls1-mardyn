#include "io/EnergyLogWriter.h"

#include <ostream>

#include "Domain.h"
#include "parallel/DomainDecompBase.h"
#include "particleContainer/ParticleContainer.h"
#include "utils/xmlfileUnits.h"

using namespace std;

void EnergyLogWriter::initOutput(ParticleContainer* particleContainer, DomainDecompBase* domainDecomp, Domain* domain) {
	global_log->info() << "Init global energy log." << endl;

#ifdef ENABLE_MPI
	int rank = domainDecomp->getRank();
	// int numprocs = domainDecomp->getNumProcs();
	if (rank!= 0)
		return;
#endif

	std::stringstream outputstream;
	outputstream.write(reinterpret_cast<const char*>(&_writeFrequency), 8);

	ofstream fileout(_outputFilename, std::ios::out | std::ios::binary);
	fileout << outputstream.str();
	fileout.close();
}

void EnergyLogWriter::readXML(XMLfileUnits& xmlconfig) {
	_writeFrequency = 1;
	xmlconfig.getNodeValue("writefrequency", _writeFrequency);
	global_log->info() << "Write frequency: " << _writeFrequency << endl;
	_outputFilename = "global_energy.log";
	xmlconfig.getNodeValue("outputfilename", _outputFilename);
	global_log->info() << "Output filename: " << _outputFilename << endl;
}

void EnergyLogWriter::doOutput(ParticleContainer* particleContainer, DomainDecompBase* domainDecomp, Domain* domain, unsigned long simstep, std::list<ChemicalPotential>* lmu, std::map<unsigned int, CavityEnsemble>* mcav) {

	if( 0 != (simstep % _writeFrequency) ) {
		return;
	}

	unsigned long nNumMolsGlobalEnergyLocal = 0ul;
	double UkinLocal = 0.;
	double UkinTransLocal = 0.;
	double UkinRotLocal = 0.;
	#if defined(_OPENMP)
	#pragma omp parallel reduction(+:nNumMolsGlobalEnergyLocal,UkinLocal,UkinTransLocal,UkinRotLocal)
	#endif
	{
		for (auto moleculeIter = particleContainer->iterator(); moleculeIter.hasNext(); moleculeIter.next()) {
			nNumMolsGlobalEnergyLocal++;
			UkinLocal += moleculeIter->U_kin();
			UkinTransLocal += moleculeIter->U_trans();
			UkinRotLocal += moleculeIter->U_rot();
		}
	}

	// calculate global values
	domainDecomp->collCommInit(4);
	domainDecomp->collCommAppendUnsLong(nNumMolsGlobalEnergyLocal);
	domainDecomp->collCommAppendDouble(UkinLocal);
	domainDecomp->collCommAppendDouble(UkinTransLocal);
	domainDecomp->collCommAppendDouble(UkinRotLocal);
	domainDecomp->collCommAllreduceSum();
	unsigned long nNumMolsGlobalEnergyGlobal = domainDecomp->collCommGetUnsLong();
	double UkinGlobal = domainDecomp->collCommGetDouble();
	double UkinTransGlobal = domainDecomp->collCommGetDouble();
	double UkinRotGlobal = domainDecomp->collCommGetDouble();
	domainDecomp->collCommFinalize();

#ifdef ENABLE_MPI
	int rank = domainDecomp->getRank();
	if (rank!= 0)
		return;
#endif

	const double globalUpot = domain->getGlobalUpot();
	const double globalT = domain->getGlobalCurrentTemperature();
	const double globalPressure = domain->getGlobalPressure() ;

	std::stringstream outputstream;
	outputstream.write(reinterpret_cast<const char*>(&nNumMolsGlobalEnergyGlobal), 8);
	outputstream.write(reinterpret_cast<const char*>(&globalUpot), 8);
	outputstream.write(reinterpret_cast<const char*>(&UkinGlobal), 8);
	outputstream.write(reinterpret_cast<const char*>(&UkinTransGlobal), 8);
	outputstream.write(reinterpret_cast<const char*>(&UkinRotGlobal), 8);
	outputstream.write(reinterpret_cast<const char*>(&globalT), 8);
	outputstream.write(reinterpret_cast<const char*>(&globalPressure), 8);

	ofstream fileout(_outputFilename, std::ios::app | std::ios::binary);
	fileout << outputstream.str();
	fileout.close();
}

void EnergyLogWriter::finishOutput(ParticleContainer* particleContainer, DomainDecompBase* domainDecomp, Domain* domain) {}
