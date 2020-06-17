/*
 * AdiosReader.cpp
 *
 *  Created on: April 2019
 *      Author: Tobias Rau
 */

#ifdef ENABLE_ADIOS

#include "AdiosReader.h"
#include "particleContainer/ParticleContainer.h"
#include "Domain.h"
#include "Simulation.h"
#include "utils/mardyn_assert.h"
#ifdef ENABLE_MPI
#include "parallel/ParticleData.h"
#include "parallel/DomainDecompBase.h"
#endif

using Log::global_log;

void AdiosReader::init(ParticleContainer* particleContainer,
        DomainDecompBase* domainDecomp, Domain* domain) {
};

void AdiosReader::readXML(XMLfileUnits& xmlconfig) {
  mode = "rootOnly";
  fname = "test.bp";
  xmlconfig.getNodeValue("filename", fname);
  xmlconfig.getNodeValue("mode", mode);
  // Adios step (frame)
  xmlconfig.getNodeValue("adiosStep", step);

  global_log->info() << "    [AdiosReader]: readXML." << std::endl;

  if (!inst) initAdios();
};

void AdiosReader::readPhaseSpaceHeader(Domain* domain, double timestep) {
//EMPTY
};


unsigned long AdiosReader::readPhaseSpace(ParticleContainer* particleContainer, Domain* domain, DomainDecompBase* domainDecomp) {

  auto variables = io->AvailableVariables();

  auto total_steps = std::stoi(variables["simulationtime"]["AvailableStepsCount"]);
  global_log->info() << "[AdiosReader]: TOTAL STEPS " << total_steps << std::endl;


  if (step > total_steps) {
      global_log->error() << "[AdiosReader]: Specified step is out of scope" << std::endl;
  } 
  if (step < 0) {
      step = total_steps + (step + 1);
  }
  particle_count = std::stoi(variables["x"]["Shape"]);
  global_log->info() << "    [AdiosReader]: Particle count: " << particle_count << std::endl;

  if (mode == "rootOnly") {
      rootOnlyRead(particleContainer, domain, domainDecomp);
  } else if (mode == "equalRanks") {
      // TODO: Implement
      equalRanksRead(particleContainer, domain, domainDecomp);
  } else {
      global_log->error() << "[AdiosReader]: Unkown Mode '" << mode << "'" << std::endl;
  }

    engine->Close();
    global_log->info() << "    [AdiosReader]: finish." << std::endl;

    return particle_count;
};

