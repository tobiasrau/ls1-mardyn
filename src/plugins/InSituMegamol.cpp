/*
 * InSituMegamol.h
 *
 *  Created on: 19 Jul 2018
 *      Author: Oliver Fernandes
 */

#include "InSituMegamol.h"
#include "utils/xmlfileUnits.h"
#include "utils/Logger.h"
#include "molecules/Molecule.h"
#include "particleContainer/ParticleContainer.h"
#include "Simulation.h"
#include "Domain.h"
#include "parallel/DomainDecompBase.h"

using Log::global_log;

#ifdef ENABLE_INSITU
#define BLOCK_POLICY_HANDSHAKE 0
#define BLOCK_POLICY_UPDATE ZMQ_DONTWAIT

////main plugin class methods
InSitu::InSituMegamol::InSituMegamol(void) 
        : _isEnabled(false) {
    constexpr bool uint64_tIsSize_t = std::is_same<uint64_t, size_t>::value;
    mardyn_assert(uint64_tIsSize_t);
}

void InSitu::InSituMegamol::init(ParticleContainer* particleContainer,
        DomainDecompBase* domainDecomp, Domain* domain) {
    _isEnabled = true;
    _zmqManager.setConnection(_connectionName);	
    _zmqManager.setReplyBufferSize(_replyBufferSize);
    _zmqManager.setSyncTimeout(_syncTimeout);
    _zmqManager.setModuleNames(domainDecomp->getRank());
    _isEnabled &= _zmqManager.performHandshake();
    try { 
        _fileWriter = InSitu::FileWriterInterface::create(_fileFormat);
    }
    catch (std::invalid_argument const ia) {
        //writer aquisition failed, disable the plugin
        global_log->warning() << ia.what() << std::endl;
        _isEnabled = false;
        return;
    }
    _fileWriter->_createFnames(domainDecomp->getRank(), _ringBufferSize);
}

void InSitu::InSituMegamol::readXML(XMLfileUnits& xmlconfig) {
    _snapshotInterval = 20;
    xmlconfig.getNodeValue("snapshotInterval", _snapshotInterval);
    global_log->info() << "    ISM: Snapshot interval: "
            << _snapshotInterval << std::endl;
    _connectionName.assign("tcp://127.0.0.1:33333");
    xmlconfig.getNodeValue("connectionName", _connectionName);
    global_log->info() << "    ISM: Ping to Megamol on: <" 
            << _connectionName << ">" << std::endl;
    _replyBufferSize = 16384;
    xmlconfig.getNodeValue("replyBufferSize", _replyBufferSize);
    global_log->info() << "    ISM: Megamol reply buffer size (defaults to 16384 byte): "
            << _replyBufferSize << std::endl;
    _syncTimeout = 10;
    xmlconfig.getNodeValue("syncTimeout", _syncTimeout);
    global_log->info() << "    ISM: Synchronization timeout (s): "
            << _syncTimeout << std::endl;
    _ringBufferSize = 5;
    xmlconfig.getNodeValue("ringBufferSize", _ringBufferSize);
    global_log->info() << "    ISM: Ring buffer size: "
            << _ringBufferSize << std::endl;
    _fileFormat = "mmpld";
    xmlconfig.getNodeValue("fileFormat", _fileFormat);
    global_log->info() << "    ISM: File format: "
            << _fileFormat << std::endl;
}

void InSitu::InSituMegamol::beforeEventNewTimestep(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        unsigned long simstep) {
}

void InSitu::InSituMegamol::beforeForces(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        unsigned long simstep) {
    _startForceCalculation = std::chrono::high_resolution_clock::now();
}

void InSitu::InSituMegamol::afterForces(
        ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
        unsigned long simstep) {
    _endForceCalculation = std::chrono::high_resolution_clock::now();
}

