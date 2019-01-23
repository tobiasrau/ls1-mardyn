/*
 * Events.h
 *
 *  Created on: 4. Dec 2018
 *      Author: Oliver Fernandes
 */

///
/// \file Events.h
/// Events. Drops out enumerated events to a file with a time stamps
///

#ifndef SRC_PLUGINS_EVENTS_H_
#define SRC_PLUGINS_EVENTS_H_

#include <cstdlib>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>


namespace InSitu {
/**
 * @brief Essentially buffers some measurement data, then dumps to a file
 * 
 * Use this class to track the development of a scalar per rank.
 * TimePrecision and EventType can be any serializable type (e.g. double, uint32).
 * Be aware that serialization is implemented via a simple reinterpret_cast: when
 * loading the data again, you need to know how to interpret it.
 */
template <typename TimePrecision, typename EventType>
class EventsBase {
public:
    /** @brief Constructor
     *
     * The only argument is an identifying tag string. Several other infos are 
     * are pre-/appended to get a final filename. Constructor throws if something
     * went wrong.
     * @param fnameTag Name that will separate the created file from other event files
     * @param dumpInterval Number of events to collect in buffer until an actual file
     *                     is written
     */
    EventsBase(std::string fnameTag, unsigned int dumpInterval); 
    virtual ~EventsBase() {};
    /** @brief adds an event to the data dump
     *
     * Used to add another event to the (internal) buffer. Event is described by a 
     * value of type EventType. The time also needs to be passed as TimePrecision type.
     */
    virtual void _addEvent(TimePrecision time, EventType event)=0;
protected:
    bool _isEnabled;
    std::vector<char> _data;
    unsigned int _eventCount = 0;
    unsigned int _dumpInterval;
    std::stringstream _fname;
};

template<typename TimePrecision, typename EventType>
class Events : public EventsBase<TimePrecision, EventType> {
public:
    Events(std::string fnameTag, unsigned int dumpInterval)
            : EventsBase<TimePrecision, EventType>(fnameTag, dumpInterval) {};
    void _addEvent(TimePrecision time, EventType event) override;
};


template <typename TimePrecision>
class Events<TimePrecision, std::string> : public EventsBase<TimePrecision, std::string> {
public:
    Events(std::string fnameTag, unsigned int dumpInterval)
            : EventsBase<TimePrecision, std::string>(fnameTag, dumpInterval) {};
    void _addEvent(TimePrecision time, std::string event) override;
};

// implementation
template <typename TimePrecision, typename EventType>
EventsBase<TimePrecision, EventType>::EventsBase(std::string fnameTag, unsigned int dumpInterval) 
        : _dumpInterval(dumpInterval) {
    ///read file name parts from environment
    std::string nodeName, jobName, resultPath;
    char* nodeName_temp = std::getenv("NODE_HOSTNAME");
    if (!nodeName_temp) {
        _isEnabled = false;
        throw std::logic_error("Getting nodeName from environment failed");
    }
    else
        nodeName.assign(nodeName_temp);

    char* jobName_temp = std::getenv("SLURM_JOB_NAME");
    if (!jobName_temp) {
        _isEnabled = false;
        throw std::logic_error("Getting jobName from environment failed");
    }
    else
        jobName.assign(jobName_temp);

    char* resultPath_temp = std::getenv("RESULT_PATH");
    if (!resultPath_temp) {
        _isEnabled = false;
        throw std::logic_error("Getting resultPath from environment failed");
    }
    else
        resultPath.assign(resultPath_temp);

    _fname.str("");
    _fname << resultPath << "/" << jobName << "." << fnameTag << "." << nodeName << ".log";

    // prepare file
    std::ofstream events;
    // set mask and clear error state flags
    events.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // reset the file if it existed, create it if not
        events.open(_fname.str(), std::ios::trunc);
        events.close();
    }
    catch (std::ofstream::failure e)  {
        //in case the rethrow is caught at higher level, disable the writer
        _isEnabled = false;
        throw;
    }
}

template <typename TimePrecision, typename EventType>
void Events<TimePrecision, EventType>::_addEvent(TimePrecision time, EventType event) {
    if (this->_isEnabled) {
        //convert to byte stream
        this->_data.insert(
                this->_data.end(),
                reinterpret_cast<char*>(&time),
                reinterpret_cast<char*>(&time)+sizeof(time));
        this->_data.insert(
                this->_data.end(),
                reinterpret_cast<char*>(&event),
                reinterpret_cast<char*>(&event)+sizeof(event));
        if ((this->_eventCount % this->_dumpInterval) == 0) {
            std::ofstream events;
            events.open(this->_fname.str(), std::ios::app);
            events.write(this->_data.data(), this->_data.size());
            events.close();
            this->_data.clear();
        }
        ++this->_eventCount;
    }
}

///string specialization
template <typename TimePrecision>
void Events<TimePrecision, std::string>::_addEvent(TimePrecision time, std::string event) {
    if (this->_isEnabled) {
        this->_data.insert(
                this->_data.end(),
                reinterpret_cast<char*>(&time),
                reinterpret_cast<char*>(&time)+sizeof(time));
        this->_data.insert(this->_data.end(), event.begin(), event.end());
        if ((this->_eventCount % this->_dumpInterval) == 0) {
            std::ofstream events;
            events.open(this->_fname.str(), std::ios::app);
            events.write(this->_data.data(), this->_data.size());
            events.close();
            this->_data.clear();
        }
        ++this->_eventCount;
    }
}

}
#endif /* SRC_PLUGINS_EVENTS_H_ */
