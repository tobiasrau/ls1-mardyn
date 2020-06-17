/*
 * AdiosReader.h
 *
 */

///
/// \file AdiosReader.h
/// Adios Reader
///

#pragma once

#include "io/InputBase.h"
#include "molecules/MoleculeForwardDeclaration.h"
#include "utils/Logger.h"

#include <chrono>
#include <set>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>
#include <map>
#include <errno.h>

#ifdef ENABLE_ADIOS

#include "adios2.h"
#include "mpi.h"

class AdiosReader : public InputBase {
public:
    AdiosReader() {}; 
    virtual ~AdiosReader() {};

    void init(ParticleContainer* particleContainer,
            DomainDecompBase* domainDecomp, Domain* domain
    );

    void readXML(XMLfileUnits& xmlconfig);

	void readPhaseSpaceHeader(Domain* domain, double timestep);

    std::string getPluginName() {
        return std::string("AdiosReader");
    }

   unsigned long readPhaseSpace(ParticleContainer* particleContainer, Domain* domain, DomainDecompBase* domainDecomp);


protected:
    // 
private:
    void initAdios();
    void rootOnlyRead(ParticleContainer* particleContainer, Domain* domain, DomainDecompBase* domainDecomp);
    void equalRanksRead(ParticleContainer* particleContainer, Domain* domain, DomainDecompBase* domainDecomp) {};
    // output filename, from XML
    std::string fname;
    std::string mode;
    int step;
    uint64_t particle_count;
    // main instance
    std::shared_ptr<adios2::ADIOS> inst;
    std::shared_ptr<adios2::Engine> engine;  
    std::shared_ptr<adios2::IO> io;

    template <typename T> 
    void doTheRead(std::string var_name, std::vector<T>& container) {
      uint64_t num = 1;
      auto advar = io->InquireVariable<T>(var_name);
      advar.SetStepSelection({step,1});
      auto info = engine->BlocksInfo(advar, 0);
      auto shape = info[0].Count;
      std::for_each(shape.begin(), shape.end(), [&](decltype(num) n) { num *= n; });
      container.resize(num);
      engine->Get<T>(advar, container);
    }

    template <typename T> 
    void doTheRead(std::string var_name, std::vector<T>& container, uint64_t buffer, uint64_t offset) {

      adios2::Dims readsize({buffer});
      adios2::Dims offs({offset});

      auto advar = io->InquireVariable<T>(var_name);
      advar.SetStepSelection({step,1});
      Log::global_log->info() << "[AdiosReader]: buffer " << buffer << " offset " << offset << std::endl;
      for (auto entry: advar.Shape())
        Log::global_log->info() << "[AdiosReader]: shape " << entry << std::endl;
      advar.SetSelection(adios2::Box<adios2::Dims>(offs, readsize));
      container.resize(buffer);
      engine->Get<T>(advar, container);
    }

    template <typename T> 
    void doTheRead(std::string var_name, T& value) {
      auto advar = io->InquireVariable<T>(var_name);
      advar.SetStepSelection({step,1});
      engine->Get<T>(advar, value);
    }

};
#endif // ENABLE_ADIOS
