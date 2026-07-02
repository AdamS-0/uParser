/**
 * @file       uParser.h
 * @brief      A minimalistic library which implements common logic part
 *             of every byte stream parser - finding defined structure
  *            in byte stream.
 * 
 * @details    Header file with definitions of uParser structs.
 * 
 * @author     Adam Smerda
 * @date       2026-05-24
 * @version    1.0
 * @copyright  Copyright (c) 2026, AdamS-0
 * @license    LGPL-2.1
 */
 
#ifndef __UPARSER_H__
#define __UPARSER_H__

/* Includes ----------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>


/* Defines ------------------------------------------------------------ */


/* Type definitions --------------------------------------------------- */

/** Parser state (used only in BY-LENGTH mode) */
typedef enum {
    uParser_state_IDLE              = 0,
    uParser_state_LOOKING_SYNC_START,
    uParser_state_LOOKING_HEADER    ,
    uParser_state_LOOKING_PAYLOAD   ,
} uParser_state_e;

/** type/mode */
typedef enum {
    uParser_type_BY_LENGTH  = 0,
    uParser_type_BY_END_SEQ ,
} uParser_type_e;

/** @brief Function type, that is called after successfull header found.
 * Defined by user, and given as parameter in register function.
 * @param   pHeader Pointer of the Header part
 * @return  expectedLength - user should return total number of bytes
 *                           in this frame (with header and footer!)
 */
typedef uint16_t (*uParser_foundHeader_f)(uint8_t *pHeader);

typedef struct {
    uint32_t    dataMaxOverflow;
    uint32_t    foundSyncStart;
    uint32_t    foundSyncStop;
    uint32_t    foundHeader;
    uint32_t    foundPayload;
    uint32_t    bytesResync;
    uint32_t    expectedLenOverflow;
} uParser_cntrs_t;

/**
 * @brief Parser structure with basic stuff, like sizes, pointers an so on,
 *          used in parsing. Struct was arranged to 4 B alignment
 */
typedef struct {
    /* Commo parameters */
    /* 0 - 3 B -------------------------------------------------------- */
    uParser_foundHeader_f   fFoundHeader;
    /* 4 - 7 B -------------------------------------------------------- */
    uint8_t                 *pData;
    /* 8 - 11 B ------------------------------------------------------- */
    uint16_t                dataMaxLength;
    uParser_state_e         state : 8;
    uParser_type_e          parserType : 8;
    /* 12 - 15 B ------------------------------------------------------ */
    //! Value set in BY-LENGTH mode, by user's implementation of fFoundHeader
    uint16_t                expectedFrameLength;
    uint16_t                byteIdx;
    /* 16 - 19 B ------------------------------------------------------ */
    uint16_t                headerLen;
    uint8_t                 seqStartLen ; //! Length of SEQ_START
    uint8_t                 seqStartIdx ; //! Index to check byteIn with SEQ_START 
    /* 20 - 23 B ------------------------------------------------------ */
    uint8_t                 *pSeqStart  ; //! Pointer to a SEQ_START
    /* 24 - 27 B ------------------------------------------------------ */
    uParser_cntrs_t         cntrs;
    
    /* Used during uParser_type_BY_END_SEQ */
    uint8_t                 *pSeqEnd    ; //! Pointer to a SEQ_END
    uint8_t                 seqEndLen   ; //! Length of SEQ_END
    uint8_t                 seqEndIdx   ; //! Index to check byteIn with SEQ_END
} uParser_t;

typedef enum {
    uParser_status_INTERNAL_ERROR   = 0,
    uParser_status_LOOKING          = 0,
    uParser_status_FOUND            ,
} uParser_status_e;


typedef enum {
    uParser_regStatus_OK    = 0,
    uParser_regStatus_ERROR ,
} uParser_regStatus_e;

/* Public functions --------------------------------------------------- */

uParser_status_e uParser_streamIn(uParser_t *pUParser, uint8_t byteIn);

/** @brief Function to register/configure parser instance with given work
 * flow model.
 * @param   pUParser    Pointer to an clear instance of uParser.
 * @param   pData       Pointer to an array of bytes, where will be stored
 *                      decoded bytes.
 * @param   dataMaxLen  Size of the pData array, in Bytes
 * @param   headerLen   Size of a header section, which after will be called
 *                      "foundHeader" callback
 * @param   foundHeader Pointer to a function, "callback" after receiving
 *                      headerLen' bytes
 * @param   pSeqStart   Pointer to n byte array with SEQ_START
 * @param   seqStartLen Size of SEQ_START array
 * 
 * @return  uParser_regStatus_e Status of registration process 
 */
uParser_regStatus_e uParser_registerByLength(
    uParser_t               *pUParser,
    uint8_t                 *pData,
    uint16_t                dataMaxLen,
    uint16_t                headerLen,
    uParser_foundHeader_f   foundHeader,
    uint8_t                 *pSeqStart,
    uint16_t                seqStartLen );

/** @brief Function to register/configure parser instance with given work
 * flow model.
 * @param   pUParser    Pointer to an clear instance of uParser.
 * @param   pData       Pointer to an array of bytes, where will be stored
 *                      decoded bytes.
 * @param   dataMaxLen  Size of the pData array, in Bytes
 * @param   pSeqStart   Pointer to an byte array with SEQ_START
 * @param   seqStartLen Size of SEQ_START array
 * @param   pSeqEnd     Pointer to an byte array with SEQ_END
 * @param   seqEndLen   Size of SEQ_END array
 * 
 * @return  uParser_regStatus_e Status of registration process 
 */
 uParser_regStatus_e uParser_registerByEndSequence(
    uParser_t               *pUParser,
    uint8_t                 *pData,
    uint16_t                dataMaxLen,
    uint8_t                 *pSeqStart,
    uint16_t                seqStartLen,
    uint8_t                 *pSeqEnd,
    uint16_t                seqEndLen );

/** @brief Reset function
 * @param pUParser  Pointer to an not NULL instance of parser struct
 */
void uParser_reset(uParser_t *pUParser);

#endif /* __UPARSER_H__ */