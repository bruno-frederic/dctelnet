#ifndef XPRZMODEM_CATALOG_H
#define XPRZMODEM_CATALOG_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
#undef CATCOMP_NUMBERS
#undef CATCOMP_STRINGS
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#endif

#ifdef CATCOMP_BLOCK
#undef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_ZMODEM_OPTIONS 0
#define MSG_TEXT_MODE 1
#define MSG_OVERWRITE_MODE 2
#define MSG_IO_BUFFER_SIZE 3
#define MSG_FRAME_SIZE 4
#define MSG_ERROR_LIMIT 5
#define MSG_AUTO_ACTIVATE_RECEIVER 6
#define MSG_DELETE_AFTER_SENDING 7
#define MSG_KEEP_PARTIAL_FILES 8
#define MSG_SEND_FULL_PATH 9
#define MSG_USE_RECEIVED_PATH 10
#define MSG_DEFAULT_RECEIVE_PATH 11
#define MSG_MAXIMUM_BLOCK_SIZE 12
#define MSG_NO_XPR_FINFO 13
#define MSG_INVALID_O_FLAG 14
#define MSG_INVALID_A_FLAG 15
#define MSG_NO_DY_XPR_UNLINK 16
#define MSG_INVALID_D_FLAG 17
#define MSG_NO_KN_XPR_UNLINK 18
#define MSG_INVALID_K_FLAG 19
#define MSG_INVALID_S_FLAG 20
#define MSG_INVALID_R_FLAG 21
#define MSG_COMM_PROG_MISSING 22
#define MSG_NOT_ENOUGH_MEMORY 23
#define MSG_UPLOAD_USER_ERROR 24
#define MSG_SKIPPED_DUE_TO_ERRORS 25
#define MSG_NO_FILES_MATCH_TEMPLATE 26
#define MSG_CANT_OPEN_FILE 27
#define MSG_DELETING_FILE_AFTER_SEND 28
#define MSG_SENDING_TEXT_FILE 29
#define MSG_SENDING_BINARY_FILE 30
#define MSG_SENDING_FILE 31
#define MSG_SKIP_COMMAND_RECEIVED 32
#define MSG_TRANSFER_CANCELLED 33
#define MSG_DONE 34
#define MSG_SENDING_EOF 35
#define MSG_EOF_ACKNOWLEDGED 36
#define MSG_RESENDING_FROM 37
#define MSG_DOWNLOAD_USER_ERROR 38
#define MSG_IGNORING_COMMAND 39
#define MSG_DELETING_RECEIVED_FILE 40
#define MSG_KEEPING_RECEIVED_FILE 41
#define MSG_RETRIES_LEFT 42
#define MSG_BAD_EOF 43
#define MSG_EOF_RECEIVED 44
#define MSG_DATA_AT_BAD_POSITION 45
#define MSG_INSUFFICIENT_DISK_SPACE 46
#define MSG_FILE_EXISTS 47
#define MSG_RECEIVE_BINARY 48
#define MSG_RECEIVE_TEXT 49
#define MSG_ERROR_WRITING_FILE 50
#define MSG_BAD_DATA_PACKET_CRC_16 51
#define MSG_SENDER_CANCELED 52
#define MSG_DATA_PACKET_TIMEOUT 53
#define MSG_UNRECOG_DATA_PACKET 54
#define MSG_DATA_PACKET_TOO_LONG 55
#define MSG_BAD_DATA_PACKET_CRC_32 56
#define MSG_HEADER_SEARCH_GARBAGE 57
#define MSG_HEADER 58
#define MSG_ERROR 59
#define MSG_BAD_HEADER_CRC_16 60
#define MSG_BAD_HEADER_CRC_32 61
#define MSG_BAD_HEADER_CRC 62
#define MSG_BAD_ZMODEM_ESC_SEQ 63

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_ZMODEM_OPTIONS_STR "ZModem options:"
#define MSG_TEXT_MODE_STR "Text mode (Y,N,?,C):"
#define MSG_OVERWRITE_MODE_STR "Overwrite mode (Y,N,R,S):"
#define MSG_IO_BUFFER_SIZE_STR "I/O buffer size (KB):"
#define MSG_FRAME_SIZE_STR "Frame size (bytes):"
#define MSG_ERROR_LIMIT_STR "Error limit:"
#define MSG_AUTO_ACTIVATE_RECEIVER_STR "Auto-activate receiver:"
#define MSG_DELETE_AFTER_SENDING_STR "Delete after sending:"
#define MSG_KEEP_PARTIAL_FILES_STR "Keep partial files:"
#define MSG_SEND_FULL_PATH_STR "Send full path:"
#define MSG_USE_RECEIVED_PATH_STR "Use received path:"
#define MSG_DEFAULT_RECEIVE_PATH_STR "Default receive path:"
#define MSG_MAXIMUM_BLOCK_SIZE_STR "Maximum Block Size:"
#define MSG_NO_XPR_FINFO_STR "Can't Resume; xpr_finfo() not supported"
#define MSG_INVALID_O_FLAG_STR "Invalid O flag ignored; should be Y, N, R, or S"
#define MSG_INVALID_A_FLAG_STR "Invalid A flag ignored; should be Y or N"
#define MSG_NO_DY_XPR_UNLINK_STR "Can't use DY; xpr_unlink() not supported"
#define MSG_INVALID_D_FLAG_STR "Invalid D flag ignored; should be Y or N"
#define MSG_NO_KN_XPR_UNLINK_STR "Can't use KN; xpr_unlink() not supported"
#define MSG_INVALID_K_FLAG_STR "Invalid K flag ignored; should be Y or N"
#define MSG_INVALID_S_FLAG_STR "Invalid S flag ignored; should be Y or N"
#define MSG_INVALID_R_FLAG_STR "Invalid R flag ignored; should be Y or N"
#define MSG_COMM_PROG_MISSING_STR "Comm prog missing required function(s); see docs"
#define MSG_NOT_ENOUGH_MEMORY_STR "Not enough memory"
#define MSG_UPLOAD_USER_ERROR_STR "Upload cancelled or timed out"
#define MSG_SKIPPED_DUE_TO_ERRORS_STR "One or more files skipped due to errors"
#define MSG_NO_FILES_MATCH_TEMPLATE_STR "No files match template"
#define MSG_CANT_OPEN_FILE_STR "Can't open file; skipping"
#define MSG_DELETING_FILE_AFTER_SEND_STR "Deleting file after send"
#define MSG_SENDING_TEXT_FILE_STR "Sending text file..."
#define MSG_SENDING_BINARY_FILE_STR "Sending binary file..."
#define MSG_SENDING_FILE_STR "Sending file..."
#define MSG_SKIP_COMMAND_RECEIVED_STR "SKIP command received"
#define MSG_TRANSFER_CANCELLED_STR "Transfer cancelled"
#define MSG_DONE_STR "Done."
#define MSG_SENDING_EOF_STR "Sending EOF"
#define MSG_EOF_ACKNOWLEDGED_STR "EOF acknowledged"
#define MSG_RESENDING_FROM_STR "Resending from"
#define MSG_DOWNLOAD_USER_ERROR_STR "Download cancelled or timed out"
#define MSG_IGNORING_COMMAND_STR "Ignoring command"
#define MSG_DELETING_RECEIVED_FILE_STR "Deleting partially received file"
#define MSG_KEEPING_RECEIVED_FILE_STR "Keeping partially received file"
#define MSG_RETRIES_LEFT_STR "retries left"
#define MSG_BAD_EOF_STR "Bad EOF; here=%ld, there=%ld"
#define MSG_EOF_RECEIVED_STR "EOF received; checking for next file"
#define MSG_DATA_AT_BAD_POSITION_STR "Data at bad position; here=%ld, there=%ld"
#define MSG_INSUFFICIENT_DISK_SPACE_STR "Insufficient disk space; need %ld bytes, have %ld"
#define MSG_FILE_EXISTS_STR "File already exists; skipping"
#define MSG_RECEIVE_BINARY_STR "Receiving binary file..."
#define MSG_RECEIVE_TEXT_STR "Receiving text file..."
#define MSG_ERROR_WRITING_FILE_STR "Error writing file; disk full?"
#define MSG_BAD_DATA_PACKET_CRC_16_STR "Bad data packet CRC-16\x20"
#define MSG_SENDER_CANCELED_STR "Sender canceled\x20"
#define MSG_DATA_PACKET_TIMEOUT_STR "Data packet timeout\x20"
#define MSG_UNRECOG_DATA_PACKET_STR "Unrecognizable data packet\x20"
#define MSG_DATA_PACKET_TOO_LONG_STR "Data packet too long\x20"
#define MSG_BAD_DATA_PACKET_CRC_32_STR "Bad data packet CRC-32\x20"
#define MSG_HEADER_SEARCH_GARBAGE_STR "Header search garbage count exceeded\x20"
#define MSG_HEADER_STR "header"
#define MSG_ERROR_STR "error"
#define MSG_BAD_HEADER_CRC_16_STR "Bad Header CRC-16\x20"
#define MSG_BAD_HEADER_CRC_32_STR "Bad Header CRC-32\x20"
#define MSG_BAD_HEADER_CRC_STR "Bad Header CRC\x20"
#define MSG_BAD_ZMODEM_ESC_SEQ_STR "Bad ZMODEM escape sequence\x20"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
    LONG   cca_ID;
    STRPTR cca_Str;
};

