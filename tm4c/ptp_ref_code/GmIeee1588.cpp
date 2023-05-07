#ifndef _GEO_APPDEFS_HPP
#include <GmCommon/GeoAppDefs.hpp>
#endif

#include "stdio.h"
#include "math.h"
#include "string.h"
#include <time.h>

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

// For time-keeping.
#include  <ti/sysbios/hal/Seconds.h>

/* RTOS Driver Header files */
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SDSPI.h>

/* Tivaware Header files */
#include <inc/hw_memmap.h>
#include <driverlib/adc.h>
#if GEO_NET_NDK == GEO_TRUE
#include <ti/ndk/inc/netmain.h>
#endif

/* NDK BSD support */
//#include <sys/socket.h>

#if GEO_NET_3100 == GEO_TRUE
#include <simplelink.h>
// TODO RHC not good place for those files.  That library
//  needs to be moved out and into this one.
#include "http-port\usertype.h"
#endif
#include <driverlib/emac.h>

#include <driverlib/rom.h>
#include "GmIeee1588.hpp"


using namespace Geometrics;

#define GM_COMM_OK 0
#define GM_COMM_FAIL 1

#if GEO_1588 == GEO_TRUE

// Port ID includes clock ID and port number/id
U8 sPortId[10];


Ieee1588Listener::Ieee1588Listener ()
{
}

Ieee1588Listener::~Ieee1588Listener ()
{
}

uint32_t  s_fineAddend = 0x80000000;
uint32_t  s_lastOffsetPeriod = 1;
volatile uint32_t s_rxTimestampNanoseconds = 0, s_rxTimestampSeconds = 0;
volatile uint32_t s_txTimestampNanoseconds= 0, s_txTimestampSeconds = 0;

long long s_diffRxSeconds,s_diffRxNanoseconds,s_diffTxSeconds,s_diffTxNanoseconds;

extern "C" void SetLocal1588 (uint32_t lo,uint32_t seconds)
{
	s_rxTimestampNanoseconds = lo;
	s_rxTimestampSeconds = seconds;
}

extern "C" void SetLocal1588Tx (uint32_t lo,uint32_t seconds)
{

	//uint32_t secondsBefore,secondsAfter,subBefore,subAfter;
	//EMACTimestampSysTimeGet(EMAC0_BASE,&secondsBefore,&subBefore);

	s_txTimestampNanoseconds = lo;
	s_txTimestampSeconds = seconds;
}

U16 s_requestSequenceId = 12000;
int s_countSynchs = 0;

#define SUBSECONDS_PER_SECOND 1000000000


// Subtract some amount from a timestamp.  The result must be a valid timestamp, i.e. > 0.
Bool AdjustTimestamp (UINT32 originalSeconds,UINT32 originalNanoseconds,UINT32 diffSeconds,UINT32 diffNanoseconds,
		UINT32 *answerSeconds, UINT32 *answerNanoseconds)
{
	long long resultSeconds, resultNanoseconds;

	resultSeconds = (long long)originalSeconds - (long long)diffSeconds;
	resultNanoseconds = (long long)originalNanoseconds - (long long)diffNanoseconds;

   if (diffSeconds < 0)
   {
	   return false;
   }

   if (resultSeconds > 0)
   {
	   if (resultNanoseconds < 0)
	   {
		   resultSeconds--;
		   resultNanoseconds = SUBSECONDS_PER_SECOND + resultNanoseconds;
	   }
   }
   else  // diffSeconds == 0
   {
	   if (resultNanoseconds < 0)
	   {
		   return false;
	   }
   }
   *answerSeconds = resultSeconds;
   *answerNanoseconds = resultNanoseconds;
   return true;
}

// Subtract one timestamp from another, getting a difference (i.e. not a timestamp).
void SubtractTimestamp (UINT32 originalSeconds,UINT32 originalNanoseconds,UINT32 diffSeconds,UINT32 diffNanoseconds,
		                long long *answerSeconds, long long *answerNanoseconds)
{
	long long resultSeconds, resultNanoseconds;

	resultSeconds = (long long)originalSeconds - (long long)diffSeconds;
	resultNanoseconds = (long long)originalNanoseconds - (long long)diffNanoseconds;

   if (resultSeconds < 0)
   {
	   if (resultNanoseconds > 0)
	   {
		   resultSeconds++;
		   resultNanoseconds = -(SUBSECONDS_PER_SECOND - resultNanoseconds);
	   }

   }
   else if (resultSeconds > 0)
   {
	   if (resultNanoseconds < 0)
	   {
		   resultSeconds--;
		   resultNanoseconds = SUBSECONDS_PER_SECOND + resultNanoseconds;
	   }
   }
   *answerSeconds = resultSeconds;
   *answerNanoseconds = resultNanoseconds;
}


