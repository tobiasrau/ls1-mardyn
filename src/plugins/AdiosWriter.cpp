/*
 * AdiosWriter.cpp
 *
 *  Created on: April 2019
 *      Author: Tobias Rau
 */

#include "AdiosWriter.h"
#include "particleContainer/ParticleContainer.h"
#include "Domain.h"
#include "parallel/DomainDecompBase.h"

using Log::global_log;

namespace Adios {
void AdiosWriter::init(ParticleContainer* particleContainer,
        DomainDecompBase* domainDecomp, Domain* domain) {
    try {
        //get adios instance
        inst = adios2::ADIOS(MPI_COMM_WORLD);

        // declare io as output
        io = std::make_shared<adios2::IO>(inst.DeclareIO("Output"));

        // use bp engine
        io->SetEngine("BPFile");

        if (!engine) {
            global_log->info() << "    AW: Opening File for writing." << fname.c_str() << std::endl;
            engine = io->Open(fname, adios2::Mode::Write);
        }
        // set variable name to write
        vars["x"] = std::vector<double>();
        vars["y"] = std::vector<double>();
        vars["z"] = std::vector<double>();
    }
    catch (std::invalid_argument& e) {
        global_log->info() 
                << "Invalid argument exception, STOPPING PROGRAM from rank" 
                << domainDecomp->getRank() 
                << ": " << e.what() << std::endl;
    } catch (std::ios_base::failure& e) {
        global_log->info() 
                << "IO System base failure exception, STOPPING PROGRAM from rank " 
                << domainDecomp->getRank() 
                << ": " << e.what() << std::endl;
    } catch (std::exception& e) {
        global_log->info() 
                << "Exception, STOPPING PROGRAM from rank" 
                << domainDecomp->getRank() 
                << ": " << e.what() << std::endl;
    }
    global_log->info() << "    AW: Init complete." << std::endl;
};

void AdiosWriter::readXML(XMLfileUnits& xmlconfig) {
    fname.assign("test.bp");
    global_log->info() << "    AW: readXML." << std::endl;
};

void AdiosWriter::beforeEventNewTimestep(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        unsigned long simstep) {
    global_log->info() << "    AW: beforeEventNewTimestep." << std::endl;

};

void AdiosWriter::beforeForces(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        unsigned long simstep
) {
    global_log->info() << "    AW: beforeForces." << std::endl;

};

void AdiosWriter::afterForces(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        unsigned long simstep
) {
    global_log->info() << "    AW: afterForces." << std::endl;

};

void AdiosWriter::endStep(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        Domain* domain, unsigned long simstep) {
	for (auto m = particleContainer->iterator(); m.isValid(); ++m) {
		vars["x"].push_back(m->r(0));
		vars["y"].push_back(m->r(1));
		vars["z"].push_back(m->r(2));
	}
    global_log->set_mpi_output_all();

    // for all outputs
    size_t localNumParticles = particleContainer->getNumberOfParticles();
    size_t const globalNumParticles = domain->getglobalNumMolecules();
    size_t const numProcs = domainDecomp->getNumProcs();

    // gather offsets
    global_log->info() << "numProcs: " << numProcs 
            << " localNumParticles: " << localNumParticles
            << " domainDecomp->getRank(): " << domainDecomp->getRank() << std::endl;
    std::vector<int> offsets(numProcs);
    int error = MPI_Allgather(&localNumParticles, 1, MPI_INT,
               offsets.data(), numProcs, MPI_INT,
               MPI_COMM_WORLD);
    mardyn_assert(MPI_SUCCESS == error);
    auto offset = getOffset(offsets, domainDecomp->getRank());
    global_log->info() << "offset: " << offset << std::endl;
    global_log->set_mpi_output_root();

    engine.BeginStep();
    io->RemoveAllVariables();
    for (auto var : vars) {
        global_log->info() << "    AW: Defining Variables" << std::endl;
        adios2::Variable<double> adiosVar =
            io->DefineVariable<double>(var.first, {globalNumParticles},
                    {static_cast<size_t>(offset)},
                    {static_cast<size_t>(localNumParticles)}, 
                    false);

        global_log->info() << "    AW: Putting Variables" << std::endl;
        if (!adiosVar) {
            global_log->info() << "    AW: Could not create variable: " << var.first << std::endl;
            return;
        }
        engine.Put<double>(adiosVar, var.second.data());
    }

    engine.EndStep();
    global_log->info() << "    AW: endStep." << std::endl;
};

int AdiosWriter::getOffset(std::vector<int>& offsets, int const rank) {
    int sum = 0;
    for (auto i=0; i<rank; ++i) {
        sum += offsets[i];
    }
    return sum;
}

void AdiosWriter::finish(ParticleContainer* particleContainer,
        DomainDecompBase* domainDecomp, Domain* domain
) {
    engine.Close();
    global_log->info() << "    AW: finish." << std::endl;
}

}