static const struct CatCompArrayType CatCompArray[] =
{
    {MSG_ZMODEM_OPTIONS,(STRPTR)MSG_ZMODEM_OPTIONS_STR},
    {MSG_TEXT_MODE,(STRPTR)MSG_TEXT_MODE_STR},
    {MSG_OVERWRITE_MODE,(STRPTR)MSG_OVERWRITE_MODE_STR},
    {MSG_IO_BUFFER_SIZE,(STRPTR)MSG_IO_BUFFER_SIZE_STR},
    {MSG_FRAME_SIZE,(STRPTR)MSG_FRAME_SIZE_STR},
    {MSG_ERROR_LIMIT,(STRPTR)MSG_ERROR_LIMIT_STR},
    {MSG_AUTO_ACTIVATE_RECEIVER,(STRPTR)MSG_AUTO_ACTIVATE_RECEIVER_STR},
    {MSG_DELETE_AFTER_SENDING,(STRPTR)MSG_DELETE_AFTER_SENDING_STR},
    {MSG_KEEP_PARTIAL_FILES,(STRPTR)MSG_KEEP_PARTIAL_FILES_STR},
    {MSG_SEND_FULL_PATH,(STRPTR)MSG_SEND_FULL_PATH_STR},
    {MSG_USE_RECEIVED_PATH,(STRPTR)MSG_USE_RECEIVED_PATH_STR},
    {MSG_DEFAULT_RECEIVE_PATH,(STRPTR)MSG_DEFAULT_RECEIVE_PATH_STR},
    {MSG_MAXIMUM_BLOCK_SIZE,(STRPTR)MSG_MAXIMUM_BLOCK_SIZE_STR},
    {MSG_NO_XPR_FINFO,(STRPTR)MSG_NO_XPR_FINFO_STR},
    {MSG_INVALID_O_FLAG,(STRPTR)MSG_INVALID_O_FLAG_STR},
    {MSG_INVALID_A_FLAG,(STRPTR)MSG_INVALID_A_FLAG_STR},
    {MSG_NO_DY_XPR_UNLINK,(STRPTR)MSG_NO_DY_XPR_UNLINK_STR},
    {MSG_INVALID_D_FLAG,(STRPTR)MSG_INVALID_D_FLAG_STR},
    {MSG_NO_KN_XPR_UNLINK,(STRPTR)MSG_NO_KN_XPR_UNLINK_STR},
    {MSG_INVALID_K_FLAG,(STRPTR)MSG_INVALID_K_FLAG_STR},
    {MSG_INVALID_S_FLAG,(STRPTR)MSG_INVALID_S_FLAG_STR},
    {MSG_INVALID_R_FLAG,(STRPTR)MSG_INVALID_R_FLAG_STR},
    {MSG_COMM_PROG_MISSING,(STRPTR)MSG_COMM_PROG_MISSING_STR},
    {MSG_NOT_ENOUGH_MEMORY,(STRPTR)MSG_NOT_ENOUGH_MEMORY_STR},
    {MSG_UPLOAD_USER_ERROR,(STRPTR)MSG_UPLOAD_USER_ERROR_STR},
    {MSG_SKIPPED_DUE_TO_ERRORS,(STRPTR)MSG_SKIPPED_DUE_TO_ERRORS_STR},
    {MSG_NO_FILES_MATCH_TEMPLATE,(STRPTR)MSG_NO_FILES_MATCH_TEMPLATE_STR},
    {MSG_CANT_OPEN_FILE,(STRPTR)MSG_CANT_OPEN_FILE_STR},
    {MSG_DELETING_FILE_AFTER_SEND,(STRPTR)MSG_DELETING_FILE_AFTER_SEND_STR},
    {MSG_SENDING_TEXT_FILE,(STRPTR)MSG_SENDING_TEXT_FILE_STR},
    {MSG_SENDING_BINARY_FILE,(STRPTR)MSG_SENDING_BINARY_FILE_STR},
    {MSG_SENDING_FILE,(STRPTR)MSG_SENDING_FILE_STR},
    {MSG_SKIP_COMMAND_RECEIVED,(STRPTR)MSG_SKIP_COMMAND_RECEIVED_STR},
    {MSG_TRANSFER_CANCELLED,(STRPTR)MSG_TRANSFER_CANCELLED_STR},
    {MSG_DONE,(STRPTR)MSG_DONE_STR},
    {MSG_SENDING_EOF,(STRPTR)MSG_SENDING_EOF_STR},
    {MSG_EOF_ACKNOWLEDGED,(STRPTR)MSG_EOF_ACKNOWLEDGED_STR},
    {MSG_RESENDING_FROM,(STRPTR)MSG_RESENDING_FROM_STR},
    {MSG_DOWNLOAD_USER_ERROR,(STRPTR)MSG_DOWNLOAD_USER_ERROR_STR},
    {MSG_IGNORING_COMMAND,(STRPTR)MSG_IGNORING_COMMAND_STR},
    {MSG_DELETING_RECEIVED_FILE,(STRPTR)MSG_DELETING_RECEIVED_FILE_STR},
    {MSG_KEEPING_RECEIVED_FILE,(STRPTR)MSG_KEEPING_RECEIVED_FILE_STR},
    {MSG_RETRIES_LEFT,(STRPTR)MSG_RETRIES_LEFT_STR},
    {MSG_BAD_EOF,(STRPTR)MSG_BAD_EOF_STR},
    {MSG_EOF_RECEIVED,(STRPTR)MSG_EOF_RECEIVED_STR},
    {MSG_DATA_AT_BAD_POSITION,(STRPTR)MSG_DATA_AT_BAD_POSITION_STR},
    {MSG_INSUFFICIENT_DISK_SPACE,(STRPTR)MSG_INSUFFICIENT_DISK_SPACE_STR},
    {MSG_FILE_EXISTS,(STRPTR)MSG_FILE_EXISTS_STR},
    {MSG_RECEIVE_BINARY,(STRPTR)MSG_RECEIVE_BINARY_STR},
    {MSG_RECEIVE_TEXT,(STRPTR)MSG_RECEIVE_TEXT_STR},
    {MSG_ERROR_WRITING_FILE,(STRPTR)MSG_ERROR_WRITING_FILE_STR},
    {MSG_BAD_DATA_PACKET_CRC_16,(STRPTR)MSG_BAD_DATA_PACKET_CRC_16_STR},
    {MSG_SENDER_CANCELED,(STRPTR)MSG_SENDER_CANCELED_STR},
    {MSG_DATA_PACKET_TIMEOUT,(STRPTR)MSG_DATA_PACKET_TIMEOUT_STR},
    {MSG_UNRECOG_DATA_PACKET,(STRPTR)MSG_UNRECOG_DATA_PACKET_STR},
    {MSG_DATA_PACKET_TOO_LONG,(STRPTR)MSG_DATA_PACKET_TOO_LONG_STR},
    {MSG_BAD_DATA_PACKET_CRC_32,(STRPTR)MSG_BAD_DATA_PACKET_CRC_32_STR},
    {MSG_HEADER_SEARCH_GARBAGE,(STRPTR)MSG_HEADER_SEARCH_GARBAGE_STR},
    {MSG_HEADER,(STRPTR)MSG_HEADER_STR},
    {MSG_ERROR,(STRPTR)MSG_ERROR_STR},
    {MSG_BAD_HEADER_CRC_16,(STRPTR)MSG_BAD_HEADER_CRC_16_STR},
    {MSG_BAD_HEADER_CRC_32,(STRPTR)MSG_BAD_HEADER_CRC_32_STR},
    {MSG_BAD_HEADER_CRC,(STRPTR)MSG_BAD_HEADER_CRC_STR},
    {MSG_BAD_ZMODEM_ESC_SEQ,(STRPTR)MSG_BAD_ZMODEM_ESC_SEQ_STR},
};

