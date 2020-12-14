#include "winsock.h"     // Winsock header file

#define PROG_NAME "A Simple Ping Program"

#define HOST_NAME "ya.ru"  // This can be any valid host name
#define WINSOCK_VERSION 0x0101  // Program requires Winsock version 1.1
#define NO_FLAGS 0        // No special flags specified
					  // RFC 792 defines ICMP message values
#define ICMP_ECHO 8        // An ICMP echo message
#define ICMP_ECHOREPLY 0     // An ICMP echo reply message
#define ICMP_HEADERSIZE 8     // ICMP header size ("echo messages" only)

struct ip             // Structure for IP datagram header
 {
  BYTE ip_verlen;        // Version and header length
  BYTE ip_tos;         // Type of service
  WORD ip_len;         // Total packet length
  UINT ip_id;          // Datagram identification
  WORD ip_fragoff;       // Fragment offset
  BYTE ip_ttl;         // Time to live
  BYTE ip_proto;        // Protocol
  UINT ip_chksum;        // Checksum
  IN_ADDR ip_src_addr;     // Source address
  IN_ADDR ip_dst_addr;     // Destination address
  BYTE ip_data[1];       // Variable length data area
 };

struct icmp            // Structure for an ICMP header
 {
  BYTE icmp_type;        // Type of message
  BYTE icmp_code;        // Type "sub code" (zero for echos)
  WORD icmp_cksum;       // 1's complement checksum
  HINSTANCE icmp_id;         // Unique ID (the instance handle)
  WORD icmp_seq;        // Tracks multiple pings
  BYTE icmp_data[1];      // The start of optional data
 };

char szPingBuffer[100];      // General purpose buffer for messages

WORD InternetChksum(LPWORD lpwIcmpData, WORD wDataLength)
 {
  long lSum;    // Store the summation
  WORD wOddByte;  // Left over byte from the summation
  WORD wAnswer;  // The 1's complement checksum

  lSum = 0L;

  while (wDataLength > 1)
	{
	 lSum += *lpwIcmpData++;
	 wDataLength -= 2;
	}

  // Handle the odd byte if necessary and make sure the top half is zero
  if (wDataLength == 1)
	{
	 wOddByte = 0;
	 *((LPBYTE) &wOddByte) = *(LPBYTE)lpwIcmpData; // One byte only
	 lSum += wOddByte;
	}

  // Add back the carry outs from the 16 bits to the low 16 bits
  lSum = (lSum >> 16) + (lSum & 0xffff); // Add high-16 to low-16
  lSum += (lSum >> 16);          // Add carry
  wAnswer = (WORD)~lSum;         // 1's complement, then truncate
							 // to 16 bits
  return(wAnswer);
 }

