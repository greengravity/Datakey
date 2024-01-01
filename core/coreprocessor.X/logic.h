/* 
 * File:   
 * Author: greengravity
 * Comments:
 * Revision history: 
 */


#ifndef XC_LOGIC_H
#define	XC_LOGIC_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>
#include "fs/ff.h"
#include "assets.h"
#include "mcc_generated_files/rtcc.h"

#define CTX_BUFFER_SIZE 2000
#define MIN_PIN_SIZE 3
#define MAX_PIN_SIZE 20
#define DEFAULT_PIN_SIZE 8
#define DEFAULT_PIN_TRIES 6
#define MAX_PIN_TRIES 9
#define MIN_PIN_TRIES 1

#define TEXT_WORK_TRESHHOLD 17
#define MAX_TEXT_LEN 1023
#define TEXTAREA_WIDTH 150
#define TEXTAREA_VIEWPORT_LINES 3
#define TEXTVIEWER_LINES 6
#define OVERVIEW_LINES 5
#define MAX_TEXTAREA_SPACE_BREAK 80
//#define TEXTAREA_RINGBUFFER_LEN 64
#define TOKEN_BLOCK_SIZE 4096  //256*16 
#define TOKEN_READ_MOD_BUFFERSIZE 512 //TOKEN_BLOCK_SIZE % TOKEN_READ_MOD_BUFFERSIZE must be zero
#define MAX_OVERVIEW_ENTRY_COUNT 16
#define MAX_SEARCH_LEN 20
//#define OVERVIEW_RELOAD_OFFS (MAX_OVERVIEW_ENTRY_COUNT / 2 - (OVERVIEW_LINES + 1))

typedef enum {   
    PFLD_NOTHING,
    PFLD_GROUND,
    PFLD_OWL,
    PFLD_LITTLEDRAGON,
    PFLD_TREE,
    PFLD_STONE,
    PFLD_SHIELD,
    PFLD_DRAGON,               
    PFLD_CRACKSTONE,                           
    PFLD_SWORD,                           
    PFLD_GRAIL,                           
} PINFIELD;


typedef enum {
    NEW,
    NAME,
    USER,        
    KEY1,
    KEY2,
    URL,                
    INFO,
    CHECK,
} TOKEN_TYPE;

typedef enum {
    MASTERKEY_GEN,
    MASTERKEY_VERIFY,
    MASTERKEY_CHECK,
    SEARCHFIELD,
    TOKEN,                            
}INPUT_TYPE;

typedef enum {
    USB_MODE_OFF,
    USB_MODE_KEYBOARD          
} USB_MODE;

typedef struct {
    uint16_t highlight_color1;
    uint16_t highlight_color2;
    uint8_t brightness;
    USB_MODE umode;    
    uint8_t pin_tries;
    uint8_t pin_len;    
} DEVICE_OPTIONS;

typedef struct {
    TOKEN_TYPE type;
    uint8_t textid; 
    uint8_t textidview; 
    uint8_t tkname;
} TOKEN_CONFIG;

typedef enum {    
    UNCHANGED,
    REFRESH,    
    ONBUTTON_PRESS,
    ONBUTTON_RELEASE,
    ONBUTTON_DOWN,
    REMOVECHAR,
    ANIMATION,
    ANIMATION2,
    LOADENTRY,
    AREASWITCH,
    IO_UPDATE,
} RENDER_INFO;

typedef enum {
    EMPTY,
    ERROR,
    ERROR_CONTEXT,
    ERROR_SD_CD,
    ERROR_SD_FAILURE,
    INITIAL,
    PIN_INPUT,
    VERIFY_KEY_AFTER_PIN,
    KEY_INPUT, 
    ENTRY_OVERVIEW,
    ENTRY_DETAIL,
    GAME_TETRIS,
    USB_PUSH,
    VIEW_TOKEN,
    EDIT_ENTRY,
    SEARCH_ENTRIES,
    MESSAGEBOX,
    CHOOSEBOX,
    DEVICEOPTIONS,            
    PINOPTIONS,
    //OPTIONS2
} CONTEXT_TYPE;

