/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

#include "./iec61850_client_connection.h"

// libiec61850 headers
#include <libiec61850/iec61850_common.h>

IEC61850ClientConnection::IEC61850ClientConnection(
    const ServerConnectionParameters &connParam)
    : m_connectionParam(connParam)
{
    Logger::getLogger()->debug("IEC61850ClientConn: constructor");
    m_iedConnection = IedConnection_create();
    open();
}

IEC61850ClientConnection::~IEC61850ClientConnection()
{
    Logger::getLogger()->debug("IEC61850ClientConn: destructor");
    close();
    IedConnection_destroy(m_iedConnection);
}

bool IEC61850ClientConnection::isNoError() const
{
    return (m_networkStack_error == IED_ERROR_OK);
}

bool IEC61850ClientConnection::isConnected()
{
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    return (IedConnection_getState(m_iedConnection) == IED_STATE_CONNECTED);
}

void IEC61850ClientConnection::open()
{
    Logger::getLogger()->debug("IEC61850ClientConn: open");
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    Logger::getLogger()->info("Open connection with: ");
    IEC61850ClientConfig::logIedConnectionParam(m_connectionParam);

    // Set OSI parameters
    if (m_connectionParam.isOsiParametersEnabled) {
        setOsiConnectionParameters();
    }

    IedConnection_connect(m_iedConnection,
                          &m_networkStack_error,
                          m_connectionParam.ipAddress.c_str(),
                          m_connectionParam.mmsPort);
}

void IEC61850ClientConnection::setOsiConnectionParameters()
{
    MmsConnection mmsConnection = IedConnection_getMmsConnection(m_iedConnection);
    IsoConnectionParameters libiecIsoParams = MmsConnection_getIsoConnectionParameters(mmsConnection);
    const OsiParameters &osiParams = m_connectionParam.osiParameters;

    // set Remote 'AP Title' and 'AE Qualifier'
    if (! osiParams.remoteApTitle.empty()) {
        IsoConnectionParameters_setRemoteApTitle(libiecIsoParams,
                osiParams.remoteApTitle.c_str(),
                osiParams.remoteAeQualifier);
    }

    // set Local 'AP Title' and 'AE Qualifier'
    if (! osiParams.localApTitle.empty()) {
        IsoConnectionParameters_setLocalApTitle(libiecIsoParams,
                                                osiParams.localApTitle.c_str(),
                                                osiParams.localAeQualifier);
    }

    /* change parameters for presentation, session and transport layers */
    IsoConnectionParameters_setRemoteAddresses(libiecIsoParams,
            osiParams.remotePSelector,
            osiParams.remoteSSelector,
            osiParams.localTSelector);
    IsoConnectionParameters_setLocalAddresses(libiecIsoParams,
            osiParams.localPSelector,
            osiParams.localSSelector,
            osiParams.remoteTSelector);
}

void IEC61850ClientConnection::close()
{
    Logger::getLogger()->debug("IEC61850ClientConn: close");
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    IedConnection_close(m_iedConnection);
}

std::shared_ptr<WrappedMms>
IEC61850ClientConnection::readMms(const std::string &daPath,
                                  const std::string &fcName)
{
    // Preconditions
    if (! isConnected()) {
        return nullptr;
    }

    FunctionalConstraint functionalConstraint =
        FunctionalConstraint_fromString(fcName.c_str());
    auto wrapped_mms = std::make_shared<WrappedMms>();
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    wrapped_mms->setMmsValue(IedConnection_readObject(m_iedConnection,
                             &m_networkStack_error,
                             daPath.c_str(),
                             functionalConstraint));
    return wrapped_mms;
}
