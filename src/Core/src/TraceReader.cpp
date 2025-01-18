#include "TraceReader.h"
#include <fstream>
#include <sstream>
#include <iostream>

#include "events/VMRequestEvent.h"
#include "events/VMDepartureEvent.h"
#include "events/VMUtilUpdateEvent.h"
#include "data/VirtualMachine.h"
#include "data/Resources.h"

TraceReader::TraceReader(const std::string &filename, ConcurrentEventQueue &q)
    : m_filename(filename), m_queue(q), m_stop(false)
{
}

TraceReader::~TraceReader()
{
    stop();
}

void TraceReader::start()
{
    m_stop = false;
    m_thread = std::thread(&TraceReader::parsingLoop, this);
}

void TraceReader::stop()
{
    m_stop = true;
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void TraceReader::parsingLoop()
{
    std::ifstream file(m_filename);
    if (!file.is_open())
    {
        std::cerr << "[TraceReader] Could not open " << m_filename << std::endl;
        return;
    }
    while (!m_stop)
    {
        std::string line;
        if (!std::getline(file, line))
        {
            break; // EOF
        }
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        std::stringstream ss(line);
        // Example format:
        // requestId, reqType, t_start, duration, c, r, d, b, f, valSize, ...
        int reqId, reqType;
        ss >> reqId;
        ss.ignore(1);
        ss >> reqType;
        ss.ignore(1);
        if (!ss || ss.fail())
        {
            continue;
        }
        if (reqType == 0)
        {
            // VM request
            double tstart, duration;
            double c, r, d, b, f;
            int valSize;
            ss >> tstart;
            ss.ignore(1);
            ss >> duration;
            ss.ignore(1);
            ss >> c;
            ss.ignore(1);
            ss >> f;
            ss.ignore(1);
            ss >> r;
            ss.ignore(1);
            ss >> d;
            ss.ignore(1);
            ss >> b;
            ss.ignore(1);
            ss >> valSize;
            ss.ignore(1);

            // create VM
            auto requested = Resources(c, r, d, b, f);
            double initUtil = 0;
            ss >> initUtil;
            ss.ignore(1);
            valSize--;

            auto vm = std::make_unique<VirtualMachine>(reqId, requested, duration);
            vm->setUtilization(initUtil / 100);
            // parse usage steps
            double step = duration / (valSize > 0 ? valSize : 1);
            if (valSize == 0)
                throw std::runtime_error("valSize == 0");
            for (int i = 0; i < valSize; i++)
            {
                double util;
                ss >> util;
                ss.ignore(1);
                vm->addFutureUtilization((i + 1) * step, util / 100.0);
            }
            // push event
            auto evt = std::make_shared<VMRequestEvent>(tstart, std::move(vm));
            m_queue.push(evt);
        }
        else
        {
            std::cerr << "[TraceReader] unknown reqType " << reqType << " line: " << line << "\n";
        }
    }
    file.close();
    std::cout << "[TraceReader] Finished reading.\n";
}