typedef enum {
    CHBX_ABORT,
    CHBX_SEARCH_ENTRY,
    CHBX_CREATE_ENTRY,
    CHBX_DELETE_ENTRY,
    CHBX_OPTIONS,
    CHBX_DEVICEOPTIONS,
    CHBX_PINOPTIONS,
    CHBX_GAMES,
    CHBX_RETURN_OVERVIEW,
    CHBX_EDIT_TOKEN,
    CHBX_DELETE_TOKEN,
    CHBX_PUSH_TOKEN        
} CHBX_SELECTIONS;

typedef enum {
    MSGB_NO_RESULT,
    MSGB_EDIT_ABORT_YES,
    MSGB_EDIT_ABORT_NO,
    MSGB_EDIT_ACCEPT_YES,
    MSGB_EDIT_ACCEPT_NO,  
    MSGB_DELETE_ENTRY_YES,
    MSGB_DELETE_ENTRY_NO,
    MSGB_DELETE_TOKEN_YES,
    MSGB_DELETE_TOKEN_NO,
    MSGB_MKEY_ERROR_OK,
    MSGB_SEARCHINFO_COMMIT,
} MSGB_CHOICES;

typedef enum {
    DEVCST_BATTERY,
    DEVCST_CHARGER,
    DEVCST_USB,
} DEVICE_CONNECTION_STATE;

typedef enum {    
    TETS_I,
    TETS_O,
    TETS_T,
    TETS_L,
    TETS_J,
    TETS_Z,
    TETS_S,     
    TETS_EMPTY,
} TETRIS_STONES;

extern const uint8_t tetris_piecesize[];
extern const uint8_t tetris_pieces[];


typedef struct {
    char msg[256];
} CTX_ERROR;

typedef struct {    
    RENDER_INFO rinf;
    INPUT_TYPE inp;
    TOKEN_TYPE type;    
    MSGB_CHOICES mboxresult;
    uint8_t datachanged;
    uint8_t okbmap;   //old keymap
    uint8_t kbmap;    //curr keymap
    uint8_t okbx;     //old keyx - for animation
    uint8_t okby;     //old keyy - for animation  
    uint8_t kbx;      //keyx
    uint8_t kby;      //keyy
    uint8_t oselarea; //old working area 
    uint8_t selarea;  //current working area
    uint8_t tewpx;    //textarea current writer x position
    uint8_t text[MAX_TEXT_LEN + TEXT_WORK_TRESHHOLD];
    uint8_t __pad1;
    uint16_t tavccoff;  //textarea viewport current character offset
    uint16_t tavcloff;  //textarea viewport current lineoffset
    uint16_t tewp;    //textarea current writer position
    uint16_t tlen;    //current length of text        
} IO_CONTEXT;


//Context for Pin Input
typedef struct {
    TETRIS_STONES field[200];
    int currx;
    int curry;      
    int currlvllines;
    TETRIS_STONES curr;
    TETRIS_STONES next;   
    uint8_t currrot;
    bool gameover;
    bool startpause;
    bool b2btetris;
    uint32_t lvl;
    uint32_t score;   
    uint32_t lines;
} CTX_TETRIS;


//Context for Pin Input
typedef struct {
    bool generatenew;
    bool pinIsInitial;
    bool verify;
    bool error;
    bool haderror;
    bool errorchecked;
    uint8_t ppos;    
    uint8_t pin[MAX_PIN_SIZE];
    uint8_t verifypin[MAX_PIN_SIZE];
    uint8_t pinerr;
    uint8_t px;
    uint8_t py;
    uint8_t field[64];    
    uint8_t extrasteps;    
} CTX_PIN_INPUT;

typedef struct { 
    IO_CONTEXT io;
    bool fschecked;
    bool generatemode;
    uint8_t generatedkey[16];
    bool error;
    bool haderror;    
} CTX_KEY_INPUT;

typedef struct {
    bool loaded;
    TCHAR path[16];
    uint8_t name[48];
    uint8_t __pad1;
} CTX_OVERVIEW_ENTRY;


