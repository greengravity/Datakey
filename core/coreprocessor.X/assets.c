#include "assets.h"

uint8_t unicodeLookup(uint16_t ct) {
    //using a full tablescan, coulde be more efficient readed because the table is sorted, but seems quick enough for lagfree experience anyway
    for ( uint8_t c=0;c<TOTAL_CHAR_COUNT;c++ ) {
        if ( unicodes[c].uccp == ct ) {
            return unicodes[c].cid;
        }
    }
    return CHAR_ENCODEERR;
}