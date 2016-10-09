// EmeraldFloorArbitrationTest.cpp : Defines the entry point for the console application.
//
#include <list>
#include <vector>

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <PH_MediatorBurstMapMock.h>
#include <EmeraldFloorArbitrationMediatorClient.h>
#include <EmeraldFloorArbitration.h>
#include <PH_FloorArbitrationResult.h>
#include <PH_FloorRequestObject.h>
#include <PH_Mediator.h>
#include <PH_CallType.h>
#include <PH_BurstObjectMasks.h>
#include <PH_ControlInvitation.h>
#include <P_Grant.h>
#include <BurstObjectComparator.h>
#include <BurstObjectByCoding.h>

#include <PH_FloorState.h>
#include <BurstObjectAbstractFactory.h>
#include <PH_ResponseHeaderBlock.h>
#include <PH_StopBuffer.h>
#include <PH_UpdateMediatorClientProfile.h>
#include "DigitalConfigure.h"

#define TV_GRANT 0x31	
#define CALL_CONTROL_INVITATION_EVENT 0x6e
#define GROUP_VOICE_CALL 9
#define SOURCE_ADD 0x3ef7
#define TARGET_ADD 0x1  //group id
#define SOURCE_SITE_ID 1
#define TARGET_SITE_ID 1
#define CHANNEL_ID 0

#define SLOT_ONE 0
#define SLOT_TWO 1
#define CHANNEL_NUM 0
#define PEER_ID 100

#define FLOOR_IDLE 0
#define FLOOR_REQUESTED 1
#define FLOOR_GRANTED 2
#define LOCAL_HANG_TIME 3
#define PHYRCE_GRP_VOICE_CALL_PRIORITY 3
#define FLOOR_CONTROL_TAG 299
#define ARBITRATION_DURATION 2



EmeraldFloorArbitration *p_floorArb = NULL;
ReqProcessEmeraldHelper * m_role = 0;

#include <PH_UTCodeplugManager.h>
#include <CypDSIF.h>
#include <PH_AbstractBurstObject.h>
#include <PH_CallHang.h>

extern PH_MediatorBurstMapMock *pBurstMapMock;
extern BurstObjectComparator comparator;

extern PH_Mediator * pMediator;

uint16_t voiceHeaderData[] = {0x140, 0xa, 0x800a, 0x60, 0x0, 0x0, 0x1, 0x0, 0x3ef7, 0xacd6, 0x11, 0x19b, 0x0, 0x0, 0x0, 0x54, 0x1ae, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};


class ReqProcessEmeraldHelperForTest: public ReqProcessEmeraldHelper
{
public:
    void setAdditionalReplicaterIPDelay(bool isReplicatorInvolved)
    {
        ReqProcessEmeraldHelper::setAdditionalReplicaterIPDelay(isReplicatorInvolved);
    }
    ReqProcessEmeraldHelperForTest(unsigned int slotNum): ReqProcessEmeraldHelper(slotNum){}
};

TEST_GROUP(FloorResultPublishWillAdjustIPDelay)
{
    static const bool ReplicatorInvolved = true;
    static const bool ReplicatorNotInvolved = false;

    void setup()
    {
        DigitalConfigure::IP_DELAY = 2;
    }

    void teardown()
    {
        ReqProcessEmeraldHelperForTest HelperTest1(REPEATER_SLOT_ONE);
        ReqProcessEmeraldHelperForTest HelperTest2(REPEATER_SLOT_TWO);

        HelperTest1.setAdditionalReplicaterIPDelay(0);
        HelperTest2.setAdditionalReplicaterIPDelay(0);
    }
};

TEST(FloorResultPublishWillAdjustIPDelay, SlotOneFloorResultWithReplicatorInvolvedWillSetAdditionalReplicatorIPDelayOnSlotOne)
{
    ReqProcessEmeraldHelperForTest HelperTest(REPEATER_SLOT_ONE);

    HelperTest.setAdditionalReplicaterIPDelay(ReplicatorInvolved);

    LONGS_EQUAL(DigitalConfigure::IP_DELAY + DigitalConfigure::ADDITIONAL_REPLICATER_IP_DELAY, DigitalConfigure::getTotalIPDelaySlot1());
}

