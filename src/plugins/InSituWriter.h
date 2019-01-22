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

#ifdef ENABLE_ADIOS2
#include "adios2.h"
#endif

#include <memory>
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
    virtual void _createFnames(int const rank, int const size) = 0;

    //shared by all writers
    static std::unique_ptr<FileWriterInterface> create(std::string writer);
    std::string _writeBuffer(void);
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
    void _createFnames(int const rank, int const size) override;
private:
    std::vector<char> _generateMmpldSeekTable(std::vector< std::vector<char> >& dataLists);
    std::vector<char> _buildMmpldDataList(ParticleContainer* particleContainer);
    // serialize all
    void _addMmpldHeader(float const bbox[6]);
    void _addMmpldSeekTable(std::vector<char> seekTable);
    void _addMmpldFrame(std::vector< std::vector<char> > dataLists);
};

#ifdef ENABLE_ADIOS2
class AdiosWriter : public FileWriterInterface {
public:
    AdiosWriter();
    void _addParticleData(
        ParticleContainer* particleContainer,
        float const bbox[6],
        float const simTime) override;
    void _createFnames(int const rank, int const size) override;
private:
    std::unique_ptr<adios2::ADIOS> _adios;
};
#endif /* ENABLE_ADIOS2 */
}
#endif /* ENABLE_INSITU */
#endif /* SRC_PLUGINS_INSITUWRITER_H_ */
