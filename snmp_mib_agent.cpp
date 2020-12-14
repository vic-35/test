/*
++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2004 S&M&A

Module Name:

    vic_snmp.cpp

Abstract:

    M2 SNMP Extension Agent for Windows NT.

*/

#include "stdafx.h"



// The prefix to all of these MIB variables is .1.3.6.1.4.1.19234.5.

UINT OID_Prefix[] = { 1,3,6,1,4,1,19234};
AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };



// OID 	  
// Это пример реальных данных пока не выдано заказчиком !

UINT MIB_v_Manufacturer[]       = { 5, 1};
UINT MIB_v_Manufacturer1[]      = { 5, 2};
UINT MIB_v_d1[]				    = { 5, 3, 2};
UINT MIB_v_d2[]                 = { 5, 5 };


//  Массив значений !
// 

char       MIB_v_t1[]     = "My Data ";
char       MIB_v_ManStor[]     = "M2 Monitor\0";
char       MIB_v_ModelStor[]   = "M2 SNMP Extension Agent for Windows NT/2000\0";


//  Массив доступа к значеням !

MIB_ENTRY Mib[] = {      

      { { OID_SIZEOF(MIB_v_Manufacturer), MIB_v_Manufacturer },
        &MIB_v_ManStor, ASN_RFC1213_DISPSTRING,
		MIB_ACCESS_READ, MIB_leaf_func, &Mib[1] },

      { { OID_SIZEOF(MIB_v_Manufacturer1), MIB_v_Manufacturer1 },
        &MIB_v_ModelStor, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_leaf_func,&Mib[2] },

	  { { OID_SIZEOF(MIB_v_d1), MIB_v_d1 },
        &MIB_v_t1, ASN_RFC1213_DISPSTRING,
		MIB_ACCESS_READ, MIB_leaf_func,  &Mib[3] },
	  
	  { { OID_SIZEOF(MIB_v_d2), MIB_v_d2 },
        &MIB_v_t1, ASN_RFC1213_DISPSTRING,
		MIB_ACCESS_READ, MIB_leaf_func,  NULL }
      };

UINT MIB_num_variables = sizeof Mib / sizeof( MIB_ENTRY );



//
// ResolveVarBind
//    Resolves a single variable binding.  Modifies the variable on a GET
//    or a GET-NEXT.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind, // Variable Binding to resolve
	IN UINT PduAction               // Action specified in PDU
	)

{
MIB_ENTRY            *MibPtr;
AsnObjectIdentifier  TempOid;
int                  CompResult;
UINT                 I;
UINT                 nResult;


   // Search for var bind name in the MIB
   I      = 0;
   MibPtr = NULL;
   while ( MibPtr == NULL && I < MIB_num_variables )
      {
      // Construct OID with complete prefix for comparison purposes
      SnmpUtilOidCpy( &TempOid, &MIB_OidPrefix );
      SnmpUtilOidAppend( &TempOid, &Mib[I].Oid );

      // Check for OID in MIB - On a GET-NEXT the OID does not have to exactly
      // match a variable in the MIB, it must only fall under the MIB root.
      CompResult = SnmpUtilOidCmp( &VarBind->name, &TempOid );
      if ( 0 > CompResult )
	 {
	 // Since there is not an exact match, the only valid action is GET-NEXT
	 if ( MIB_ACTION_GETNEXT != PduAction )
	    {
	    nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

	 // Since the match was not exact, but var bind name is within MIB,
	 // we are at the NEXT MIB variable down from the one specified.
	 PduAction = MIB_ACTION_GET;
	 MibPtr = &Mib[I];

         // Replace var bind name with new name
         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MibPtr->Oid );
	 }
      else
         {
	 // An exact match was found.
         if ( 0 == CompResult )
            {
	    MibPtr = &Mib[I];
	    }
	 }

      // Free OID memory before checking another variable
      SnmpUtilOidFree( &TempOid );

      I++;
      } // while

   // If OID not within scope of MIB, then no such name
   if ( MibPtr == NULL )
      {
      nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Call function to process request.  Each MIB entry has a function pointer
   // that knows how to process its MIB variable.
   nResult = (*MibPtr->MibFunc)( PduAction, MibPtr, VarBind );

   // Free temp memory
   SnmpUtilOidFree( &TempOid );

Exit:
   return nResult;
} // ResolveVarBind



//
// MIB_leaf_func
//    Performs generic actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	)

{
UINT   ErrStat=0;

switch ( Action )
      {
      case MIB_ACTION_GETNEXT:
	 
	 // If there is no GET-NEXT pointer, this is the end of this MIB
	 if ( MibPtr->MibNext == NULL )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
		goto Exit;
	    }

         //  NEXT MIB variable
         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MibPtr->MibNext->Oid );

 		
         ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GET,
	                                        MibPtr->MibNext, VarBind );
        break;

      case MIB_ACTION_GET:
         // Make sure that this variable's ACCESS is GET'able
	 if ( MibPtr->Access != MIB_ACCESS_READ &&
	      MibPtr->Access != MIB_ACCESS_READWRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }
	 // Setup varbind's return value
	 VarBind->value.asnType = MibPtr->Type;
	 switch ( VarBind->value.asnType )
	    {
            case ASN_RFC1155_COUNTER:
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               VarBind->value.asnValue.number = *(AsnInteger *)(MibPtr->Storage);
	       break;

            case ASN_OCTETSTRING: // This entails ASN_RFC1213_DISPSTRING also
	       VarBind->value.asnValue.string.length =
                                 strlen( (LPSTR)MibPtr->Storage );

	       if ( NULL == 
                    (VarBind->value.asnValue.string.stream = (unsigned char *)SnmpUtilMemAlloc(  VarBind->value.asnValue.string.length * sizeof(char))) )
	          {
				ErrStat = SNMP_ERRORSTATUS_GENERR;
				//MessageBeep(   MB_ICONEXCLAMATION);
				goto Exit;
	          }

	       memcpy( VarBind->value.asnValue.string.stream,
	               (LPSTR)MibPtr->Storage,
	               VarBind->value.asnValue.string.length );
	       VarBind->value.asnValue.string.dynamic = TRUE;

	       break;

	    default:
	       ErrStat = SNMP_ERRORSTATUS_GENERR;
	       goto Exit;
	    }

	 break;
      default:
	 ErrStat = SNMP_ERRORSTATUS_GENERR;
	 goto Exit;
      } // switch

   // Signal no error occurred
   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
  return ErrStat;
} 