void InSitu::InSituMegamol::endStep(ParticleContainer* particleContainer,
        DomainDecompBase* domainDecomp, Domain* domain, unsigned long simstep) {
    if (!(simstep % _snapshotInterval)) {
        if (!_isEnabled) {
            global_log->info() << "    ISM: Disabled. Skipping InSitu plugin." << std::endl;
            return;
        }
        auto start = std::chrono::high_resolution_clock::now();
        //get bbox 
        float bbox[6] {
            0.0f, 0.0f, 0.0f,
            static_cast<float>(domain->getGlobalLength(0)),
            static_cast<float>(domain->getGlobalLength(1)),
            static_cast<float>(domain->getGlobalLength(2))
        };
        //convert simulation time
        float simTime = static_cast<float>(global_simulation->getSimulationTime());
        _fileWriter->_addParticleData(particleContainer, bbox, simTime);
        std::string fname = _fileWriter->_writeBuffer();

        //post update to MegaMol and log duration of copy op
        auto end = std::chrono::high_resolution_clock::now();
        global_log->info() << "    ISM: copy: " << std::chrono::duration<double>(end-start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();
        _zmqManager.triggerUpdate(fname);
        end = std::chrono::high_resolution_clock::now();
        global_log->info() << "    ISM: update: " << std::chrono::duration<double>(end-start).count() << std::endl;

        //dump local molecule count to log
        global_log->info() << "    ISM: Molecule count of local data: " << particleContainer->getNumberOfParticles() << std::endl;
    }
}

////nested zmq manager implementation
InSitu::InSituMegamol::ZmqManager::ZmqManager(void) 
        : _sendCount(0)
        , _context(zmq_ctx_new(), &zmq_ctx_destroy)
        , _requester(zmq_socket(_context.get(), ZMQ_REQ), &zmq_close) 
        , _pairsocket(zmq_socket(_context.get(), ZMQ_PAIR), &zmq_close) {
    mardyn_assert(_context.get());
    mardyn_assert(_requester.get());
    mardyn_assert(_pairsocket.get());
    int lingerTime=0;
    zmq_setsockopt(_requester.get(), ZMQ_LINGER, &lingerTime, sizeof(int));
    global_log->info() << "    ISM: Acquired ZmqManager resources." << std::endl;
    global_log->info() << "    ISM: Using ZeroMQ version: " << getZmqVersion() << std::endl;
}

InSitu::InSituMegamol::ZmqManager::~ZmqManager(void) {
    _pairsocket.reset();
    _requester.reset();
    _context.reset();
}

InSitu::InSituMegamol::ZmqManager::ZmqManager(ZmqManager const& rhs)
        : _context(zmq_ctx_new(), &zmq_ctx_destroy)
        , _requester(zmq_socket(_context.get(), ZMQ_REQ), &zmq_close) 
        , _pairsocket(zmq_socket(_context.get(), ZMQ_PAIR), &zmq_close) {
}

std::string InSitu::InSituMegamol::ZmqManager::getZmqVersion(void) const {
    std::stringstream version;
    version << ZMQ_VERSION_MAJOR << "."
            << ZMQ_VERSION_MINOR << "."
            << ZMQ_VERSION_PATCH;
    return version.str();
}

void InSitu::InSituMegamol::ZmqManager::setConnection(std::string connectionName) {
    if (zmq_connect(_requester.get(), connectionName.data())) {
        global_log->info() << "    ISM: Requester connection failed, releasing resources." << std::endl;
        // TODO: propagate plugin shutdown;
    }
    global_log->info() << "    ISM: Requester initialized." << std::endl;
}

int InSitu::InSituMegamol::ZmqManager::setPairConnection(std::string connectionName) {
    if (zmq_connect(_pairsocket.get(), connectionName.data())) {
        auto connectErrno = errno;
        global_log->info() << "    ISM: Pair connection failed, releasing resources." << std::endl;
        return connectErrno;
        // TODO: propagate plugin shutdown;
    }
    else {
        global_log->info() << "    ISM: Setup pair connection." << std::endl;
        return 0;
    }
}

void InSitu::InSituMegamol::ZmqManager::setModuleNames(int rank) {
    _datTag << "::dat" << std::setfill('0') << std::setw(6) << rank;
    _geoTag << "::geo" << std::setfill('0') << std::setw(6) << rank;
    _concatTag << "::concat" << std::setfill('0') << std::setw(6) << rank;
}

bool InSitu::InSituMegamol::ZmqManager::performHandshake(void) {
    // get pair port
    InSituMegamol::ISM_SYNC_STATUS ismStatus = ISM_SYNC_SYNCHRONIZING;
    global_log->info() << "    ISM: Connecting via requester..." << std::endl;
    _timeoutCheck(true);
    do {
        // try to sync to Megamol. Abort either on error or after _syncTimeout sec.
        ismStatus = _getMegamolPairPort();
        if (ismStatus == ISM_SYNC_TIMEOUT
                || ismStatus == ISM_SYNC_REPLY_BUFFER_OVERFLOW
                || ismStatus == ISM_SYNC_SEND_FAILED
                || ismStatus == ISM_SYNC_UNKNOWN_ERROR) {
            global_log->info() << "    ISM: Megamol sync failed." << std::endl;
            return false;
        }
    } while (ismStatus != ISM_SYNC_SUCCESS);

    // add the pair connection
    std::string reply;
    std::stringstream msg;
    mardyn_assert(_replyBuffer.size() == static_cast<size_t>(_replyBufferSize));
    std::string pairConnectionName("tcp://127.0.0.1:"+_pairPort);
    global_log->info() << "    ISM: Setting pair connection: " << pairConnectionName << std::endl;
    auto pairConnError = setPairConnection(pairConnectionName);
    if (pairConnError) {
        global_log->info() << "    ISM: An error occured during pair socket setup: " 
                << strerror(pairConnError) << std::endl;
        return false;
    }

    // wait for the renderer to spawn. This will happen when the vnc node connects
    _timeoutCheck(true); // reset timeout counter
    ismStatus = ISM_SYNC_SYNCHRONIZING;
    global_log->info() << "    ISM: Waiting for renderer module to spawn..." << std::endl;
    do {
        // try to sync to Megamol. Abort either on error or after _syncTimeout sec.
        ismStatus = _waitOnMegamolModules();
        if (ismStatus == ISM_SYNC_TIMEOUT
                || ismStatus == ISM_SYNC_REPLY_BUFFER_OVERFLOW
                || ismStatus == ISM_SYNC_SEND_FAILED
                || ismStatus == ISM_SYNC_UNKNOWN_ERROR) {
            global_log->info() << "    ISM: Megamol sync failed." << std::endl;
            return false;
        }
    } while (ismStatus != ISM_SYNC_SUCCESS);

    // send message creating all the modules
    // msg << "mmCreateModule(\"MMPLDDataSource\", \"" << _datTag.str() << "\")\n"
    //     << "mmCreateModule(\"OSPRayNHSphereGeometry\", \"" << _geoTag.str() << "\")\n"
    //     << "mmCreateCall(\"MultiParticleDataCall\", \"" << _geoTag.str() << "::getData\", \"" << _datTag.str() << "::getData\")\n"
    //     << "mmCreateCall(\"CallOSPRayMaterial\", \""<< _geoTag.str() <<"::getMaterialSlot\", " << "\"::mat::deployMaterialSlot\")\n"
    //     << "mmCreateChainCall(\"CallOSPRayStructure\", \"::rnd::getStructure\", \"" << _geoTag.str() << "::deployStructureSlot\")\n";
    // global_log->set_mpi_output_all();
    // global_log->info() << "    ISM: Sending creation message." << std::endl;
    // auto bytesSent = zmq_send(_pairsocket.get(), msg.str().data(), msg.str().size(), 0);
    // if (bytesSent != msg.str().size()) {
    // 	auto pairSendError = errno;
    // 	global_log->info() << "    ISM: zmq_send bytes sent <-> message length: mismatched." 
    // 			<< pairSendError << " " << strerror(pairSendError) << std::endl;
    // 	return false;
    // }
    // global_log->info() << "    ISM: Creation message sent." << std::endl;
    // global_log->set_mpi_output_root();
    // /// confirm that Megamol replied after constructing the modules
    // _timeoutCheck(true); // reset timeout counter
    // do {
    // 	// try to get ack from Megamol. Abort either on error or after _syncTimeout sec.
    // 	ismStatus = _recvCreationMessageAck();
    // 	if (ismStatus == ISM_SYNC_TIMEOUT
    // 			|| ismStatus == ISM_SYNC_REPLY_BUFFER_OVERFLOW
    // 			|| ismStatus == ISM_SYNC_SEND_FAILED
    // 			|| ismStatus == ISM_SYNC_UNKNOWN_ERROR) {
    // 		global_log->info() << "    ISM: Megamol send message failed." << std::endl;
    // 		return false;
    // 	}
    // } while (ismStatus != ISM_SYNC_SUCCESS);
    // surprisingly, stuff actually worked, enable the plugin
    return true;
}

InSitu::InSituMegamol::ISM_SYNC_STATUS InSitu::InSituMegamol::ZmqManager::_getMegamolPairPort(void) {
    std::string msg("Requesting Port!");
    std::string reply;
    std::stringstream statusStr("");
    statusStr << "    ISM: Requesting pair port...";
    if (zmq_send(_requester.get(), msg.data(), msg.size(), BLOCK_POLICY_HANDSHAKE) == -1) {
        statusStr << "send message failed. Error: " << strerror(errno);
        global_log->info() << statusStr.str() << std::endl;
        return InSituMegamol::ISM_SYNC_SEND_FAILED;
    }
    int replySize = zmq_recv(_requester.get(), _replyBuffer.data(), _replyBufferSize, BLOCK_POLICY_HANDSHAKE);
    // evaluate Megamol's reply
    if (replySize == -1) {
        // No reply from Megamol yet, let's wait.
        statusStr << "No pair port received. Error: " << strerror(errno);
        global_log->info() << statusStr.str() << std::endl;
        return _timeoutCheck();
    } 
    if (replySize > _replyBufferSize) {
        // This is an error. Return and handle outside. Should probably do an exception here.
        global_log->info() << "    ISM: reply size exceeded buffer size." << std::endl;
        return InSituMegamol::ISM_SYNC_REPLY_BUFFER_OVERFLOW;
    }
    if (replySize == 0) {
        // Megamol replied, but does not seem ready yet, let's wait.
        statusStr << "ZMQ reply was empty.";
        global_log->info() << statusStr.str() << std::endl;
        return _timeoutCheck();
    }
    if (replySize > 0 && replySize < _replyBufferSize) {
        // Check Megamol's reply
        reply.assign(_replyBuffer.data(), 0, replySize);
        statusStr << "ZMQ reply received (size: " << replySize << ").";
        global_log->info() << statusStr.str() << std::endl;
        if (reply.find_first_not_of("0123456789") == std::string::npos) {
            // Megamol seems ready. Confirm synchronization happened.
            _pairPort.assign(reply);
            global_log->info() << "    ISM: Received Megamol pair port." << std::endl;
            return InSituMegamol::ISM_SYNC_SUCCESS;
        }
        else {
             // Megamol replied, but does not seem ready yet, let's wait.
            global_log->info() << "    ISM: Port reply contained non digits: " << reply << std::endl;
            return _timeoutCheck();
        }
    }
    return InSituMegamol::ISM_SYNC_UNKNOWN_ERROR;
}

InSitu::InSituMegamol::ISM_SYNC_STATUS InSitu::InSituMegamol::ZmqManager::_recvCreationMessageAck(void) {
    std::string reply;
    std::stringstream statusStr("");
    int replySize = zmq_recv(_pairsocket.get(), _replyBuffer.data(), _replyBufferSize, ZMQ_DONTWAIT);
    // evaluate Megamol's reply
    if (replySize == -1) {
        // No reply from Megamol yet, let's wait.
        statusStr << "    ISM: Creation not acknowledged. Error: " << errno << " " << strerror(errno);
        global_log->info() << statusStr.str() << std::endl;
        return _timeoutCheck();
    } 
    if (replySize > _replyBufferSize) {
        // This is an error. Return and handle outside. Should probably do an exception here.
        global_log->info() << "    ISM: reply size exceeded buffer size." << std::endl;
        return InSituMegamol::ISM_SYNC_REPLY_BUFFER_OVERFLOW;
    }
    if (replySize == 0) {
        // Check Megamol's reply
        global_log->info() << "    ISM: Received Megamol ack (empty string)." << std::endl;
        return InSituMegamol::ISM_SYNC_SUCCESS;
    }
    return InSituMegamol::ISM_SYNC_UNKNOWN_ERROR;
}

InSitu::InSituMegamol::ISM_SYNC_STATUS InSitu::InSituMegamol::ZmqManager::_waitOnMegamolModules(void) {
    std::string msg("return mmListModules()");
    std::string reply;
    std::stringstream statusStr("");
    statusStr << "    ISM: Requesting module list...";
    if (zmq_send(_pairsocket.get(), msg.data(), msg.size(), 0) == -1) {
        statusStr << "send message failed. Error: " << strerror(errno);
        global_log->info() << statusStr.str() << std::endl;
        return InSituMegamol::ISM_SYNC_SEND_FAILED;
    }
    int replySize = zmq_recv(_pairsocket.get(), _replyBuffer.data(), _replyBufferSize, BLOCK_POLICY_HANDSHAKE);
    // evaluate Megamol's reply
    if (replySize == -1) {
        // No reply from Megamol yet, let's wait.
        statusStr << "Waiting for renderer. Error: " << errno << " " << strerror(errno);
        global_log->info() << statusStr.str() << std::endl;
        return _timeoutCheck();
    } 
    if (replySize > _replyBufferSize) {
        // This is an error. Return and handle outside. Should probably do an exception here.
        global_log->info() << "    ISM: reply size exceeded buffer size." << std::endl;
        return InSituMegamol::ISM_SYNC_REPLY_BUFFER_OVERFLOW;
    }
    if (replySize == 0) {
        // Megamol replied, but does not seem ready yet, let's wait.
        statusStr << "ZMQ reply was empty.";
        global_log->info() << statusStr.str() << std::endl;
        return _timeoutCheck();
    }
    if (replySize > 0 && replySize < _replyBufferSize) {
        // Check Megamol's reply
        reply.assign(_replyBuffer.data(), 0, replySize);
        statusStr << "ZMQ reply received (size: " << replySize << ").\n" << reply;
        global_log->info() << statusStr.str() << std::endl;
        if (reply.find("OSPRayRenderer") != std::string::npos) {
            // Megamol seems ready. Confirm synchronization happened.
            global_log->info() << "    ISM: Synchronized Megamol." << std::endl;
            return InSituMegamol::ISM_SYNC_SUCCESS;
        }
        else {
             // Megamol replied, but does not seem ready yet, let's wait.
            return _timeoutCheck();
        }
    }
    return InSituMegamol::ISM_SYNC_UNKNOWN_ERROR;
}

InSitu::InSituMegamol::ISM_SYNC_STATUS InSitu::InSituMegamol::ZmqManager::_timeoutCheck(bool const reset) const {
    static int iterationCounter = 0;
    if (reset) {
        iterationCounter = 0;
        return InSituMegamol::ISM_SYNC_RESET;
    }
    std::chrono::high_resolution_clock hrc;
    auto again = hrc.now() + std::chrono::milliseconds(1000);
    while (hrc.now() < again);
    ++iterationCounter;
    // return iterationCounter > _syncTimeout;
    if (iterationCounter > _syncTimeout) {
        // we waited long enough, consider the sync failed
        return InSituMegamol::ISM_SYNC_TIMEOUT;
    }
    else {
        return InSituMegamol::ISM_SYNC_SYNCHRONIZING;
    }
}

bool InSitu::InSituMegamol::ZmqManager::triggerUpdate(std::string fname) {
    InSituMegamol::ISM_SYNC_STATUS ismStatus = ISM_SYNC_SYNCHRONIZING;
    std::stringstream msg;
    msg << "mmSetParamValue(\"::insitu::dat::filename\", \"" << fname << "\")";
    global_log->set_mpi_output_all();
    global_log->info() << " send msg:\n" << msg.str() << std::endl;
    global_log->set_mpi_output_root();
    auto bytesSent = zmq_send(_pairsocket.get(), msg.str().data(), msg.str().size(), 0);
    if (bytesSent != msg.str().size()) {
        auto pairSendError = errno;
        global_log->info() << "    ISM: zmq_send bytes sent <-> message length: mismatched. Error code " 
                << pairSendError << ": " << strerror(pairSendError) << std::endl;
        return false;
    }
    return false;
}

InSitu::InSituMegamol::ISM_SYNC_STATUS InSitu::InSituMegamol::ZmqManager::_recvFileAck(void) {
    std::stringstream statusStr;
    int replySize = zmq_recv(_pairsocket.get(), _replyBuffer.data(), _replyBufferSize, BLOCK_POLICY_HANDSHAKE);
    if (replySize > _replyBufferSize) {
        // This is an error. Should probably throw here.
        global_log->info() << "    ISM: reply size exceeded buffer size. Disabling plugin." << std::endl;
        return ISM_SYNC_REPLY_BUFFER_OVERFLOW;
    }
    if (replySize == -1) {
        // Reply failed, raise unhandled reply counter
        statusStr << "    ISM: Megamol not ready (size: -1). ";
        global_log->info() << statusStr.str() << std::endl;
        return _timeoutCheck();
    } 
    if (replySize == 0) {
        // Megamol replied with 0. This is what we want.
        statusStr << "    ISM: ZMQ reply was empty (size: 0).";
        global_log->info() << statusStr.str() << std::endl;
        return ISM_SYNC_SUCCESS;
    }
    if (replySize > 0 && replySize < _replyBufferSize) {
        // Megamol's reply was not empty. Dump the reply, stop the plugin.
        std::string reply;
        reply.assign(_replyBuffer.data(), 0, replySize);
        statusStr << "    ISM: ZMQ reply received (size: " << replySize << ", should be 0).";
        statusStr << "    ISM: Reply: <" << reply << ">. Disabling plugin.";
        global_log->info() << statusStr.str() << std::endl;
        return ISM_SYNC_UNKNOWN_ERROR;
    }
    return ISM_SYNC_UNKNOWN_ERROR;
}

int InSitu::InSituMegamol::ZmqManager::_send(std::string msg, int blockPolicy) {
    int status;
    // global_log->info() << "    ISM: _sendCount: " << _sendCount << std::endl;
    // while (_sendCount > 0) {
    // 	status = _recv(blockPolicy);
    // 	if (status == -1 && errno != EAGAIN) {
    // 		global_log->info() << "    ISM: Stuff is really messed up." << std::endl;
    // 		return status;
    // 	}
    // }
    status = zmq_send(_pairsocket.get(), msg.data(), msg.size(), blockPolicy);
    if (status != -1) {
        ++_sendCount;
    }
    return status;
}

int InSitu::InSituMegamol::ZmqManager::_recv(int blockPolicy) {
    int status = zmq_recv(_pairsocket.get(), _replyBuffer.data(), _replyBufferSize, blockPolicy);
    if (status != -1) {
        --_sendCount;
    }
    return status;
}
#else

InSitu::InSituMegamol::InSituMegamol(void) {
    global_log->info() << "InSituMegamol: This is a just a dummy."
            << "Set ENABLE_INSITU=ON in cmake options to enable the actual plugin."
            << std::endl;
}

#endif //ENABLE_INSITU
