/*
 * InSituMegamol.h
 *
 *  Created on: 11 Jul 2018
 *      Author: tchipevn / Oliver Fernandes
 */

///
/// \file InSituMegamol.h
/// Insitu Megamol Plugin Header. See the InSituMegamol class description for a manual on how to use the plugin
///

#ifndef SRC_PLUGINS_INSITUWRITER_H_
#define SRC_PLUGINS_INSITUWRITER_H_

#include "PluginBase.h"
#include "molecules/MoleculeForwardDeclaration.h"

#include <fstream>
#include <vector>

#ifdef ENABLE_INSITU
namespace InSitu {
typedef std::vector<std::string> RingBuffer;

class FileWriterInterface {
public:
    //these need to be implemented
    virtual void _addParticleData(
        ParticleContainer* particleContainer,
        float const bbox[6],
        float const simTime) = 0;
    virtual std::string _writeBuffer(void) = 0;
    virtual void _createFnames(int const rank, int const size) = 0;
protected:
    std::vector<char> _buffer;
    void _resetBuffer(void);
    std::string _getNextFname(void);
    RingBuffer _fnameRingBuffer;
};

class MmpldWriter : public FileWriterInterface {
public:
    void _addParticleData(
        ParticleContainer* particleContainer,
        float const bbox[6],
        float const simTime) override;
    std::string _writeBuffer(void) override;
    void _createFnames(int const rank, int const size) override;
private:
    std::vector<char> _generateMmpldSeekTable(std::vector< std::vector<char> >& dataLists);
    std::vector<char> _buildMmpldDataList(ParticleContainer* particleContainer);
    // serialize all
    void _addMmpldHeader(float const bbox[6]);
    void _addMmpldSeekTable(std::vector<char> seekTable);
    void _addMmpldFrame(std::vector< std::vector<char> > dataLists);
    std::string _writeMmpldBuffer(int rank, unsigned long simstep);
};
}
#endif // ENABLE_INSITU
#endif /* SRC_PLUGINS_INSITUWRITER_H_ */
