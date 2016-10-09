/*******************************************************************************
*
*        COPYRIGHT 2013-2014 MOTOROLA SOLUTIONS, INC. ALL RIGHTS RESERVED.
*                   Motorola Solutions Confidential Restricted
*
********************************************************************************/

// {{{RME classifier 'Logical View::phyrCe::Code::P2P::Link Establishment Management::PH_Site2SiteSM'

#if defined( PRAGMA ) && ! defined( PRAGMA_IMPLEMENTED )
#pragma implementation "PH_Site2SiteSM.h"
#endif

#include <PH_Site2SiteSM.h>
#include <CypherARMDefines.h>
#include <MessageQueue.h>
#include <PH_LEDeregistrationRequest.h>
#include <PH_LEEvent.h>
#include <PH_LEKeepAliveRequest.h>
#include <PH_LEKeepAliveResponse.h>
#include <PH_LEMessage.h>
#include <PH_LEPeerRegistrationRequest.h>
#include <PH_LEPeerRegistrationResponse.h>
#include <PH_LESiteKeepAliveBroadcast.h>
#include <PH_LESoftTimer.h>
#include <PH_LEStateTimerExpiry.h>
#include <PH_LETerminatePeerLink.h>
#include <PH_LEUpdateMap.h>
#include <PH_P2PLogger.h>
#include <PH_PeerLEManagerBase.h>
#include <PH_PeerLESession.h>
#include <FieldLogInterface.h >
#include <FieldLogTagDef.h>


// {{{RME tool 'OT::Cpp' property 'ImplementationPreface'
// {{{USR
extern "C" void Rd_APP_DebugOut(unsigned int enprint, char *fmt, ...);
// }}}USR
// }}}RME
// {{{RME classifier 'PH_P2PLinkStates' tool 'OT::Cpp' property 'ImplementationPreface'
// {{{USR

// }}}USR
// }}}RME

// {{{RME classAttribute 'm_previousSiteID'
unsigned char PH_Site2SiteSM::m_previousSiteID( 0 );
// }}}RME

// {{{RME classAttribute 'm_previousDelay'
unsigned char PH_Site2SiteSM::m_previousDelay( 0 );
// }}}RME

// {{{RME operation 'PH_Site2SiteSM(PH_PeerLEManagerBase*,PH_PeerLESession*,bool,bool)'
PH_Site2SiteSM::PH_Site2SiteSM( PH_PeerLEManagerBase * peerLEManager, PH_PeerLESession * remoteInfo, bool virtualSM, bool noTimeout )
	// {{{RME tool 'OT::Cpp' property 'ConstructorInitializer'
	// {{{USR

	// }}}USR
	// }}}RME
{
	// {{{USR
	m_pLogger = PH_P2PLogger::GetInstance();
	m_keepAliveTimerExpireCount = 0;
	m_keepAliveTimerExpireCountMax = PH_PeerLinkSMTimingValues::PEER_LINK_ACTIVE_INTERVAL / PH_PeerLinkSMTimingValues::PEER_KEEP_ALIVE_FW_OPEN_INTERVAL;
	m_UDP_SYN_TimerExpirationCount = 0;
	m_UDP_SYN_TimerExpirationCountMax = PH_PeerLinkSMTimingValues::PEER_REGISTRATION_QUICK_SYNC_ATTEMPTS;
	m_pPeerLEManager = peerLEManager;
	m_state = 0;
	m_prevState = 0;
	m_linkStatus = 0;
	m_activeInboundCallStatus = false;
	m_activeInboundCallSlot1 = 0;
	m_activeInboundCallSlot2 = 0;
	m_activeOutboundCallStatus = false;
	m_activeOutboundCallSlot1 = 0;
	m_activeOutboundCallSlot2 = 0;
	m_acceptedVersion = LE_PROTOCOL_UNSUPPORTED_VERSION;
	m_OldRequest_TimerExpirationCount = 0;
	m_OldRequest_TimerExpirationCountMax = 3;
	m_noTimeout = noTimeout;
	m_isVirtual = virtualSM;
	m_smLock = false;

	if(m_isVirtual)
	{
		m_smLock = !(m_pPeerLEManager->getRestOwnerFlag());
	}

	m_linkType = P2P_LINK_SM_TYPE;

	m_pPeerLESession = remoteInfo;
	if(!m_isVirtual)
	{
		m_pPeerLESession->setPeerLinkSM( this );
	}
	else
	{
		m_pPeerLESession->setVirtualPeerLinkSM(this);
	}
	PH_LEStateTimerExpiry * stateTimerEvent = new PH_LEStateTimerExpiry(remoteInfo);
	m_pStateTimer = m_pPeerLEManager->getSoftTimer((EventBase *)stateTimerEvent, true);

	if(m_pStateTimer !=0)
	{
		m_pStateTimer->set(0);
		m_pStateTimer->registerListener(this,PH_LEEvent::LE_STATE_TIMER_EXPIRY);
	}
	else
	{
		if(stateTimerEvent)
		{
			delete stateTimerEvent;
		}
	}
	m_pPeerLEManager->registerListener(this, PH_LEEvent::LE_SITE_KEEP_ALIVE_BROADCAST);

	// Trigger the initial state machine transition
	rtg_init1();
#if defined(REPEATER_32M) || defined(WIN32_BUILD)
    m_pPeerLEManager->createLinkSM(S2S_LINK_SM_TYPE);
#endif
	// }}}USR
}
// }}}RME