TEST(FloorResultPublishWillAdjustIPDelay, SlotTwoFloorResultWithReplicatorInvolvedWillSetAdditionalReplicatorIPDelayOnSlotTwo)
{
    ReqProcessEmeraldHelperForTest HelperTest(REPEATER_SLOT_TWO);

    HelperTest.setAdditionalReplicaterIPDelay(ReplicatorInvolved);

    LONGS_EQUAL(DigitalConfigure::IP_DELAY + DigitalConfigure::ADDITIONAL_REPLICATER_IP_DELAY, DigitalConfigure::getTotalIPDelaySlot2());
}
TEST(FloorResultPublishWillAdjustIPDelay, BothFloorResultWithReplicatorInvolvedWillSetAdditionalReplicatorIPDelayOnBothSlot)
{
    ReqProcessEmeraldHelperForTest HelperTest1(REPEATER_SLOT_ONE);
    ReqProcessEmeraldHelperForTest HelperTest2(REPEATER_SLOT_TWO);

    HelperTest1.setAdditionalReplicaterIPDelay(ReplicatorInvolved);
    HelperTest2.setAdditionalReplicaterIPDelay(ReplicatorInvolved);

    LONGS_EQUAL(DigitalConfigure::IP_DELAY + DigitalConfigure::ADDITIONAL_REPLICATER_IP_DELAY, DigitalConfigure::getTotalIPDelaySlot2());
    LONGS_EQUAL(DigitalConfigure::IP_DELAY + DigitalConfigure::ADDITIONAL_REPLICATER_IP_DELAY, DigitalConfigure::getTotalIPDelaySlot1());
}

TEST(FloorResultPublishWillAdjustIPDelay, SlotTwoFloorResultWithOutReplicatorInvolvedWillResetAdditionalReplicatorIPDelayOnSlotTwo)
{
    ReqProcessEmeraldHelperForTest HelperTest(REPEATER_SLOT_ONE);

    HelperTest.setAdditionalReplicaterIPDelay(ReplicatorInvolved);
    HelperTest.setAdditionalReplicaterIPDelay(ReplicatorNotInvolved);

    LONGS_EQUAL(DigitalConfigure::IP_DELAY, DigitalConfigure::getTotalIPDelaySlot1());
}

TEST(FloorResultPublishWillAdjustIPDelay, SlotOneFloorResultWithOutReplicatorInvolvedWillResetAdditionalReplicatorIPDelayOnSlotOne)
{
    ReqProcessEmeraldHelperForTest HelperTest(REPEATER_SLOT_TWO);

    HelperTest.setAdditionalReplicaterIPDelay(ReplicatorInvolved);
    HelperTest.setAdditionalReplicaterIPDelay(ReplicatorNotInvolved);

    LONGS_EQUAL(DigitalConfigure::IP_DELAY, DigitalConfigure::getTotalIPDelaySlot2());
}
TEST_GROUP(EmeraldFloorArbitrationUT)
{
	void setup()
	{
		m_role = new ReqProcessEmeraldHelper(SLOT_ONE);
		p_floorArb = new EmeraldFloorArbitration(SLOT_ONE,pMediator);

        DigitalConfigure::IP_DELAY = 2;
		p_floorArb->initSM(m_role);
		
		mock().installComparator("PH_AbstractBurstObject*", comparator);
		mock().ignoreOtherCalls();

	}

	void teardown()
	{
		mock().checkExpectations();
		mock().removeAllComparators();

		delete p_floorArb;
		delete m_role;
		mock().clear();
	}
};


