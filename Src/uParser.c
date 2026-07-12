/**
 * @file       uParser.c
 * @brief      A minimalistic library which implements common logic part
 *             of every byte stream parser - finding defined structure
  *            in byte stream.
 * 
 * @details    .
 * 
 * @author     Adam Smerda
 * @date       2026-05-24
 * @version    1.0
 * @copyright  Copyright (c) 2026, AdamS-0
 * @license    LGPL-2.1
 */

/* Includes ----------------------------------------------------------- */
#include "uParser.h"


/* Private variables -------------------------------------------------- */


/* Private functions -------------------------------------------------- */
void _uParser_resetCntrs(uParser_t *pUParser) {
    if ( pUParser == NULL ) { return; }

    pUParser->cntrs.dataMaxOverflow = 0;
    pUParser->cntrs.foundSyncStart  = 0;
    pUParser->cntrs.foundSyncStop   = 0;
    pUParser->cntrs.foundHeader     = 0;
    pUParser->cntrs.foundPayload    = 0;
    pUParser->cntrs.bytesSkipped    = 0;
}

uParser_regStatus_e _uParser_registerCommon(
    uParser_t               *pUParser,
    uint8_t                 *pData,
    uint16_t                dataMaxLen,
    uint8_t                 *pSeqStart,
    uint16_t                seqStartLen
    )
{
    if ( pUParser       == NULL ) { return uParser_regStatus_ERROR; }
    if ( pData          == NULL ) { return uParser_regStatus_ERROR; }
    if ( pSeqStart      == NULL ) { return uParser_regStatus_ERROR; }
    
    if ( dataMaxLen     == 0 ) { return uParser_regStatus_ERROR; }
    if ( seqStartLen    == 0 ) { return uParser_regStatus_ERROR; }
    
    pUParser->pData                 = pData;
    pUParser->dataMaxLength         = dataMaxLen;
    pUParser->pSeqStart             = pSeqStart;
    pUParser->seqStartLen           = seqStartLen;
    
    _uParser_resetCntrs( pUParser );
    
    return uParser_regStatus_OK;
}

uParser_status_e _uParser_parseByLen(uParser_t *pUParser, uint8_t byteIn) {
    if ( pUParser == NULL ) { return uParser_status_INTERNAL_ERROR; }
    
    if      ( pUParser->state == uParser_state_LOOKING_SYNC_START   ) {
        /* Looking for sync sequence */
        
        if ( pUParser->seqStartIdx >= pUParser->seqStartLen ) {
            /* This case should never be reached */
            pUParser->state = uParser_state_IDLE;
            return uParser_status_LOOKING;
        }
        
        if ( pUParser->pSeqStart[pUParser->seqStartIdx] != byteIn ) {
            pUParser->pData[0]  = byteIn;
            pUParser->byteIdx   = 1;

            if ( pUParser->pSeqStart[0] == byteIn ) {
                /* This is the first char of a SYNC START */
                pUParser->seqStartIdx   = 1;
                pUParser->cntrs.foundSyncStartAfterResync++;
            } else {
                pUParser->cntrs.bytesSkipped++;
            }
            return uParser_status_LOOKING;
        }
        pUParser->seqStartIdx++;
        
        if ( pUParser->seqStartIdx == pUParser->seqStartLen ) {
            /* Found whole start sequence! */
            pUParser->state     = uParser_state_LOOKING_HEADER;
            pUParser->byteIdx   = pUParser->seqStartLen;
            pUParser->seqEndIdx = 0;
            pUParser->cntrs.foundSyncStart++;
            return uParser_status_LOOKING;
        }
        return uParser_status_LOOKING;
    }
    else if ( pUParser->state == uParser_state_LOOKING_HEADER       ) {
        if ( pUParser->byteIdx < pUParser->headerLen ) {
            return uParser_status_LOOKING;
        }
        pUParser->cntrs.foundHeader++;

        /* Found whole header! */
        if ( pUParser->fFoundHeader == NULL ) {
            /* User not defined header function, so cannot get
            expectedLength, thus rise error */
            pUParser->state = uParser_state_IDLE;
            return uParser_status_INTERNAL_ERROR;
        }
        
        /* Get expected length */
        pUParser->expectedFrameLength = pUParser->fFoundHeader( pUParser->pData );
        
        if ( pUParser->expectedFrameLength >= pUParser->dataMaxLength ) {
            pUParser->cntrs.expectedLenOverflow++;
            pUParser->state = uParser_state_IDLE;
            return uParser_status_INTERNAL_ERROR;
        } else if ( pUParser->expectedFrameLength == pUParser->byteIdx ) {
            /* No bytes left, so just quit as FOUND */
            pUParser->state = uParser_state_IDLE;
            return uParser_status_FOUND;
        }

        pUParser->state = uParser_state_LOOKING_PAYLOAD;
        return uParser_status_LOOKING;
    }
    else if ( pUParser->state == uParser_state_LOOKING_PAYLOAD      ) {
        if ( pUParser->byteIdx < pUParser->expectedFrameLength ) {
            return uParser_status_LOOKING;
        }
        pUParser->cntrs.foundPayload++;
        /* Found whole frame! */
        pUParser->state = uParser_state_IDLE;
        return uParser_status_FOUND;
    }
    /* This code should never be reached, as there is finite list of states! */
    return uParser_status_INTERNAL_ERROR;
}