int Ieee1588Process( SOCKET Sock, UINT32 unused )
{
   struct ptp_header header;
   struct ptp_sync   sync;
   struct ptp_delay_resp  response;
   struct ptp_delay_req  delayRequest;

   U8                 msgType;
   //socklen_t          addrlen;
   int addrlen;

   UINT32 low, high;
   struct sockaddr_in clientAddr;
   int bytesSent;

   UINT32 seconds, nanoseconds;

   int bytesRcvd = recv (Sock, (void *)&msgType, sizeof(msgType), MSG_PEEK);

   if (bytesRcvd == sizeof(msgType))
   {
	   switch (msgType)
	   {
	   	   case 0: // synch
		   bytesRcvd = recvfrom(Sock, (void *)&sync, sizeof(sync), 0,
								   (struct sockaddr *)&clientAddr, &addrlen);
		   s_countSynchs++;

		   if (bytesRcvd == sizeof(sync))
		   {
			   // TODO RHC get the epoch in here, i.e. the upper 16 bits.  Maybe?
			   seconds     = ntohl(*((uint32_t*)(sync.origin_tstamp+2)));
			   nanoseconds = ntohl(*((uint32_t*)(sync.origin_tstamp+6)));

			   s_diffRxSeconds     = seconds - s_rxTimestampSeconds;
			   s_diffRxNanoseconds = (long long)nanoseconds - (long long)s_rxTimestampNanoseconds;

			   sync.hdr.msg_type = 1;

			   struct  sockaddr_in sin1;

			    /* Prepare address for connect */
			    bzero( &sin1, sizeof(struct sockaddr_in) );
			    sin1.sin_family      = AF_INET;
                sin1.sin_addr.s_addr = inet_addr("224.0.1.129");
			    sin1.sin_port        = htons(319);
			    bzero(&sync.origin_tstamp,10);
			    memcpy((void*)&sync.hdr.src_port_id,(void*)&sPortId,10);

			    s_requestSequenceId+= 10;
			    sync.hdr.seq_id = s_requestSequenceId;


			   bytesSent = sendto(Sock, (void*)&sync, sizeof(sync), 0,
					   (struct sockaddr *)&sin1, sizeof(sin1) );

			   if (bytesSent < 0 || bytesSent != bytesRcvd) {

			   }
		   }
		   break;

		   case 9: // Delay response.
			   bytesRcvd = recvfrom(Sock, (void *)&response, sizeof(response), 0,
									   (struct sockaddr *)&clientAddr, &addrlen);
			   seconds = *((uint32_t*)(response.recv_tstamp+2));
			   nanoseconds = *((uint32_t*)(response.recv_tstamp+6));

			   seconds 	   = ntohl(seconds);
			   nanoseconds = ntohl(nanoseconds);

			   // Find the difference between local time and master time, not yet adjusted for the travel
			   //    time.  A negative difference means that the master clock says later than the local.
			   SubtractTimestamp (s_txTimestampSeconds,s_txTimestampNanoseconds,seconds,nanoseconds,
					              &s_diffTxSeconds, &s_diffTxNanoseconds);

			   signed long long bigSeconds = (signed long long)seconds - (signed long long)s_txTimestampSeconds;
			   signed long long bigNanoseconds = (signed long long)nanoseconds - (signed long long)s_txTimestampNanoseconds;
			   if (bigSeconds < 0)
			   {
				   bigSeconds *= -1;
			   }

			   // If we're way off, then jam in a big correction.
			   if (bigSeconds > 1)
			   {
				   EMACTimestampSysTimeSet(EMAC0_BASE,seconds,nanoseconds);
				   break;
			   }


			   Bool bAdd = true;

			   long long travelSeconds, travelNanoseconds;

			   travelSeconds = (s_diffTxSeconds + s_diffRxSeconds) / 2;
			   travelNanoseconds = (s_diffTxNanoseconds + s_diffRxNanoseconds) / 2;

			   if (travelSeconds < 0)
			   {
				   if (travelNanoseconds < 0)
				   {
				   }
				   else if (travelNanoseconds > 0)
				   {
					   travelSeconds++;
					   travelNanoseconds = -(SUBSECONDS_PER_SECOND - travelNanoseconds);
				   }
			   }
			   else if (travelSeconds > 0)
			   {
				   if (travelNanoseconds < 0)
				   {
					   travelSeconds--;
					   travelNanoseconds = SUBSECONDS_PER_SECOND + travelNanoseconds;
				   }
			   }

			   long long realDifferenceSeconds, realDifferenceNanoseconds;

			   realDifferenceSeconds = s_diffTxSeconds + travelSeconds;
			   realDifferenceNanoseconds = s_diffTxNanoseconds + travelNanoseconds;
			   U16 responseSequence = response.hdr.seq_id;
			   U16 lastRequestId    = s_requestSequenceId;
			   {
				   int nanosOffPerSecond = realDifferenceNanoseconds / s_lastOffsetPeriod;
				   if (nanosOffPerSecond < 0)
				   {
					   realDifferenceNanoseconds *= -1;
					   bAdd = true;
				   }
				   else
					   bAdd = false;
//
				   // Get the inverse of the drift ratio.
				   long long addend = (long long)s_fineAddend * (long long) (SUBSECONDS_PER_SECOND - nanosOffPerSecond);
				   addend = addend / SUBSECONDS_PER_SECOND;


				   s_fineAddend = addend;

				   EMACTimestampAddendSet(EMAC0_BASE,s_fineAddend);

				   EMACTimestampSysTimeUpdate (EMAC0_BASE,
											   //bigSeconds,
											   realDifferenceSeconds,
											   realDifferenceNanoseconds,
											   bAdd);
			   }
			   break;

	   default:
		   bytesRcvd = recvfrom(Sock, (void *)&response, sizeof(response), 0,
								   (struct sockaddr *)&clientAddr, &addrlen);
		   low = *((uint32_t*)(response.recv_tstamp+2));
		   high = *((uint32_t*)(response.recv_tstamp+6));

		   low = ntohl(low);
		   high = ntohl(high);
		   break;
	   }
   }

   CI_IPNET        NA;
   if (CfgGetImmediate( 0, CFGTAG_IPNET, 1, 1, sizeof(NA), (UINT8 *)&NA) == sizeof(NA))
   {
	   struct ip_mreq     group;
	   group.imr_multiaddr.s_addr = inet_addr("224.0.1.129");
	   group.imr_interface.s_addr = NA.IPAddr;

	   setsockopt (Sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)&group, sizeof(group));
   }



   fdClose(Sock);
   return 0;
   //return 1;
}