// {{{RME operation 'eventOccurred(const EventBase* const)'
void PH_Site2SiteSM::eventOccurred( const EventBase * const event )
{
	// {{{USR
	if(event != 0)
	{
		if (event->getType() == LE_EVENT)
		{
			UINT32 eventID = event->getEvent();
			#ifdef INTERNAL_BUILD
			m_pLogger->log(LogStream::WARNING_SEVERITY, "PH_SITESM, eventID: %x", eventID);
			#endif
			
			switch(eventID)
			{
				case PH_LEEvent::LE_STATE_TIMER_EXPIRY:
				{
					trgTimerExpiry();
					break;
				}
				case PH_LEEvent::LE_SITE_KEEP_ALIVE_BROADCAST:
				{	
					trgSiteKeepAliveBroadcast();
					break;
				}
				default:
				{
					// Do nothing, invalid eventID
					break;
				}
			}
		}
		else
		{
			// Do nothing, wrong event type
		}
	}
	else
	{
		// Do nothing, Event object is NULL
	}
	// }}}USR
}
// }}}RME

// {{{RME operation 'startTimer(bool)'
bool PH_Site2SiteSM::startTimer( bool fanout )
{
	// {{{USR
	bool status = false;
	m_startFanOut = fanout;
	if (m_pStateTimer != 0)
	{
		status = resetTimer();	
	}

	return status;
	// }}}USR
}
// }}}RME

// {{{RME operation 'stopTimer()'
bool PH_Site2SiteSM::stopTimer( void )
{
	// {{{USR
	bool status = false;

	if (m_pStateTimer != 0)
	{
		m_pStateTimer->set(0);

		status = true;

	}

	return status;
	// }}}USR
}
// }}}RME

// {{{RME operation 'resetTimer()'
bool PH_Site2SiteSM::resetTimer( void )
{
	// {{{USR
	bool status = false;

	if (m_pStateTimer != 0)
	{
		m_pStateTimer->set(m_timerDuration);
		status = true;
	}

	return status;
	// }}}USR
}
// }}}RME

// {{{RME operation 'setTimerDuration(unsigned int)'
bool PH_Site2SiteSM::setTimerDuration( unsigned int timerDuration )
{
	// {{{USR
	bool status = false;

	if(timerDuration <= MAX_STATE_TIMER_DURATION)
	{
		if( stopTimer() )
		{
			m_timerDuration = timerDuration;
			status = true;
		}
	}

	return status;
	// }}}USR
}
// }}}RME

// {{{RME operation '~PH_Site2SiteSM()'
PH_Site2SiteSM::~PH_Site2SiteSM( void )
{
	// {{{USR
	if (m_pStateTimer != NULL)
	{
		m_pStateTimer->unregisterListener(this,PH_LEEvent::LE_STATE_TIMER_EXPIRY);
		delete m_pStateTimer;
		m_pStateTimer = 0;
	}

	if (m_pPeerLEManager != NULL)
	{
		m_pPeerLEManager->unregisterListener(this, PH_LEEvent::LE_SITE_KEEP_ALIVE_BROADCAST);	
#if defined(REPEATER_32M) || defined(WIN32_BUILD)
        m_pPeerLEManager->destroyedLinkSM(S2S_LINK_SM_TYPE);
#endif
	}
	// }}}USR
}
// }}}RME