uParser_status_e _uParser_parseByEndSeq( uParser_t *pUParser, uint8_t byteIn ) {
    
    /* Looking for another sync START sequence */
    if ( pUParser->seqStartIdx >= pUParser->seqStartLen ) {
        pUParser->seqStartIdx = 0;
    }
    
    if ( pUParser->pSeqStart[pUParser->seqStartIdx++] != byteIn ) {
        pUParser->seqStartIdx = 0;
    }
    if ( pUParser->seqStartIdx == pUParser->seqStartLen ) {
        /* Found whole START sequence! */
        pUParser->byteIdx   = pUParser->seqStartLen;
        pUParser->seqEndIdx = 0;
        pUParser->cntrs.foundSyncStart++;
        return uParser_status_LOOKING;
    }
    
    /* Looking for another sync STOP sequence */
    if ( pUParser->seqEndIdx >= pUParser->seqEndLen ) {
        pUParser->seqEndIdx = 0;
    }
    if ( pUParser->pSeqEnd[pUParser->seqEndIdx++] != byteIn ) {
        pUParser->seqEndIdx = 0;
    }
    if ( pUParser->seqEndIdx == pUParser->seqEndLen ) {
        /* Found whole END sequence! */
        pUParser->seqStartIdx   = 0;
        pUParser->seqEndIdx     = 0;
        pUParser->cntrs.foundSyncStop++;
        return uParser_status_FOUND;
    }
    
    return uParser_status_LOOKING;
}

/* Public functions --------------------------------------------------- */

uParser_status_e uParser_streamIn(uParser_t *pUParser, uint8_t byteIn) {
    if ( pUParser == NULL ) { return uParser_status_INTERNAL_ERROR; }
    
    if ( pUParser->byteIdx >= pUParser->dataMaxLength ) {
        pUParser->cntrs.dataMaxOverflow++;
        pUParser->state = uParser_state_IDLE;
    }
    
    if ( pUParser->state == uParser_state_IDLE ) {
        /* Set for new sync */
        pUParser->byteIdx       = 0;
        pUParser->seqStartIdx   = 0;
        pUParser->seqEndIdx     = 0;
        pUParser->state = uParser_state_LOOKING_SYNC_START;
    }
    
    pUParser->pData[pUParser->byteIdx++] = byteIn;
    
    
    switch ( pUParser->parserType ) {
        case uParser_type_BY_LENGTH : return _uParser_parseByLen    ( pUParser, byteIn ); break;
        case uParser_type_BY_END_SEQ: return _uParser_parseByEndSeq ( pUParser, byteIn ); break;
        default:;
    }
    return uParser_status_INTERNAL_ERROR;
}

uParser_regStatus_e uParser_registerByLength(
    uParser_t               *pUParser,
    uint8_t                 *pData,
    uint16_t                dataMaxLen,
    uint16_t                headerLen,
    uParser_foundHeader_f   foundHeader,
    uint8_t                 *pSeqStart,
    uint16_t                seqStartLen )
{
    if ( foundHeader    == NULL ) { return uParser_regStatus_ERROR; }
    if ( headerLen      == 0    ) { return uParser_regStatus_ERROR; }
    
    uParser_regStatus_e regStatus =  uParser_regStatus_ERROR; 
    regStatus = _uParser_registerCommon( pUParser, pData, dataMaxLen,
                             pSeqStart, seqStartLen );
    
    if ( regStatus != uParser_regStatus_OK ) { return regStatus; }
    pUParser->parserType    = uParser_type_BY_LENGTH;
    pUParser->fFoundHeader  = foundHeader;
    pUParser->headerLen     = headerLen;


    uParser_reset( pUParser );
    return uParser_regStatus_OK;
}

uParser_regStatus_e uParser_registerByEndSequence(
    uParser_t               *pUParser,
    uint8_t                 *pData,
    uint16_t                dataMaxLen,
    uint8_t                 *pSeqStart,
    uint16_t                seqStartLen,
    uint8_t                 *pSeqEnd,
    uint16_t                seqEndLen )
{
    if ( pSeqEnd        == NULL ) { return uParser_regStatus_ERROR; }
    if ( seqStartLen    == 0    ) { return uParser_regStatus_ERROR; }
 
    uParser_regStatus_e regStatus =  uParser_regStatus_ERROR; 
    regStatus = _uParser_registerCommon( pUParser, pData, dataMaxLen,
                             pSeqStart, seqStartLen );
    
    if ( regStatus != uParser_regStatus_OK ) { return regStatus; }
    
    pUParser->parserType= uParser_type_BY_END_SEQ;
    pUParser->pSeqEnd   = pSeqEnd;
    pUParser->seqEndLen = seqEndLen;
    
    uParser_reset( pUParser );
    return uParser_regStatus_OK;
}

void uParser_reset(uParser_t *pUParser) {
    if ( pUParser == NULL ) { return; }
    pUParser->expectedFrameLength   = 0;
    pUParser->byteIdx               = 0;
    pUParser->state                 = uParser_state_IDLE;
    pUParser->seqStartIdx           = 0;
    pUParser->seqEndIdx             = 0;
}