TEST(EmeraldFloorArbitrationUT, handleCallHangRequest_isValidResponse)
{

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	uint16_t data[] = {0x640,0xB,0x840A,0x60,0x140,0x3e,0xf700,0x0001,0x0008,0xB071,0x0016,0x0136,0x14de};	
	uint16_t *rawdata = new uint16_t[0x0b];
	memcpy(rawdata, data,0x0b*2);
	PH_ResponseHeaderBlock *dataObj = new PH_ResponseHeaderBlock(rawdata);

	PH_CallInvitation* inv = new PH_CallInvitation(1, dataObj->getOrigPeerID(), dataObj->getSourceAddress(), dataObj->getTargetAddress(), dataObj->getFloorControlTag(), 
	PH_CallType::getCallType(dataObj), dataObj->getSlotNum(), dataObj->getSlotNum(), 
	PH_CallType::getCallPriorityByType(PH_CallType::getCallType(dataObj)), NO_ARBITRATION,dataObj);

	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,dataObj->getSourceAddress(),dataObj->getTargetAddress(),0,0,PH_CallType::getCallType(dataObj),dataObj->getSlotNum(),1,0);

	sBurstObjectType burstObjectCI = {  TYPE_POINTER, inv, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectCI);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation");
	
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);  
	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	delete expectedFloorResult1;

}
TEST(EmeraldFloorArbitrationUT, handleCallHangRequest_isValidResponse_FROMIP)
{

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);	
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	uint16_t data[] = {0x640,0xB,0x840A,0x60,0x140,0x3e,0xf700,0x0086,0x0008,0xB071,0x0016,0x0136,0x14de};	
	uint16_t *rawdata = new uint16_t[0x0b];
	memcpy(rawdata, data,0x0b*2);
	PH_ResponseHeaderBlock *dataObj = new PH_ResponseHeaderBlock(rawdata);

	PH_CallInvitation* inv = new PH_CallInvitation(1, 5, 134, SOURCE_ADD, 1234, PH_CallType::PHYRCE_PRI_CSBK_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_PRI_CSBK_CALL), NO_ARBITRATION,dataObj);
	inv->setOrigin(FROM_IP);

	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,134,SOURCE_ADD,0,0,PH_CallType::PHYRCE_PRI_CSBK_CALL,SLOT_ONE,1,0);

	sBurstObjectType burstObjectCI = {  TYPE_POINTER, inv, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectCI);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");

	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);
	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	delete expectedFloorResult1;

}
TEST(EmeraldFloorArbitrationUT, handleCallHangRequest_NO_ARB)
{
//Set m_floorOwner via invokeTrgViaMap function.

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);
	p_GrantEvent->updateCallSeqNum(1);
	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);  
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);

//Insert no arbitration invitation when call hang

	PH_CallInvitation *inv = new PH_CallInvitation(SOURCE_SITE_ID, PEER_ID, SOURCE_ADD, TARGET_ADD, FLOOR_CONTROL_TAG, 
		PH_CallType::PHYRCE_PRI_CSBK_CALL, SLOT_ONE, SLOT_ONE, PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_PRI_CSBK_CALL), NO_ARBITRATION,NULL);	
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	sBurstObjectType burstObjectCI = {  TYPE_POINTER, inv, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectCI);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation");
//The invitation inv will be rejected since it is invalid call request in emerald call hang.
	PH_ArbFail * expectedFail = new PH_ArbFail(ARB_INVALIDACLL,SOURCE_ADD,TARGET_ADD,PH_CallType::PHYRCE_PRI_CSBK_CALL,FROM_OTA,SLOT_ONE);

	std::list<PH_ArbFail*>* failList = new std::list<PH_ArbFail*> ;
	failList->push_back(expectedFail);
	PH_ArbFailListEvent* expectFailList = new PH_ArbFailListEvent(failList,SLOT_ONE);
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectFailList);  

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");
	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	delete expectedFloorResult1;
	delete expectFailList;

}



TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_MultipleGrant)
{
	//take voice group on slot1 for example
	bool ret = false;

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);
	p_GrantEvent->updateCallSeqNum(1);
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, TARGET_SITE_ID);
	p_GrantEvent1->updateCallSeqNum(11);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO1);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	

	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	PH_AbstractBurstObject *p_GrantBO2 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+2, 1, TARGET_SITE_ID+2, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+2, TARGET_ADD+2,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2, TARGET_SITE_ID);
	p_GrantEvent2->updateCallSeqNum(22);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	((PH_ControlInvitation*) p_GrantEvent2)->setCtrlOrigin(CTRL_FROM_LATEENTRY);

	PH_AbstractBurstObject *p_GrantBO3 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+3, 1, TARGET_SITE_ID+3, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent3 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+3, TARGET_ADD+3,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO3, TARGET_SITE_ID);
	p_GrantEvent3->updateCallSeqNum(33);
	((PH_ControlInvitation*) p_GrantEvent3)->setCtrlOrigin(CTRL_FROM_LATEENTRY);
	sBurstObjectType burstObjectGrant3= { TYPE_POINTER, p_GrantEvent3, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant3);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");


	p_floorArb->invokeTrgViaMap(pBurstMapMock);
}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_LocalAndTCGrant)
{
	//take voice group on slot1 for example
	bool ret = false;

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	p_GrantEvent->updateCallSeqNum(11);

	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD+1, SOURCE_ADD+1, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+1, TARGET_ADD+1,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, TARGET_SITE_ID);
	((PH_ControlInvitation*) p_GrantEvent1)->setCtrlOrigin(CTRL_FROM_LATEENTRY);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };
	p_GrantEvent1->updateCallSeqNum(22);

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");
//
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO);
	

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb->syncSlotState(pBurstMapMock);


	PH_AbstractBurstObject *p_GrantBO2 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+2, 1, TARGET_SITE_ID+2, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+2, TARGET_ADD+2,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2, TARGET_SITE_ID);
	p_GrantEvent2->updateCallSeqNum(33);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };

	PH_AbstractBurstObject *p_GrantBO3 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+3, 1, TARGET_SITE_ID+3, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent3 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+3, TARGET_ADD+3,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO3, TARGET_SITE_ID);
		((PH_ControlInvitation*) p_GrantEvent3)->setCtrlOrigin(CTRL_FROM_LATEENTRY);
		p_GrantEvent3->updateCallSeqNum(44);
	sBurstObjectType burstObjectGrant3= { TYPE_POINTER, p_GrantEvent3, 0, 0, 0,"" };


	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant3);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").ignoreOtherParameters();
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO2);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);

}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_IDLE)
{
//take voice group on slot1 for example
	bool ret = false;

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };
	
	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO);


	p_floorArb->invokeTrgViaMap(pBurstMapMock);
}

TEST(EmeraldFloorArbitrationUT, TwoGRANT_in_Samecall)
{
	bool ret = true;

	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, TARGET_SITE_ID);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);   
	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, TARGET_SITE_ID);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");
	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	delete expectedFloorResult1;

}


TEST(EmeraldFloorArbitrationUT, TwoGRANT_For_Differentcall)
{	
	bool ret = false;

	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, TARGET_SITE_ID);
	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	p_GrantEvent1->updateCallSeqNum(1);

	PH_AbstractBurstObject *p_GrantBO2 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+1, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+1, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2, TARGET_SITE_ID);	
	PH_AbstractBurstObject *p_GrantBO2x = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+1, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent2x = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+1, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2x, TARGET_SITE_ID);	
	PH_FloorArbitrationResult *expectedFloorResult2 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+1,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);

	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectGrant2x= { TYPE_POINTER, p_GrantEvent2x, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	
    p_floorArb->invokeTrgViaMap(pBurstMapMock);
	p_GrantEvent2->updateCallSeqNum(2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2x);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult2);
	//mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2x);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	
    p_floorArb->invokeTrgViaMap(pBurstMapMock);
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb->syncSlotState(pBurstMapMock);
	delete expectedFloorResult1;
	delete expectedFloorResult2;



}
TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_REQUESTED_SameCSNGrantAsCallHang_Reject)
{
	bool ret = false;
// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);
	p_GrantEvent->updateCallSeqNum(1);
	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

//Inject invitation from IP when in call hang
	PH_CallInvitation* inv  = new PH_CallInvitation(3,5, SOURCE_ADD+1, TARGET_ADD, 111, PH_CallType::PHYRCE_GRP_VOICE_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_GRP_VOICE_CALL), CALL_START,NULL);

	inv->setOrigin(FROM_IP);

	sBurstObjectType burstObjectType = {  TYPE_POINTER, inv, 0, 0, 0,"" };
//	mock("PH_MediatorBurstMap").expectOneCall("getBurstByType");
	sBurstObjectType burstNullType = {  TYPE_POINTER, NULL, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstNullType);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectType);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	p_floorArb->invokeTrgViaMap(pBurstMapMock); 