// {{{RME operation 'getType()'
ClassType PH_Site2SiteSM::getType( void ) const
{
	// {{{USR
	return P2P_LINK_SM_TYPE;
	// }}}USR
}
// }}}RME

// {{{RME operation 'sendSiteKeepAliveBroadcast(UINT8)'
void PH_Site2SiteSM::sendSiteKeepAliveBroadcast( UINT8 site )
{
	// {{{USR
	// a keepAlive is sent to the Peer’s port to which this state machine is dedicated.
	// State machine must be unlocked to send messages

	if (m_smLock == false)
	{
		m_pPeerLEManager->sendSiteKeepAliveBroadcast(site);
	}
	// }}}USR
}
// }}}RME

// {{{RME operation 'terminateSM(bool)'
void PH_Site2SiteSM::terminateSM( bool callDereg )
{
	// {{{USR
	if(!callDereg)
	{
		//Create a Terminate Peer Link Event and Message and send it to the Intermediary to destroy the link
		PH_LETerminatePeerLink * terminateEvent = new PH_LETerminatePeerLink(this, m_isVirtual, callDereg);
		m_pPeerLEManager->sendMessageInternal((PH_LEEvent*)terminateEvent);
	}
	// }}}USR
}
// }}}RME

// {{{RME operation 'updateLinkStatus(UINT8)'
void PH_Site2SiteSM::updateLinkStatus( UINT8 newLinkStatus )
{
	// {{{USR
	if( newLinkStatus != m_linkStatus )
	{
		m_linkStatus = newLinkStatus;
		m_pPeerLEManager->informLinkStatusUpdate(m_pPeerLESession->getPeerID(), m_linkStatus, m_pPeerLESession->getSiteID());
	}
	else
	{
		// Do nothing, status same as previous status
	}
	// }}}USR
}
// }}}RME

// {{{RME operation 'updateState(UINT8)'
void PH_Site2SiteSM::updateState( UINT8 newState )
{
	// {{{USR
	m_prevState = m_state;
	m_state = newState;
        // field log
	#if defined(REPEATER_32M)  
	uint16_t fieldlogTag = TAG_STATE_MACHINE_STATE<< 8 | SITE2SITE_SM;
	uint32_t fieldlogValue = m_state;
	FIELD_LOG_TV(fieldlogTag, fieldlogValue);
	#endif
	// }}}USR
}
// }}}RME

// {{{RME operation 'restartP2ISM()'
void PH_Site2SiteSM::restartP2ISM( void )
{
	// {{{USR
	PH_PeerLESession* remoteInterm = 0;


	if (m_pPeerLEManager != 0)
	{
		if (m_pPeerLEManager->getLocalPeerType() != INTERMEDIARY_PEER_TYPE)
		{
			remoteInterm = m_pPeerLEManager->getRemoteIntermLESession();

			if (remoteInterm != 0)
			{
				// Do not send PH_LEUpdateMap event to LEManager when RDAC leave this system in Neptune PH_Mode
				if ((g_NRIMode == true) && (m_pPeerLESession->getXNLMasterSupport() == false) && (m_pPeerLESession->getXNLSlaveSupport() == true))
				{
					// RDAC leave system in Neptune PH_Mode
				}
				else
				{
					// Send update map message event to LEManager to restart P2I SM
					PH_LEUpdateMap *updateMap = new PH_LEUpdateMap(remoteInterm);
					m_pPeerLEManager->sendMessageInternal((PH_LEEvent*)updateMap);
				}
			}

		}
	}

	// }}}USR
}
// }}}RME

// {{{RME operation 'setSystemProtocolVersion(UINT16)'
void PH_Site2SiteSM::setSystemProtocolVersion( UINT16 version )
{
	// {{{USR
	//Set protocol version to acceptedVersion in this Peer's LESession
	m_pPeerLESession->setSystemProtocolVersion(version);

	// }}}USR
}
// }}}RME

