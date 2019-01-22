/*
 * InSituWriter.h
 *
 *  Created on: 17 Jan 2019
 *      Author: Oliver Fernandes
 */

#include "InSituWriter.h"
#include "particleContainer/ParticleContainer.h"

using Log::global_log;

#ifdef ENABLE_INSITU
////file writer interface

std::unique_ptr<InSitu::FileWriterInterface> InSitu::FileWriterInterface::create(std::string writer) {
    if (writer.compare("mmpld") == 0) {
        return std::unique_ptr<InSitu::FileWriterInterface>(new InSitu::MmpldWriter());
    }
#ifdef ENABLE_ADIOS2
    if (writer.compare("adios") == 0) {
        return std::unique_ptr<InSitu::FileWriterInterface>(new InSitu::AdiosWriter());
    }
#endif
    // if we reach here, something went wrong
    std::string const errormsg(" ISM: Insitu plugin error, disabling plugin > Invalid writer: "+writer);
    throw std::invalid_argument(errormsg);
}

std::string InSitu::FileWriterInterface::_getNextFname(void) {
    static RingBuffer::iterator nextFname = _fnameRingBuffer.begin();
    auto temp = nextFname;
    if (++nextFname == _fnameRingBuffer.end()) {
        nextFname = _fnameRingBuffer.begin();
    }
    return *temp;
}

void InSitu::FileWriterInterface::_resetBuffer(void) {
    _buffer.clear();
}

std::string InSitu::FileWriterInterface::_writeBuffer(void) {
    std::ofstream filestream;
    // std::stringstream fname;
    std::string fname(_getNextFname());
    filestream.open(fname, std::ios::binary | std::ios::trunc);
    filestream.write(_buffer.data(), _buffer.size());
    filestream.close();
    global_log->info() << "    ISM: Shared memory file written." << std::endl;
    return fname;
}

////mmpld implementation of writer
void InSitu::MmpldWriter::_createFnames(int const rank, int const size) {
    std::stringstream fname;
    for (size_t i=0; i<size; ++i) {
        fname << "/dev/shm/part_rnk" 
                << std::setfill('0') << std::setw(6) << rank 
                << "_buf" << std::setfill('0') << std::setw(2) << i << ".mmpld";
        // fname << "/home1/05799/fernanor/setups/insitu/part_rnk" 
        //         << std::setfill('0') << std::setw(6) << rank 
        //         << "_buf" << std::setfill('0') << std::setw(2) << i << ".mmpld";
        _fnameRingBuffer.push_back(fname.str());
        fname.str("");
    }
}

void InSitu::MmpldWriter::_addParticleData(
            ParticleContainer* particleContainer,
            float const bbox[6],
            float const simTime) {
    //build data lists
    std::vector< std::vector<char> > particleLists;
    //generate the mmpld-style particle lists from simulation data structure
    particleLists.push_back(_buildMmpldDataList(particleContainer));
    //generate seekTable from the lists built above
    std::vector<char> seekTable = _generateMmpldSeekTable(particleLists);

    //dump everything into the _buffer
    _resetBuffer();
    _addMmpldHeader(bbox);
    _addMmpldSeekTable(seekTable);
    mardyn_assert(*reinterpret_cast<size_t*>(seekTable.data()) == _buffer.size());
    _addMmpldFrame(particleLists);
    mardyn_assert(*reinterpret_cast<size_t*>(seekTable.data()+sizeof(size_t)) == _buffer.size());
}

std::vector<char> InSitu::MmpldWriter::_generateMmpldSeekTable(std::vector< std::vector<char> >& particleLists) {
    size_t const headerSize = 60;
    // seek table contains exactly 2 uint64_t, the start of the frame and the end
    size_t frameStart = 60+2*sizeof(size_t);
    // each frame start with an uint32_t, containing the number of particle lists
    // hence, add 4 bytes for that
    size_t frameEnd = frameStart+sizeof(unsigned int);
    // sum the individual particle list counts up
    for (auto const& list : particleLists) {
        frameEnd += list.size();
    }
    std::vector<char> seekTable;
    std::copy(reinterpret_cast<char*>(&frameStart),
              reinterpret_cast<char*>(&frameStart)+sizeof(size_t),
              std::back_inserter(seekTable));
    std::copy(reinterpret_cast<char*>(&frameEnd),
              reinterpret_cast<char*>(&frameEnd)+sizeof(size_t),
              std::back_inserter(seekTable));
    mardyn_assert(2*sizeof(size_t) == seekTable.size());
    return seekTable;
}