typedef struct {        
    bool initialized;
    bool needinitsearch;    
    uint8_t ohighlightpos;  
    uint8_t searchstr[MAX_SEARCH_LEN];            
    uint8_t searchsuccessful;
    CTX_OVERVIEW_ENTRY entries[MAX_OVERVIEW_ENTRY_COUNT];
    uint16_t cursor;
    uint16_t bufferstart;
    uint16_t bufferlen;
    //uint16_t entrycenter;
    uint16_t entrycount;
} CTX_ENTRY_OVERVIEW;

typedef struct {
    TOKEN_TYPE type;
    uint8_t isset;
    uint8_t text[48];   
    uint8_t __pad1;
} CTX_ENTRY_DETAIL_TOKEN;

typedef struct {
    uint16_t tlen;
    uint8_t text[MAX_TEXT_LEN + TEXT_WORK_TRESHHOLD];     
} CTX_USB_PUSH;

typedef struct {    
    TCHAR path[16];
    TOKEN_TYPE selected;
    TOKEN_TYPE oselected;
    CTX_ENTRY_DETAIL_TOKEN tokeninfo[6];
    uint8_t tokencount;
    uint8_t __pad1;
    uint16_t overviewcursor;
} CTX_ENTRY_DETAIL;


typedef struct {    
    IO_CONTEXT io;
    TCHAR path[16];
    uint16_t overviewcursor;
} CTX_EDIT_ENTRY;


typedef struct {      
    IO_CONTEXT io;    
    uint8_t searchstr[MAX_SEARCH_LEN];      
    uint16_t overviewcursor;
} CTX_SEARCH_ENTRIES;
             
typedef struct {    
    uint8_t text[MAX_TEXT_LEN + TEXT_WORK_TRESHHOLD];
    TOKEN_TYPE type;
    uint8_t __pad1;
    uint16_t tlen;
    uint16_t currline;
    uint16_t lines;
} CTX_VIEW_TOKEN;

typedef struct {
    uint8_t msgtxt[50];
    uint8_t sel;
    uint8_t osel;
    uint8_t  choicecount;
    uint8_t __pad1;
    MSGB_CHOICES mchoices[6];    
    uint16_t mchoiceicon[6];    
} CTX_MSG_BOX;

typedef struct {
    uint16_t textid[6];
    uint8_t selectid[6];
    uint8_t options;
    CHBX_SELECTIONS selected;
    CHBX_SELECTIONS oselected;    
} CTX_CHOOSE_BOX;


typedef struct {
    bcdTime_t time;
    uint8_t selected;
    uint8_t oselected;    
    uint8_t holdtime;
    uint8_t __pad1;
} CTX_DEVICEOPTIONS;

typedef struct {
    uint8_t selected;
    uint8_t oselected;
    uint8_t pin_len;
    uint8_t __pad1;
} CTX_PINOPTIONS;


//Context storage
typedef struct {
    uint16_t ctxptr; 
    uint16_t bufferlen;
    uint8_t *ctxbuffer;
    uint8_t ctxtype;    
    uint8_t power;       
    bool fsmounted;
    bool fileopen;    
    uint8_t chrg_open;
    uint8_t adc_rw_state;
    uint8_t __pad1;
    RENDER_INFO rinf;
    DEVICE_CONNECTION_STATE dcstate;
    uint8_t bus_sense;        
    uint16_t analogtest;
    FATFS drive;
} APP_CONTEXT;

extern const uint16_t highlight_color_tab[];
extern const TOKEN_CONFIG token_configs[];
extern DEVICE_OPTIONS device_options;

bool isPinSet();
bool isKeySet( );
void setMasterKey(uint8_t *key);
uint8_t* getMasterKey();
void swipeKeys();
uint8_t verifyMasterKey( );
bool isKeyEncr();
void setKeyEncr(bool encr);


void tet_getRotatedShape( uint8_t *shape, TETRIS_STONES piece, uint8_t rot );
uint8_t getCharactersInLine(uint8_t* text, uint16_t coff, uint16_t maxchars );
bool setContext(APP_CONTEXT *ctx, CONTEXT_TYPE type);
void setInitialContext( APP_CONTEXT* ctx );
void updateContext(APP_CONTEXT* ctx);

#endif	/* XC_HEADER_TEMPLATE_H */

