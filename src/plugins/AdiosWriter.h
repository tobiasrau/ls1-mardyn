/*
 * AdiosWriter.h
 *
 *  Created on: 11 Jul 2018
 *      Author: Oliver Fernandes
 */

///
/// \file AdiosWriter.h
/// Insitu Megamol Plugin Header. See the AdiosWriter class description for a manual on how to use the plugin
///

#ifndef SRC_PLUGINS_AdiosWriter_H_
#define SRC_PLUGINS_AdiosWriter_H_

#include "PluginBase.h"
#include "molecules/MoleculeForwardDeclaration.h"

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

namespace Adios {

class AdiosWriter : public PluginBase {
public:
    AdiosWriter() {}; 
    virtual ~AdiosWriter() {};

    void init(ParticleContainer* particleContainer,
            DomainDecompBase* domainDecomp, Domain* domain
    );

    void readXML(XMLfileUnits& xmlconfig);

    void beforeEventNewTimestep(
            ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
            unsigned long simstep
    );

    void beforeForces(
            ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
            unsigned long simstep
    );

    void afterForces(
            ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
            unsigned long simstep
    );

    void endStep(
            ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
            Domain* domain, unsigned long simstep
    );
    
    void finish(ParticleContainer* particleContainer,
            DomainDecompBase* domainDecomp, Domain* domain);

    std::string getPluginName() {
        return std::string("AdiosWriter");
    }

    static PluginBase* createInstance() { return new AdiosWriter(); }

protected:
    // 
private:
    int getOffset(std::vector<int>&, int const);
    // output filename, from XML
    std::string fname;
    // variables to write, see documentation
    std::map<std::string, std::vector<double> > vars;
    // main instance
    adios2::ADIOS inst;
    adios2::Engine engine;  
    std::shared_ptr<adios2::IO> io;
};
}
#endif // ENABLE_ADIOS
#endif /* SRC_PLUGINS_REDUNDANCYRESILIENCE_H_ */
