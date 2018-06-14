#ifndef SRC_IO_HALOPARTICLEWRITER_H_
#define SRC_IO_HALOPARTICLEWRITER_H_

#include <string>

#include "plugins/PluginBase.h"


class HaloParticleWriter : public PluginBase {
public:
	
    HaloParticleWriter() {}
	~HaloParticleWriter() {}
	

	/** @brief Read in XML configuration for HaloParticleWriter.
	 *
	 * The following xml object structure is handled by this method:
	 * \code{.xml}
	   <outputplugin name="HaloParticleWriter">
	     <writefrequency>INTEGER</writefrequency>
	     <outputprefix>STRING</outputprefix>
	     <incremental>INTEGER</incremental>
	     <appendTimestamp>INTEGER</appendTimestamp>
	   </outputplugin>
	   \endcode
	 */
	void readXML(XMLfileUnits& xmlconfig);
	
	void init(ParticleContainer *particleContainer,
              DomainDecompBase *domainDecomp, Domain *domain);
	void afterForces(ParticleContainer *particleContainer,
			DomainDecompBase *domainDecomp, unsigned long simstep) override;

	void endStep(ParticleContainer* particleContainer,
			DomainDecompBase* domainDecomp, Domain* domain,
			unsigned long simstep) override {
	}

	void finish(ParticleContainer *particleContainer,
				DomainDecompBase *domainDecomp, Domain *domain);
	
	std::string getPluginName() {
		return std::string("HaloParticleWriter");
	}
	static PluginBase* createInstance() { return new HaloParticleWriter(); }
private:
	std::string _outputPrefix;
	unsigned long _writeFrequency;
	bool	_incremental;
	bool	_appendTimestamp;
};

#endif  // SRC_IO_HALOPARTICLEWRITER_H_