// {{{RME operation 'updateSystemProtocolInfo()'
void PH_Site2SiteSM::updateSystemProtocolInfo( void )
{
	// {{{USR
	if( m_pPeerLESession->getPeerServices() & 0x00000F00 )
	{
	   if(LE_SYSTEM_ID_CAPACITY_PLUS != m_pPeerLESession->getSystemID())
	   {
	      UINT16 currentVersion = m_pPeerLESession->getCurrentSystemProtocolVersion();
	      UINT16 oldestVersion = m_pPeerLESession->getOldestSystemProtocolVersion();

	      m_pPeerLESession->setSystemID(LE_SYSTEM_ID_CAPACITY_PLUS);

	      currentVersion = (LE_PROTOCOL_VERSION_MASK & currentVersion) | LE_SYSTEM_ID_CAPACITY_PLUS;
	      oldestVersion = (LE_PROTOCOL_VERSION_MASK & oldestVersion) | LE_SYSTEM_ID_CAPACITY_PLUS;
	      m_pPeerLESession->updateProtocolVersions( currentVersion, oldestVersion);
	   }
	}
	else
	{
	   if(LE_SYSTEM_ID_IP_SITE_CONNECT != m_pPeerLESession->getSystemID())
	   {   
	      UINT16 currentVersion = m_pPeerLESession->getCurrentSystemProtocolVersion();
	      UINT16 oldestVersion = m_pPeerLESession->getOldestSystemProtocolVersion();

	      m_pPeerLESession->setSystemID(LE_SYSTEM_ID_IP_SITE_CONNECT);

	      currentVersion = (LE_PROTOCOL_VERSION_MASK & currentVersion) | LE_SYSTEM_ID_IP_SITE_CONNECT;
	      oldestVersion = (LE_PROTOCOL_VERSION_MASK & oldestVersion) | LE_SYSTEM_ID_IP_SITE_CONNECT;
	      m_pPeerLESession->updateProtocolVersions( currentVersion, oldestVersion);
	   }
	}
	// }}}USR
}
// }}}RME

// {{{RME operation 'lock()'
void PH_Site2SiteSM::lock( void )
{
	// {{{USR
	m_smLock = true;
	// }}}USR
}
// }}}RME

// {{{RME operation 'unlock()'
void PH_Site2SiteSM::unlock( void )
{
	// {{{USR
	m_smLock = false;
	startTimer();
	// }}}USR
}
// }}}RME

// {{{RME enter ':TOP:S2_PEER_KEEP_ALIVE'
void PH_Site2SiteSM::rtg_enter2( void )
{
	rtg_state_PH_Site2SiteSM.state = 2U;
	{
		// {{{USR
		updateState(S2_KEEP_ALIVE);

#ifdef INTERNAL_BUILD
		m_pLogger->log(LogStream::WARNING_SEVERITY, "PH_Site2SiteSM, PeerID: %x, State: %x, Version: %x", m_pPeerLESession->getPeerID(), m_state, m_pPeerLESession->getSystemProtocolVersion());
#endif
		// }}}USR
	}
}
// }}}RME

void PH_Site2SiteSM::rtg_init1( void )
{
	// {{{RME transition ':TOP:Initial:Initial'
	{
		// {{{USR
		setTimerDuration(PH_PeerLinkSMTimingValues::PEER_KEEP_ALIVE_FW_OPEN_INTERVAL);

		updateLinkStatus(ACTIVE_LINK_STATUS);
		//1.6 ProtocolVersioning Support
		//setSystemProtocolVersion(m_pPeerLEManager->getCurrentSystemVersionInfo());

		//sendPeerRegistrationRequest();

		startTimer();
		// }}}USR
	}
	// }}}RME
	rtg_enter2();
}