//Get same CSN with call hang call info csn grant when in floor requested state.
	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+5, 1, 2, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+5, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	p_GrantEvent1->updateCallSeqNum(1);

	
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);

	PH_ArbFail* expectedFail = new PH_ArbFail(ARB_DESTBUSY,SOURCE_ADD+5,TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,0,SLOT_ONE);
	std::list<PH_ArbFail*>* failList = new std::list<PH_ArbFail*> ;
	failList->push_back(expectedFail);
	PH_ArbFailListEvent* expectFailList = new PH_ArbFailListEvent(failList,SLOT_ONE);
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO",expectFailList);
	
	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	p_floorArb ->handleFailEvent(pBurstMapMock);

	delete expectFailList;


}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_REQUESTED)
{
	bool ret = false;
// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);
	//p_GrantEvent->updateCallSeqNum(1);

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

//Inject invitation from IP when in call hang
	PH_CallInvitation* inv  = new PH_CallInvitation(3,5, SOURCE_ADD+1, TARGET_ADD, 111, PH_CallType::PHYRCE_GRP_VOICE_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_GRP_VOICE_CALL), CALL_START,NULL);

	inv->setOrigin(FROM_IP);

	sBurstObjectType burstObjectType = {  TYPE_POINTER, inv, 0, 0, 0,"" };
//	mock("PH_MediatorBurstMap").expectOneCall("getBurstByType");
	sBurstObjectType burstNullType = {  TYPE_POINTER, NULL, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstNullType);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectType);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	p_floorArb->invokeTrgViaMap(pBurstMapMock); 


//Get grant when in floor requested state.
	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD+5, SOURCE_ADD+5, 1, 2, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+5, TARGET_ADD+5,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	PH_AbstractBurstObject *p_GrantBO2 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2, 3);
	PH_AbstractBurstObject *p_GrantBO3 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent3 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO3, 3);
	p_GrantEvent1->updateCallSeqNum(2);
	p_GrantEvent2->updateCallSeqNum(2);
	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectGrant3= { TYPE_POINTER, p_GrantEvent3, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantEvent2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant3);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	delete expectedFloorResult;
}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_GrantComeInCALHANG_differentCSN_differentControlOrigin)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);
	p_GrantEvent->updateCallSeqNum(1);
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD+5, SOURCE_ADD+5, 1, 2, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+5, TARGET_ADD+5,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	p_GrantEvent1->updateCallSeqNum(2);
	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+5,TARGET_ADD+5,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);
	
	p_floorArb->invokeTrgViaMap(pBurstMapMock);

    //Update to local_call_hang
    mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
    p_floorArb ->syncSlotState(pBurstMapMock);

    p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD+6, SOURCE_ADD+6, 1, 2, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
    p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+5, TARGET_ADD+5,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
    ((PH_ControlInvitation *)p_GrantEvent1)->setCtrlOrigin(CTRL_FROM_LATEENTRY);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

    mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
    mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");
	//If in grant state, the late entry grant shall not derive the floor result
    //mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);

    p_floorArb->invokeTrgViaMap(pBurstMapMock);
	

	delete expectedFloorResult;
}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_GrantComeInCALHANG_sameCSN)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);
	p_GrantEvent->updateCallSeqNum(1);
	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	p_GrantEvent1->updateCallSeqNum(1);


	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);

	PH_ArbFail* expectedFail = new PH_ArbFail(ARB_SLOTBUSY,SOURCE_ADD,TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,0,SLOT_ONE);
	std::list<PH_ArbFail*>* failList = new std::list<PH_ArbFail*> ;
	failList->push_back(expectedFail);
	PH_ArbFailListEvent* expectFailList = new PH_ArbFailListEvent(failList,SLOT_ONE);
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO",expectFailList);
	
	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	p_floorArb ->handleFailEvent(pBurstMapMock);

	delete expectFailList;
}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_InvalidPMaintComeInCALHANG)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_CallInvitation *p_GrantEvent2 = new PH_CallInvitation(SOURCE_SITE_ID, 100, SOURCE_ADD, TARGET_ADD, 0, 
		PH_CallType::PHYRCE_P_MAINT_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_P_MAINT_CALL), CALL_START,NULL);