#endif /* CATCOMP_ARRAY */


/****************************************************************************/


#ifdef CATCOMP_BLOCK

static const char CatCompBlock[] =
{
    "\x00\x00\x00\x00\x00\x10"
    MSG_ZMODEM_OPTIONS_STR "\x00"
    "\x00\x00\x00\x01\x00\x16"
    MSG_TEXT_MODE_STR "\x00\x00"
    "\x00\x00\x00\x02\x00\x1A"
    MSG_OVERWRITE_MODE_STR "\x00"
    "\x00\x00\x00\x03\x00\x16"
    MSG_IO_BUFFER_SIZE_STR "\x00"
    "\x00\x00\x00\x04\x00\x14"
    MSG_FRAME_SIZE_STR "\x00"
    "\x00\x00\x00\x05\x00\x0E"
    MSG_ERROR_LIMIT_STR "\x00\x00"
    "\x00\x00\x00\x06\x00\x18"
    MSG_AUTO_ACTIVATE_RECEIVER_STR "\x00"
    "\x00\x00\x00\x07\x00\x16"
    MSG_DELETE_AFTER_SENDING_STR "\x00"
    "\x00\x00\x00\x08\x00\x14"
    MSG_KEEP_PARTIAL_FILES_STR "\x00"
    "\x00\x00\x00\x09\x00\x10"
    MSG_SEND_FULL_PATH_STR "\x00"
    "\x00\x00\x00\x0A\x00\x14"
    MSG_USE_RECEIVED_PATH_STR "\x00\x00"
    "\x00\x00\x00\x0B\x00\x16"
    MSG_DEFAULT_RECEIVE_PATH_STR "\x00"
    "\x00\x00\x00\x0C\x00\x14"
    MSG_MAXIMUM_BLOCK_SIZE_STR "\x00"
    "\x00\x00\x00\x0D\x00\x28"
    MSG_NO_XPR_FINFO_STR "\x00"
    "\x00\x00\x00\x0E\x00\x30"
    MSG_INVALID_O_FLAG_STR "\x00"
    "\x00\x00\x00\x0F\x00\x2A"
    MSG_INVALID_A_FLAG_STR "\x00\x00"
    "\x00\x00\x00\x10\x00\x2A"
    MSG_NO_DY_XPR_UNLINK_STR "\x00\x00"
    "\x00\x00\x00\x11\x00\x2A"
    MSG_INVALID_D_FLAG_STR "\x00\x00"
    "\x00\x00\x00\x12\x00\x2A"
    MSG_NO_KN_XPR_UNLINK_STR "\x00\x00"
    "\x00\x00\x00\x13\x00\x2A"
    MSG_INVALID_K_FLAG_STR "\x00\x00"
    "\x00\x00\x00\x14\x00\x2A"
    MSG_INVALID_S_FLAG_STR "\x00\x00"
    "\x00\x00\x00\x15\x00\x2A"
    MSG_INVALID_R_FLAG_STR "\x00\x00"
    "\x00\x00\x00\x16\x00\x32"
    MSG_COMM_PROG_MISSING_STR "\x00\x00"
    "\x00\x00\x00\x17\x00\x12"
    MSG_NOT_ENOUGH_MEMORY_STR "\x00"
    "\x00\x00\x00\x18\x00\x1E"
    MSG_UPLOAD_USER_ERROR_STR "\x00"
    "\x00\x00\x00\x19\x00\x28"
    MSG_SKIPPED_DUE_TO_ERRORS_STR "\x00"
    "\x00\x00\x00\x1A\x00\x18"
    MSG_NO_FILES_MATCH_TEMPLATE_STR "\x00"
    "\x00\x00\x00\x1B\x00\x1A"
    MSG_CANT_OPEN_FILE_STR "\x00"
    "\x00\x00\x00\x1C\x00\x1A"
    MSG_DELETING_FILE_AFTER_SEND_STR "\x00\x00"
    "\x00\x00\x00\x1D\x00\x16"
    MSG_SENDING_TEXT_FILE_STR "\x00\x00"
    "\x00\x00\x00\x1E\x00\x18"
    MSG_SENDING_BINARY_FILE_STR "\x00\x00"
    "\x00\x00\x00\x1F\x00\x10"
    MSG_SENDING_FILE_STR "\x00"
    "\x00\x00\x00\x20\x00\x16"
    MSG_SKIP_COMMAND_RECEIVED_STR "\x00"
    "\x00\x00\x00\x21\x00\x14"
    MSG_TRANSFER_CANCELLED_STR "\x00\x00"
    "\x00\x00\x00\x22\x00\x06"
    MSG_DONE_STR "\x00"
    "\x00\x00\x00\x23\x00\x0C"
    MSG_SENDING_EOF_STR "\x00"
    "\x00\x00\x00\x24\x00\x12"
    MSG_EOF_ACKNOWLEDGED_STR "\x00\x00"
    "\x00\x00\x00\x25\x00\x10"
    MSG_RESENDING_FROM_STR "\x00\x00"
    "\x00\x00\x00\x26\x00\x20"
    MSG_DOWNLOAD_USER_ERROR_STR "\x00"
    "\x00\x00\x00\x27\x00\x12"
    MSG_IGNORING_COMMAND_STR "\x00\x00"
    "\x00\x00\x00\x28\x00\x22"
    MSG_DELETING_RECEIVED_FILE_STR "\x00\x00"
    "\x00\x00\x00\x29\x00\x20"
    MSG_KEEPING_RECEIVED_FILE_STR "\x00"
    "\x00\x00\x00\x2A\x00\x0E"
    MSG_RETRIES_LEFT_STR "\x00\x00"
    "\x00\x00\x00\x2B\x00\x1E"
    MSG_BAD_EOF_STR "\x00\x00"
    "\x00\x00\x00\x2C\x00\x26"
    MSG_EOF_RECEIVED_STR "\x00\x00"
    "\x00\x00\x00\x2D\x00\x2A"
    MSG_DATA_AT_BAD_POSITION_STR "\x00"
    "\x00\x00\x00\x2E\x00\x32"
    MSG_INSUFFICIENT_DISK_SPACE_STR "\x00"
    "\x00\x00\x00\x2F\x00\x1E"
    MSG_FILE_EXISTS_STR "\x00"
    "\x00\x00\x00\x30\x00\x1A"
    MSG_RECEIVE_BINARY_STR "\x00\x00"
    "\x00\x00\x00\x31\x00\x18"
    MSG_RECEIVE_TEXT_STR "\x00\x00"
    "\x00\x00\x00\x32\x00\x20"
    MSG_ERROR_WRITING_FILE_STR "\x00\x00"
    "\x00\x00\x00\x33\x00\x18"
    MSG_BAD_DATA_PACKET_CRC_16_STR "\x00"
    "\x00\x00\x00\x34\x00\x12"
    MSG_SENDER_CANCELED_STR "\x00\x00"
    "\x00\x00\x00\x35\x00\x16"
    MSG_DATA_PACKET_TIMEOUT_STR "\x00\x00"
    "\x00\x00\x00\x36\x00\x1C"
    MSG_UNRECOG_DATA_PACKET_STR "\x00"
    "\x00\x00\x00\x37\x00\x16"
    MSG_DATA_PACKET_TOO_LONG_STR "\x00"
    "\x00\x00\x00\x38\x00\x18"
    MSG_BAD_DATA_PACKET_CRC_32_STR "\x00"
    "\x00\x00\x00\x39\x00\x26"
    MSG_HEADER_SEARCH_GARBAGE_STR "\x00"
    "\x00\x00\x00\x3A\x00\x08"
    MSG_HEADER_STR "\x00\x00"
    "\x00\x00\x00\x3B\x00\x06"
    MSG_ERROR_STR "\x00"
    "\x00\x00\x00\x3C\x00\x14"
    MSG_BAD_HEADER_CRC_16_STR "\x00\x00"
    "\x00\x00\x00\x3D\x00\x14"
    MSG_BAD_HEADER_CRC_32_STR "\x00\x00"
    "\x00\x00\x00\x3E\x00\x10"
    MSG_BAD_HEADER_CRC_STR "\x00"
    "\x00\x00\x00\x3F\x00\x1C"
    MSG_BAD_ZMODEM_ESC_SEQ_STR "\x00"
};

#endif /* CATCOMP_BLOCK */


/****************************************************************************/


struct LocaleInfo
{
    APTR li_LocaleBase;
    APTR li_Catalog;
};


#ifdef CATCOMP_CODE

STRPTR GetString(struct LocaleInfo *li, LONG stringNum)
{
LONG   *l;
UWORD  *w;
STRPTR  builtIn;

    l = (LONG *)CatCompBlock;

    while (*l != stringNum)
    {
        w = (UWORD *)((ULONG)l + 4);
        l = (LONG *)((ULONG)l + (ULONG)*w + 6);
    }
    builtIn = (STRPTR)((ULONG)l + 6);

#define XLocaleBase LocaleBase
#define LocaleBase li->li_LocaleBase
    
    if (LocaleBase)
        return(GetCatalogStr(li->li_Catalog,stringNum,builtIn));
#define LocaleBase XLocaleBase
#undef XLocaleBase

    return(builtIn);
}


#endif /* CATCOMP_CODE */


/****************************************************************************/


#endif /* XPRZMODEM_CATALOG_H */