// {{{RME operation 'trgTimerExpiry()'
void PH_Site2SiteSM::trgTimerExpiry( void )
{
	unsigned char rtg_state = rtg_state_PH_Site2SiteSM.state;
	for(;;)
	{
		switch( rtg_state )
		{
		case 2U:
			// {{{RME state ':TOP:S2_PEER_KEEP_ALIVE'
			// {{{RME transition ':TOP:S2_PEER_KEEP_ALIVE:Junction6:S2_T2_Send_SITE_Keep_Alive_Broadcast' guard '1'
			if(
				// {{{USR
				m_keepAliveTimerExpireCount < m_keepAliveTimerExpireCountMax
				// }}}USR
				)
			// }}}RME
			{
				// {{{RME transition ':TOP:S2_PEER_KEEP_ALIVE:Junction6:S2_T2_Send_SITE_Keep_Alive_Broadcast'
				{
					// {{{USR
					if(m_noTimeout)
					{
						if(m_startFanOut)
						{
							if(!m_vectorOfSites.empty())
							{
								m_vectorOfSites.clear();
							}

							//retrieve the sorted site list which need send keep alive to when startFanOut flag set
							m_pPeerLEManager->getConnectedSites(m_vectorOfSites);
							m_previousDelay = 0;
						}

						//send keep alive
						if(m_previousDelay>0)
						{
							sendSiteKeepAliveBroadcast(m_previousSiteID);
						}

						if (m_vectorOfSites.empty())
						{
							//reset to 6 seconds as no staggering sending this time
							setTimerDuration(PH_PeerLinkSMTimingValues::PEER_KEEP_ALIVE_FW_OPEN_INTERVAL-m_previousDelay);
							resetTimer();
							m_startFanOut = true; //try to fanout next timer expiry
							m_previousDelay =0;
						}
						else
						{
							//continue the fan out sending
							UINT8 remoteSiteID = m_vectorOfSites.back();
							UINT8 localSiteID =  m_pPeerLEManager->getLocalLESession()->getSiteID();

							//this code is for staggering keep alive sending, do not change it until you understand the algorithm
							//opt for avoid /50UINT32 delay = (localSiteID + remoteSiteID) * 50 - 100 + ((remoteSiteID>localSiteID)?50:0);
							UINT32 delay = (localSiteID + remoteSiteID) - 2 + ((remoteSiteID>localSiteID)?1:0);

					            if(delay>31)
							    delay %= 31;//1550/50
						
							UINT32 timeValue=delay-m_previousDelay;
							m_previousDelay = delay;
							m_previousSiteID= remoteSiteID;

							m_vectorOfSites.pop_back();

							//timeValue is based on 50ms timer resolution
							setTimerDuration(timeValue);
							startTimer(); //resets fanout flag in this function
							m_startFanOut = false;	

						}	
					}
					else
					{
						m_keepAliveTimerExpireCount++;

						if (m_keepAliveTimerExpireCount > 2)
						{
							updateLinkStatus(IDLE_LINK_STATUS);
						}
						startTimer();
					}

					// }}}USR
				}
				// }}}RME
				rtg_enter2();
				return;
			}
			// {{{RME transition ':TOP:S2_PEER_KEEP_ALIVE:J4A79A3E800D6:S2_T5_Link_Timeout' guard '1'
			if(
				// {{{USR
				(m_keepAliveTimerExpireCount >= m_keepAliveTimerExpireCountMax)
				// }}}USR
				)
			// }}}RME
			{
				// {{{RME transition ':TOP:S2_PEER_KEEP_ALIVE:J4A79A3E800D6:S2_T5_Link_Timeout'
				{
					// {{{USR
#if 1//disable site keep alive detection					
					stopTimer();

					updateState(S2_EXIT_1);
					updateLinkStatus(REMOVE_LINK_STATUS);

#ifdef INTERNAL_BUILD
					m_pLogger->log(LogStream::WARNING_SEVERITY, "PH_Site2SiteSM::S2_T5-Link Timeout, PeerID: %x, State: %x", m_pPeerLESession->getPeerID(), m_state);
#endif

					restartP2ISM();

					terminateSM();
#endif					
					// }}}USR
				}
				// }}}RME
				// enter final state ':TOP:S2_EXIT_1'
				rtg_state_PH_Site2SiteSM.state = 1U;
				return;
			}
			rtg_state = 1U;
			break;
			// }}}RME
		default:
			return;
		}
	}
}
// }}}RME

// {{{RME operation 'trgSiteKeepAliveBroadcast()'
void PH_Site2SiteSM::trgSiteKeepAliveBroadcast( void )
{
	unsigned char rtg_state = rtg_state_PH_Site2SiteSM.state;
	for(;;)
	{
		switch( rtg_state )
		{
		case 2U:
			// {{{RME state ':TOP:S2_PEER_KEEP_ALIVE'
			// {{{RME transition ':TOP:S2_PEER_KEEP_ALIVE:J5339320300E6:S2_T1_Receive_SITE_Keep_Alive_Broadcast'
			{
				// {{{USR

				m_keepAliveTimerExpireCount = 0;
				updateLinkStatus(ACTIVE_LINK_STATUS);
				// }}}USR
			}
			// }}}RME
			rtg_enter2();
			return;
			// }}}RME
		default:
			return;
		}
	}
}
// }}}RME

// {{{RME tool 'OT::Cpp' property 'ImplementationEnding'
// {{{USR

// }}}USR
// }}}RME
// {{{RME classifier 'PH_P2PLinkStates' tool 'OT::Cpp' property 'ImplementationEnding'
// {{{USR

// }}}USR
// }}}RME

// }}}RME