BOOL DoPingOperation(HANDLE hInstance)
 {
					  // Local variables
  int iPacketSize;       // ICMP packet size
  int iHostAddrLength;     // Host address length
  int iIPHeadLength;      // IP datagram header length
  int iReceivedBytes;      // Number of bytes received
  int iSentBytes;        // Number of bytes sent
  int nProtocol;        // ICMP protocol number
  int iSocketError;       // Stores any error codes
  PDWORD pdwTimeStamp;     // Tick count at transmission
  DWORD dwReturnTime;      // Tick count upon receipt of echo reply
  DWORD dwRoundTrip;      // Tick count for the round-trip
					  // Structures defined in WINSOCK.H
  SOCKADDR_IN sockAddrLocal;  // Socket address structures
  SOCKADDR_IN sockAddrHost;   //
  SOCKET hSocket;        // Socket handle (or descriptor)
  LPHOSTENT lpHostEntry;    // Internet host data structure
  LPPROTOENT lpProtocolEntry;  // Internet protocol data structure

  BYTE IcmpSendPacket[1024];  // Buffer space for data to send
  BYTE IcmpRecvPacket[4096];  // Buffer space for received data

  struct icmp *pIcmpHeader;   // A pointer to the ICMP structure
  struct ip *pIpHeader;     // A pointer to the IP header structure
  LPSTR lpszHostName;      // A pointer to the time server host

  lpszHostName = HOST_NAME;

  if ((lpHostEntry = gethostbyname(HOST_NAME)) == NULL)
	{
	 wsprintf(szPingBuffer, "Could not get %s IP address.",
		 (LPSTR)lpszHostName);
	 return(FALSE);
	}

  sockAddrLocal.sin_family = AF_INET;
  sockAddrLocal.sin_addr = *((LPIN_ADDR) *lpHostEntry->h_addr_list);

  // With a raw socket, the program must specify a protocol
  if ((lpProtocolEntry = getprotobyname("icmp")) == NULL)
	nProtocol = IPPROTO_ICMP;
  else
	nProtocol = lpProtocolEntry->p_proto;

  // Create a "raw" socket and specify ICMP as the protocol to use
  //if ((hSocket = socket(PF_INET, SOCK_RAW, nProtocol)) ==
  //   INVALID_SOCKET)
  if ((hSocket = socket(AF_INET, SOCK_RAW,IPPROTO_ICMP)) ==
	  INVALID_SOCKET)
	{
	 wsprintf(szPingBuffer, "Could not create a RAW socket.");
	 return(FALSE);
   }

  pIcmpHeader = (struct icmp *) IcmpSendPacket; // Point at the data area
  pIcmpHeader->icmp_type = ICMP_ECHO;      // then fill in the data.
  pIcmpHeader->icmp_code = 0;          // Use the Sockman instance
  pIcmpHeader->icmp_id = (HINSTANCE) hInstance;       // handle as a unique ID.
  pIcmpHeader->icmp_seq = 0;          // It's important to reset
  pIcmpHeader->icmp_cksum = 0;         // the checksum to zero.

  //Put tick count in the optional data area
  pdwTimeStamp = (PDWORD)&IcmpSendPacket[ICMP_HEADERSIZE];
  *pdwTimeStamp = GetTickCount();
  iPacketSize = ICMP_HEADERSIZE + sizeof(DWORD);
  pIcmpHeader->icmp_cksum = InternetChksum((LPWORD)pIcmpHeader,
     iPacketSize);

  if (pIcmpHeader->icmp_cksum !=0 )
   {
    iSentBytes = sendto(hSocket, (LPSTR) IcmpSendPacket, iPacketSize,
       NO_FLAGS, (LPSOCKADDR) &sockAddrLocal, sizeof(sockAddrLocal));
    if (iSentBytes == SOCKET_ERROR)
     {
      closesocket(hSocket);
      wsprintf(szPingBuffer,
         "The sendto() function returned a socket error.");
      return(FALSE);
     }

    if (iSentBytes != iPacketSize)
     {
      closesocket(hSocket);
      wsprintf(szPingBuffer,
         "Wrong number of bytes sent: %d", iSentBytes);
      return(FALSE);
     }

    iHostAddrLength = sizeof(sockAddrHost);

    iReceivedBytes = recvfrom(hSocket, (LPSTR) IcmpRecvPacket,
       sizeof(IcmpRecvPacket), NO_FLAGS, (LPSOCKADDR) &sockAddrHost,
       &iHostAddrLength);
   }
  else
   {
    closesocket(hSocket);
    wsprintf(szPingBuffer,
       "Checksum computation error! Result was zero!");
    return(FALSE);
   }

  closesocket(hSocket);

  if (iReceivedBytes == SOCKET_ERROR)
   {
    iSocketError = WSAGetLastError();
    if (iSocketError == 10004)
     {
      wsprintf(szPingBuffer,
         "Ping operation for %s was cancelled.",
         (LPSTR)lpszHostName);
      dwRoundTrip = 0;
      return(TRUE);
     }
    else
     {
      wsprintf(szPingBuffer,
         "Socket Error from recvfrom(): %d", iSocketError);
      return(FALSE);
     }
   }

  dwReturnTime = GetTickCount();
  dwRoundTrip = dwReturnTime - *pdwTimeStamp;

  // Point to the IP Header in the received packet
  pIpHeader = (struct ip *)IcmpRecvPacket;

  // Extract bits 4-7 and convert the number of 32-bit words to bytes
  iIPHeadLength = (pIpHeader->ip_verlen >> 4) << 2;

  // Test the length to make sure an ICMP header was received
  if (iReceivedBytes < iIPHeadLength + ICMP_HEADERSIZE)
   {
    wsprintf(szPingBuffer, "Received packet was too short.");
    return(FALSE);
   }

  // Point to the ICMP message which immediately follows the IP header
  pIcmpHeader = (struct icmp *) (IcmpRecvPacket + iIPHeadLength);

  // Make sure this is an ICMP "echo reply"
  //if (pIcmpHeader->icmp_type != ICMP_ECHOREPLY)
  // {
  //  wsprintf(szPingBuffer,
  //     "Received packet was not an echo reply to your ping.");
  //  return(FALSE);
  // }

  //// Make sure this program sent the packet
  //if (pIcmpHeader->icmp_id != hInstance)
  // {
  //  wsprintf(szPingBuffer,
  //     "Received packet was not sent by this program.");
  //  return(FALSE);
  // }

  // This appears to be a matching reply. Note the IP address and
  // host name that sent the ICMP echo reply message.
  lstrcpy(lpszHostName, (LPSTR)lpHostEntry->h_name);
  wsprintf(szPingBuffer,
     "Round-trip travel time to %s [%s] was %d milliseconds.",
     (LPSTR)lpszHostName, (LPSTR)inet_ntoa(sockAddrHost.sin_addr),
     dwRoundTrip);

  return(TRUE);
 }

//int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
//   LPSTR lpszCmdParam, int nCmdShow)

   int main() 
{
  
  HANDLE hInstance=GetModuleHandle("pn.exe");
  WSADATA wsaData;       // Winsock implementation details

  // Start the program here
  WSAStartup(WINSOCK_VERSION, &wsaData);

  DoPingOperation(hInstance);
  MessageBox(NULL, szPingBuffer, PROG_NAME, MB_OK|MB_ICONSTOP);

  WSACleanup();
  return(NULL);
 }