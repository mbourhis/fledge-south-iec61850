#ifndef INCLUDE_IEC61850_CLIENT_H_
#define INCLUDE_IEC61850_CLIENT_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <memory>
#include <thread>  // NOLINT
#include <mutex>   // NOLINT
#include <atomic>

// libiec61850 headers
#include <libiec61850/mms_value.h>

// Fledge headers
#include <logger.h>
#include <reading.h>


// local library
#include "./iec61850_client_config.h"

class IEC61850;
class IEC61850ClientConnection;
class Mms;

/** \class IEC61850Client
 *  \brief Read from and write to a IED
 *
 *  Handle IEC61850 data objects
 *  for reading data
 *  or sending order and operation
 */
class IEC61850Client
{
    public :

        explicit IEC61850Client(IEC61850 *iec61850,
                                std::shared_ptr<IEC61850ClientConfig> config);

        ~IEC61850Client();

        /**
         * Open the connection with the IED
         */
        void start();

        /**
         * Close the connection with the IED
         */
        void stop();

    private:
        void createConnection();
        void destroyConnection();

        /**
         * Create the Datapoint object that will be ingest by Fledge
         * (dynamic allocation, deallocation by Fledge core).
         * Reentrant function, thread safe
         */
        template <typename T>
        static Datapoint *createDatapoint(const std::string &dataName,
                                          T primitiveTypeValue);

        /**
         * Convert the MMS into Datapoint
         * by extracting the MMS content and creating a new Datapoint
         * Reentrant function, thread safe
         */
        static Datapoint *convertMmsToDatapoint(std::shared_ptr<Mms> mms);

        /**
         * Send a datapoint to Fledge core
         * Reentrant function, thread safe
         */
        void sendData(Datapoint *datapoint);


        IEC61850 *m_iec61850;
        std::shared_ptr<IEC61850ClientConfig> m_config;

        std::unique_ptr<IEC61850ClientConnection> m_connection;

        // For demo only
        void startDemo();
        void stopDemo();
        void readMmsLoop();
        std::atomic<bool> m_isDemoLoopActivated{false};
        std::thread m_demoLoopThread;
};

#endif  // INCLUDE_IEC61850_CLIENT_H_