void AdiosReader::rootOnlyRead(ParticleContainer* particleContainer, Domain* domain, DomainDecompBase* domainDecomp) {

	Timer inputTimer;
	inputTimer.start();

    std::vector<double> x ,y, z, vx, vy, vz, q0, q1, q2, q3, Dx, Dy, Dz;
    std::vector<uint32_t> comp_id;
    std::vector<uint64_t> mol_id;
    double simtime;
    uint64_t buffer = 1000;
    MPI_Datatype mpi_Particle;
	ParticleData::getMPIType(mpi_Particle);


    auto num_reads = particle_count / buffer;
    if (particle_count % buffer != 0) num_reads += 1;

    auto variables = io->AvailableVariables();

    for (int read = 0; read < num_reads; read++) {
        uint64_t offset = read * buffer;
        if (read == num_reads -1) buffer = particle_count % buffer;
        for (auto var : variables) {
            if(domainDecomp->getRank() == 0) {
                if (var.first == "x") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, x, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Positions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "y") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, y, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Positions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "z") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, z, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Positions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "vx") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, vx, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Velocities should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "vy") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, vy, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Velocities should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "vz") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, vz, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Velocities should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "q0") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, q0, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Quaternions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "q1") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, q1, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Quaternions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "q2") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, q2, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Quaternions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "q3") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, q3, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Quaternions should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "Dx") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, Dx, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Angular momentum should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "Dy") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, Dy, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Angular momentum should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "Dz") {
                    if (var.second["Type"] == "double") {
                        doTheRead(var.first, Dz, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Angular momentum should be doubles (for now)." << std::endl;
                    }
                }
                if (var.first == "component_ids") {
                    if (var.second["Type"] == "uint32_t") {
                        doTheRead(var.first, comp_id, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Component ids should be uint32_t (for now)." << std::endl;
                    }
                }
                if (var.first == "molecule_ids") {
                    if (var.second["Type"] == "uint64_t") {
                        doTheRead(var.first, mol_id, buffer, offset);
                    } else {
                        global_log->error() << "    [AdiosReader] Molecule ids should be uint64_t (for now)." << std::endl;
                    }
                }
            }
            if (var.first == "simulationtime") {
                if (var.second["Type"] == "double") {
                    doTheRead(var.first, simtime);
                } else {
                    global_log->error() << "    [AdiosReader] Simulation time should be double (for now)." << std::endl;
                }
            }
        }
        engine->PerformGets();

        std::vector<ParticleData> particle_buff(buffer);
        auto& dcomponents = *(_simulation.getEnsemble()->getComponents());
        if(domainDecomp->getRank() == 0) {
            for (int i = 0; i < buffer; i++) {
                    Molecule m1 = Molecule(mol_id[i], &dcomponents[comp_id[i]], x[i], y[i], z[i], vx[i],
                                            vy[i], vz[i], q0[i], q1[i], q2[i], q3[i], Dx[i], Dy[i], Dz[i]);
                    ParticleData::MoleculeToParticleData(particle_buff[i], m1);
            }
        }
        MPI_Bcast(particle_buff.data(), buffer, mpi_Particle, 0, MPI_COMM_WORLD);
        for (int j = 0; j < buffer; j++) {
            Molecule m;
            ParticleData::ParticleDataToMolecule(particle_buff[j], m);
            // only add particle if it is inside of the own domain!
            if(particleContainer->isInBoundingBox(m.r_arr().data())) {
                particleContainer->addParticle(m, true, false);
            }

            dcomponents[m.componentid()].incNumMolecules();
            domain->setglobalRotDOF(
            dcomponents[m.componentid()].getRotationalDegreesOfFreedom()
                    + domain->getglobalRotDOF());

            // Only called inside GrandCanonical
            // TODO
            //global_simulation->getEnsemble()->storeSample(&m, componentid);
        }
        // Print status message
		unsigned long iph = num_reads / 100;
		if(iph != 0 && (read % iph) == 0)
			global_log->info() << "Finished reading molecules: " << read / iph
							   << "%\r" << std::flush;
    }

    global_log->info() << "Finished reading molecules: 100%" << std::endl;
	global_log->info() << "Reading Molecules done" << std::endl;

    inputTimer.stop();
	global_log->info() << "Initial IO took:                 "
					   << inputTimer.get_etime() << " sec" << std::endl;
    
	if(domain->getglobalRho() == 0.) {
		domain->setglobalRho(
				domain->getglobalNumMolecules() / domain->getGlobalVolume());
		global_log->info() << "Calculated Rho_global = "
						   << domain->getglobalRho() << endl;
	}

    _simulation.setSimulationTime(simtime);
    global_log->info() << "    [AdiosReader] simulation time is: " << simtime << std::endl;

};

void AdiosReader::initAdios() {
  auto& domainDecomp = _simulation.domainDecomposition();

  try {
    //get adios instance
        inst = std::make_shared<adios2::ADIOS>(MPI_COMM_WORLD);

        // declare io as output
        io = std::make_shared<adios2::IO>(inst->DeclareIO("Input"));

        // use bp engine
        io->SetEngine("BPFile");

        if (!engine) {
            global_log->info() << "    [AdiosReader]: Opening File for writing." << fname.c_str() << std::endl;
            engine = std::make_shared<adios2::Engine>(io->Open(fname, adios2::Mode::Read));
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
    global_log->info() << "    [AdiosReader]: Init complete." << std::endl;
};

#endif // ENABLE_ADIOS