void InSitu::MmpldWriter::_addMmpldHeader(float const bbox[6]) {
    /// add the standard header data here
    std::string magicId("MMPLD");
    std::copy(magicId.begin(), magicId.end(), std::back_inserter(_buffer));
    _buffer.push_back(0);
    mardyn_assert(_buffer.size() == 6);
    mardyn_assert(sizeof(unsigned short) == 2);
    unsigned short version = 1 * 100 + 3;
    std::copy(reinterpret_cast<char*>(&version),
              reinterpret_cast<char*>(&version)+sizeof(unsigned short),
              std::back_inserter(_buffer));
    mardyn_assert(_buffer.size() == 8);
    mardyn_assert(sizeof(unsigned int) == 4);
    unsigned int numberOfTimesteps = 1;
    std::copy(reinterpret_cast<char*>(&numberOfTimesteps),
              reinterpret_cast<char*>(&numberOfTimesteps)+sizeof(unsigned int),
              std::back_inserter(_buffer));
    mardyn_assert(_buffer.size() == 12);
    std::copy(reinterpret_cast<char const*>(bbox),
              reinterpret_cast<char const*>(bbox)+6*sizeof(float),
              std::back_inserter(_buffer));
    mardyn_assert(_buffer.size() == 36);
    //add another copy of bbox as fake clipping box
    std::copy(reinterpret_cast<char const*>(bbox),
              reinterpret_cast<char const*>(bbox)+6*sizeof(float),
              std::back_inserter(_buffer));
    mardyn_assert(_buffer.size() == 60);
}

void InSitu::MmpldWriter::_addMmpldSeekTable(std::vector<char> seekTable) {
    std::copy(seekTable.begin(), seekTable.end(), std::back_inserter(_buffer));
}

void InSitu::MmpldWriter::_addMmpldFrame(std::vector< std::vector<char> > particleLists) {
    unsigned int numLists = static_cast<unsigned int>(particleLists.size());
    std::copy(reinterpret_cast<char*>(&numLists),
              reinterpret_cast<char*>(&numLists)+sizeof(float),
              std::back_inserter(_buffer));
    for (auto const& list : particleLists) {
        std::copy(list.begin(), list.end(), std::back_inserter(_buffer));
    }
}

std::vector<char> InSitu::MmpldWriter::_buildMmpldDataList(ParticleContainer* particleContainer) {
    // add list header
    std::vector<char> dataList;
    dataList.push_back(1); //vertex type
    dataList.push_back(0); //color type
    // insert global radius
    float radius = 1.0;
    std::copy(reinterpret_cast<char*>(&radius),
              reinterpret_cast<char*>(&radius)+sizeof(float),
              std::back_inserter(dataList));

    // dummies for Global RGB Color, insert 0.8*255 4 times for RGBA.
    dataList.insert(dataList.end(), 4, static_cast<unsigned char>(0.8*255.0));

    // number of particles
    size_t particleCount = static_cast<size_t>(particleContainer->getNumberOfParticles());
    mardyn_assert(sizeof(size_t) == 8);
    std::copy(reinterpret_cast<char*>(&particleCount),
              reinterpret_cast<char*>(&particleCount)+sizeof(size_t),
              std::back_inserter(dataList));
    // list bounding box OMG, this needs fixing i guess
    dataList.insert(dataList.end(), 6*sizeof(float), 0);

    // add vertex data
    for (auto mol = particleContainer->iterator(); mol.isValid(); ++mol) {
        float pos[3] {
            static_cast<float>(mol->r(0)),
            static_cast<float>(mol->r(1)),
            static_cast<float>(mol->r(2))
        };
        std::copy(reinterpret_cast<char*>(&pos),
                  reinterpret_cast<char*>(&pos)+3*sizeof(float),
                  std::back_inserter(dataList));
    }
    return dataList;
}

////adios implementation of writer
#ifdef ENABLE_ADIOS2
InSitu::AdiosWriter::AdiosWriter() {
    _adios = std::unique_ptr<adios2::ADIOS>(new adios2::ADIOS());
}

void InSitu::AdiosWriter::_addParticleData(
        ParticleContainer* particleContainer,
        float const bbox[6],
        float const simTime) {
    // get input settings, here: use default settings
    adios2::IO io = _adios->DeclareIO("Output");
    adios2::Variable<float> varGlobalArray =
        io.DefineVariable<float>("Position", {particleContainer->getNumberOfParticles()*3});
    adios2::Engine engine = io.Open(_getNextFname(), adios2::Mode::Write);

    // add vertex data
    std::vector<float> dataList;
    for (auto mol = particleContainer->iterator(); mol.isValid(); ++mol) {
        dataList.push_back(static_cast<float>(mol->r(0)));
        dataList.push_back(static_cast<float>(mol->r(1)));
        dataList.push_back(static_cast<float>(mol->r(2)));
    }

    engine.BeginStep();
    varGlobalArray.SetSelection(adios2::Box<adios2::Dims>({0},{particleContainer->getNumberOfParticles()*3}));
    engine.Put<float>(varGlobalArray, dataList.data());
    engine.EndStep();
}

void InSitu::AdiosWriter::_createFnames(int const rank, int const size) {
    std::stringstream fname;
    for (size_t i=0; i<size; ++i) {
        // fname << "/dev/shm/part_rnk" 
        //         << std::setfill('0') << std::setw(6) << rank 
        //         << "_buf" << std::setfill('0') << std::setw(2) << i << ".adios";
        fname << "/home1/05799/fernanor/setups/insitu/part_rnk" 
                << std::setfill('0') << std::setw(6) << rank 
                << "_buf" << std::setfill('0') << std::setw(2) << i << ".adios";
        _fnameRingBuffer.push_back(fname.str());
        fname.str("");
    }
}
#endif

#endif //ENABLE_INSITU
