/*
 * AdiosWriter.cpp
 *
 *  Created on: April 2019
 *      Author: Tobias Rau
 */

#include "AdiosWriter.h"
#include "particleContainer/ParticleContainer.h"
#include "Domain.h"
#include "Simulation.h"
#include "parallel/DomainDecompBase.h"
#include "utils/mardyn_assert.h"

using Log::global_log;

namespace Adios {
void AdiosWriter::init(ParticleContainer* particleContainer,
        DomainDecompBase* domainDecomp, Domain* domain) {

        // set variable name to write
        vars["x"] = std::vector<double>();
        vars["y"] = std::vector<double>();
        vars["z"] = std::vector<double>();

};

void AdiosWriter::readXML(XMLfileUnits& xmlconfig) {


  fname = "test.bp";
  _writefrequency = 50000;
  xmlconfig.getNodeValue("outputfile", fname);
  xmlconfig.getNodeValue("writefrequency", _writefrequency);
  global_log->info() << "    AW: readXML." << std::endl;

  if (!inst) initAdios();

};

void AdiosWriter::initAdios() {
  auto& domainDecomp = _simulation.domainDecomposition();

  try {
    //get adios instance
        inst = std::make_shared<adios2::ADIOS>(MPI_COMM_WORLD);

        // declare io as output
        io = std::make_shared<adios2::IO>(inst->DeclareIO("Output"));

        // use bp engine
        io->SetEngine("BPFile");

        if (!engine) {
            global_log->info() << "    AW: Opening File for writing." << fname.c_str() << std::endl;
            engine = std::make_shared<adios2::Engine>(io->Open(fname, adios2::Mode::Write));
        }
  }
    catch (std::invalid_argument& e) {
        global_log->fatal() 
                << "Invalid argument exception, STOPPING PROGRAM from rank" 
                << domainDecomp.getRank() 
                << ": " << e.what() << std::endl;
	mardyn_exit(1);
    } catch (std::ios_base::failure& e) {
        global_log->fatal() 
                << "IO System base failure exception, STOPPING PROGRAM from rank " 
                << domainDecomp.getRank() 
                << ": " << e.what() << std::endl;
	mardyn_exit(1);
    } catch (std::exception& e) {
        global_log->fatal() 
                << "Exception, STOPPING PROGRAM from rank" 
                << domainDecomp.getRank() 
                << ": " << e.what() << std::endl;
	mardyn_exit(1);
    }
    global_log->info() << "    AW: Init complete." << std::endl;
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

  if (simstep % _writefrequency != 0) return;
  
    // for all outputs
    size_t localNumParticles = particleContainer->getNumberOfParticles();
    size_t const globalNumParticles = domain->getglobalNumMolecules();
    size_t const numProcs = domainDecomp->getNumProcs();

  std::vector<uint64_t> m_id;
  m_id.reserve(localNumParticles);

  std::vector<uint32_t> comp_id;
  comp_id.reserve(localNumParticles);

    for (auto m = particleContainer->iterator(ParticleIterator::ONLY_INNER_AND_BOUNDARY); m.isValid(); ++m) {
		vars["x"].emplace_back(m->r(0));
		vars["y"].emplace_back(m->r(1));
		vars["z"].emplace_back(m->r(2));
        m_id.emplace_back(m->getID());
        comp_id.emplace_back(m->componentid());
	}
  
    global_log->set_mpi_output_all();



    // gather offsets
    global_log->info() << "numProcs: " << numProcs 
            << " localNumParticles: " << localNumParticles
            << " domainDecomp->getRank(): " << domainDecomp->getRank() << std::endl;

    std::vector<int> offsets(numProcs);
    MPI_Allgather(&localNumParticles, 1, MPI_INT,
               offsets.data(), numProcs, MPI_INT,
               MPI_COMM_WORLD);
    
    auto offset = getOffset(offsets, domainDecomp->getRank());
    global_log->info() << "offset: " << offset << std::endl;
    #ifdef DEBUG
    global_log->set_mpi_output_root();
    #endif


    std::array<double,6> global_box{0, 0,0, domain->getGlobalLength(0), domain->getGlobalLength(1), domain->getGlobalLength(2)};
    std::array<double,6> local_box;
    domainDecomp->getBoundingBoxMinMax(domain, &local_box[0], &local_box[3]);
    
    global_log->info() << "Local Box: " << local_box[0] << " "<< local_box[1]<< " " << local_box[2] << " "<< local_box[3] << " "<< local_box[4]<< " " << local_box[5] <<  std::endl;

    engine->BeginStep();
    io->RemoveAllVariables();
    for (auto var : vars) {
        global_log->info() << "    AW: Defining Variables" << std::endl;
        adios2::Variable<double> adiosVar =
            io->DefineVariable<double>(var.first, {globalNumParticles},
                    {offset},
                    {localNumParticles}, adios2::ConstantDims);

        global_log->info() << "    AW: Putting Variables" << std::endl;
        if (!adiosVar) {
            global_log->error() << "    AW: Could not create variable: " << var.first << std::endl;
            return;
        }
	// ready for write; transfer data to adios
        engine->Put<double>(adiosVar, var.second.data());
    }

    // global box
    if(domainDecomp->getRank() == 0) {
    adios2::Variable<double> adios_global_box =
        io->DefineVariable<double>("global_box", {6}, {0}, {6}, adios2::ConstantDims);

    global_log->info() << "    AW: Putting Variables" << std::endl;
    if (!adios_global_box) {
        global_log->error() << "    AW: Could not create variable: global_box" << std::endl;
        return;
    }
    engine->Put<double>(adios_global_box, global_box.data());
    }

    //local box
    adios2::Variable<double> adios_local_box =
	//   io->DefineVariable<double>("local_box", {domainDecomp->getNumProcs(),6},
    //                 {domainDecomp->getRank(),6},
    //                 {1,6}, adios2::ConstantDims);
    // io->DefineVariable<double>("local_box", {domainDecomp->getNumProcs()*6},
    //                  {domainDecomp->getRank() * 6},
    //                  {6}, adios2::ConstantDims);
	  io->DefineVariable<double>("local_box", {},
                    {},
                    {6});

      if (!adios_local_box) {
            global_log->error() << "    AW: Could not create variable: local_box" << std::endl;
            return;
      }
      engine->Put<double>(adios_local_box, local_box.data());
    
    //molecule ids
    adios2::Variable<uint64_t> adios_molecule_ids =
	  io->DefineVariable<uint64_t>("molecule_ids", {globalNumParticles},
                    {offset},
                    {localNumParticles}, adios2::ConstantDims);
      if (!adios_molecule_ids) {
            global_log->error() << "    AW: Could not create variable: molecule_ids" << std::endl;
            return;
      }             
    engine->Put<uint64_t>(adios_molecule_ids, m_id.data());


    //component ids
    adios2::Variable<uint32_t> adios_component_ids =
	  io->DefineVariable<uint32_t>("component_ids", {globalNumParticles},
                    {offset},
                    {localNumParticles}, adios2::ConstantDims);
      if (!adios_component_ids) {
            global_log->error() << "    AW: Could not create variable: component_ids" << std::endl;
            return;
      }             
      engine->Put<uint32_t>(adios_component_ids, comp_id.data());


    //simulation time
    current_time = _simulation.getSimulationTime();
    if(domainDecomp->getRank() == 0) {
        adios2::Variable<double> adios_simulationtime =
            io->DefineVariable<double>("simulationtime");
        if (!adios_simulationtime) {
            global_log->error() << "    AW: Could not create variable: simulationtime" << std::endl;
            return;
        }             
        engine->Put<double>(adios_simulationtime, current_time);
    }

    /* Add cell generation
    //cell ids
    adios2::Variable<uint64_t> adios_cell_ids =
	                io->DefineVariable<uint64_t>("cell_ids", {},
                    {},
                    {});
    if (!adios_cell_ids) {
        global_log->info() << "    AW: Could not create variable: adios_cell_ids" << std::endl;
        return;
    }
    engine->Put<uint64_t>(adios_cell_ids, local_box.data());

     //cell offsets
    adios2::Variable<uint64_t> adios_cell_offsets =
	  io->DefineVariable<uint64_t>("cell_offsets", {},
                    {},
                    {});
      if (!adios_cell_offsets) {
            global_log->info() << "    AW: Could not create variable: adios_cell_offsets" << std::endl;
            return;
      }             
      engine->Put<uint64_t>(adios_cell_offsets, local_box.data());
    */
    
    // wait for completion of write
    engine->EndStep();
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
    engine->Close();
    global_log->info() << "    AW: finish." << std::endl;
}

}