extern "C" HANDLE DaemonNew( uint Type, IPN LocalAddress, uint LocalPort, int (*pCb)(SOCKET,UINT32),
                  uint Priority, uint StackSize, UINT32 Argument, uint MaxSpawn );



// Start up...
int Ieee1588Listener::Start1588Listener (uint32_t ulUser0,uint32_t ulUser1)
{
	// First, clock ID, which by default is MAC address with 0xfffe inserted in the middle.
	sPortId[0] = ((ulUser0 >>  0) & 0xff);
	sPortId[1] = ((ulUser0 >>  8) & 0xff);
	sPortId[2] = ((ulUser0 >> 16) & 0xff);
	sPortId[3] = 0xff;
	sPortId[4] = 0xfe;
	sPortId[5] = ((ulUser1 >>  0) & 0xff);
	sPortId[6] = ((ulUser1 >>  8) & 0xff);
	sPortId[7] = ((ulUser1 >> 16) & 0xff);

	// Port number is the last 2 bytes.
	sPortId[8]  = 0;
	sPortId[9]  = 0x01;

	   uint32_t uTest,
	            uTest2;

	uTest = EMACStatusGet(EMAC0_BASE);
	uTest = EMACTimestampConfigGet(EMAC0_BASE, &uTest2);
	uTest2 = //EMAC_TS_PTP_VERSION_1 |
		   EMAC_TS_PTP_VERSION_2 |
		 EMAC_TS_DIGITAL_ROLLOVER |
		// EMAC_TS_UPDATE_COARSE |
		 EMAC_TS_UPDATE_FINE |
		 EMAC_TS_PROCESS_IPV4_UDP; // |
	   //  EMAC_TS_ALL |
	   //  EMAC_TS_ALL_RX_FRAMES;
	EMACTimestampConfigSet (EMAC0_BASE,
		 uTest2,
		 //40);
	 80);

	EMACTimestampAddendSet(EMAC0_BASE,s_fineAddend);

	uTest = EMACTimestampConfigGet(EMAC0_BASE, &uTest2);
	EMACTimestampEnable(EMAC0_BASE);

	// Set the daemons.  We will use the same service routine for both ports;
	//    We can sort out what message it is, in the routine.

   DaemonNew( SOCK_DGRAM, INADDR_ANY, 319, &Ieee1588Process,
                  4, 4096, 0, 3 );
   DaemonNew( SOCK_DGRAM, INADDR_ANY, 320, &Ieee1588Process /*&Ieee1588General*/,
                  4, 4096, 0, 3 );
   return 0;
}

#endif