//	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+5,TARGET_ADD+5,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectNULL = { TYPE_POINTER, NULL, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation").andReturnValue(&burstObjectNULL);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectNULL);
//	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);

}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_AllcallInvComeInCALHANG)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_EMR_MULTISITE_ALL_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_CallInvitation *p_GrantEvent2 = new PH_CallInvitation(SOURCE_SITE_ID, 100, SOURCE_ADD, TARGET_ADD, 0, 
		PH_CallType::PHYRCE_EMR_MULTISITE_ALL_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_EMR_MULTISITE_ALL_CALL), CALL_START,NULL);

	//	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+5,TARGET_ADD+5,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectNULL = { TYPE_POINTER, NULL, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation").andReturnValue(&burstObjectNULL);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectNULL);
	//	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);



}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_BroadcastGroupCallInvComeInCALHANG)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_BROADCAST_GROUP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_CallInvitation *p_GrantEvent2 = new PH_CallInvitation(SOURCE_SITE_ID, 100, SOURCE_ADD, TARGET_ADD, 0, 
		PH_CallType::PHYRCE_BROADCAST_GROUP_VOICE_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_BROADCAST_GROUP_VOICE_CALL), CALL_START,NULL);

	//	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+5,TARGET_ADD+5,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectNULL = { TYPE_POINTER, NULL, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation").andReturnValue(&burstObjectNULL);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectNULL);
	//	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);



}
TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_2LocalGroupComeInCALHANG)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_CallInvitation *p_GrantEvent2 = new PH_CallInvitation(SOURCE_SITE_ID, 100, SOURCE_ADD, TARGET_ADD, 0, 
		PH_CallType::PHYRCE_GRP_VOICE_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_BROADCAST_GROUP_VOICE_CALL), CALL_START,NULL);

		PH_CallInvitation *p_GrantEvent3 = new PH_CallInvitation(SOURCE_SITE_ID, 100, SOURCE_ADD+1, TARGET_ADD, 0, 
		PH_CallType::PHYRCE_GRP_VOICE_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_BROADCAST_GROUP_VOICE_CALL), CALL_START,NULL);

	//	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+5,TARGET_ADD+5,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectNULL = { TYPE_POINTER, NULL, 0, 0, 0,"" };

		sBurstObjectType burstObjectGrant3= { TYPE_POINTER, p_GrantEvent3, 0, 0, 0,"" };


	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation").andReturnValue(&burstObjectNULL);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant3);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectNULL);


	p_floorArb->invokeTrgViaMap(pBurstMapMock);



}
TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_ValidPMaintComeInCALHANG)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO, 3);

	sBurstObjectType burstObjectGrant= { TYPE_POINTER, p_GrantEvent, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	//Update to local_call_hang
	mock("PH_MediatorBurstMap").expectOneCall("getSlotState").andReturnValue(F2_CALLAPP_CALL_HANG);
	p_floorArb ->syncSlotState(pBurstMapMock);

	PH_CallInvitation *p_GrantEvent2 = new PH_CallInvitation(SOURCE_SITE_ID, 100, SOURCE_ADD, 0xFFFECA, 0, 
		PH_CallType::PHYRCE_P_MAINT_CALL, SLOT_ONE, SLOT_ONE, 
		PH_CallType::getCallPriorityByType(PH_CallType::PHYRCE_P_MAINT_CALL), CALL_START,NULL);

	//	PH_FloorArbitrationResult *expectedFloorResult = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD+5,TARGET_ADD+5,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	sBurstObjectType burstObjectNULL = { TYPE_POINTER, NULL, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation").andReturnValue(&burstObjectNULL);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectNULL);
	//	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);


	//	delete expectedFloorResult;
}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_2SameGrantComeinCallHang)
{
	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	p_GrantEvent1->updateCallSeqNum(1);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO1);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	
	p_floorArb->invokeTrgViaMap(pBurstMapMock);


	PH_AbstractBurstObject * p_GrantBO2 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD+1, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject * p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD+1,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2, 3);
	PH_FloorArbitrationResult * expectedFloorResult2 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD+1,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	p_GrantEvent2->updateCallSeqNum(2);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	//mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult2);
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO2);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");


	PH_ArbFail* expectedFail = new PH_ArbFail(ARB_PREEMPTION,SOURCE_ADD,TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,0,SLOT_ONE);
	std::list<PH_ArbFail*>* failList = new std::list<PH_ArbFail*> ;
	failList->push_back(expectedFail);
	PH_ArbFailListEvent* expectFailList = new PH_ArbFailListEvent(failList,SLOT_ONE);
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO",expectFailList);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	p_floorArb ->handleFailEvent(pBurstMapMock);

	delete expectedFloorResult1;
	delete expectedFloorResult2;
	delete expectFailList;
}
TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_GrantANDClearComeinCallHangSameTime)
{
	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	PH_ControlInvitation *p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+1, TARGET_ADD+1,PH_CallType::PHYRCE_P_CLEAR_CALL,SLOT_ONE, SLOT_ONE, NULL, 3);
	p_GrantEvent2->setOrigin(FROM_OTA);
	p_GrantEvent2->setCtrlOrigin(CTRL_FROM_OTA);	
	p_GrantEvent2->setControlInvitationType(P_CLEAR_INVITATION);
	PH_FloorArbitrationResult * expectedFloorResult2 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD+1,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO1);
	mock("PH_MediatorBurstMap").expectNCalls(2,"popOutCallInvitation");
	
	p_floorArb->invokeTrgViaMap(pBurstMapMock);

	delete expectedFloorResult2;
}

TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_GrantState_ComeinSameCSNGrant)
{

	bool ret = false;
	// update m_floorOWner according to grant
	PH_AbstractBurstObject *p_GrantBO1 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject *p_GrantEvent1 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO1, 3);
	PH_FloorArbitrationResult *expectedFloorResult1 = new PH_FloorArbitrationResult(CHANNEL_ID,SOURCE_ADD,TARGET_ADD,0,0,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE,1,0);
	p_GrantEvent1->updateCallSeqNum(1);
	sBurstObjectType burstObjectGrant1= { TYPE_POINTER, p_GrantEvent1, 0, 0, 0,"" };

	mock("PH_MediatorBurstMap").expectOneCall("getRepeaterState").andReturnValue(F2_CP_HANGTIME);
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant1);
	mock("PH_MediatorBurstMap").expectNCalls(1,"popOutCallInvitation");
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO", expectedFloorResult1);
	mock("PH_MediatorBurstMap").expectOneCall("overwriteData").withParameterOfType("PH_AbstractBurstObject*","pABO", p_GrantBO1);
	p_floorArb->invokeTrgViaMap(pBurstMapMock);


	PH_AbstractBurstObject * p_GrantBO2 = new P_Grant(SLOT_ONE, CHANNEL_ID, TV_GRANT , TARGET_ADD, SOURCE_ADD+1, 1, TARGET_SITE_ID, PH_CallType::PHYRCE_GRP_VOICE_CALL, true);
	PH_AbstractBurstObject * p_GrantEvent2 = new PH_ControlInvitation(SOURCE_SITE_ID, SOURCE_ADD+1, TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,SLOT_ONE, SLOT_ONE, p_GrantBO2, 3);

	p_GrantEvent2->updateCallSeqNum(1);
	sBurstObjectType burstObjectGrant2= { TYPE_POINTER, p_GrantEvent2, 0, 0, 0,"" };
	mock("PH_MediatorBurstMap").expectOneCall("popOutCallInvitation").andReturnValue(&burstObjectGrant2);
	mock("PH_MediatorBurstMap").expectNCalls(3,"popOutCallInvitation");

	PH_ArbFail* expectedFail = new PH_ArbFail(ARB_SLOTBUSY,SOURCE_ADD+1,TARGET_ADD,PH_CallType::PHYRCE_GRP_VOICE_CALL,0,SLOT_ONE);
	std::list<PH_ArbFail*>* failList = new std::list<PH_ArbFail*> ;
	failList->push_back(expectedFail);
	PH_ArbFailListEvent* expectFailList = new PH_ArbFailListEvent(failList,SLOT_ONE);
	mock("PH_MediatorBurstMap").expectOneCall("addBurstToMap").withParameterOfType("PH_AbstractBurstObject*","pABO",expectFailList);

	p_floorArb->invokeTrgViaMap(pBurstMapMock);
	p_floorArb ->handleFailEvent(pBurstMapMock);

	delete expectedFloorResult1;
	delete expectFailList;

}
TEST(EmeraldFloorArbitrationUT, invokeTrgViaMap_IDLEState_multipleGrant)
{

}

