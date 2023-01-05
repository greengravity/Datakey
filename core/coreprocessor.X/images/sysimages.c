#include "../images/sysimages.h"

const int sizeofimage = sizeof(GFXimage);

extern int getEntryIndexTextCompareValue(uint8_t cval) {
    if ( cval >= 67 && cval < 93) return cval - 33; //a-z get same val than A-Z
    if ( cval >= 151 && cval < 212) return cval - 32; //àáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþ get same val than ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞ
    return cval;                
}

/************** BEGIN OF BITMAPS **********************/
/**************** Bitmaps for Systemmenu **************/
const uint8_t systemsymb_Bitmaps[] = {
//SSYMB_KEYBOARD_DEL_SYMBOL w:10 h:13 id:0
//          
//          
//     #    
//    ##    
//   ###### 
//  ## # ## 
// #### ### 
//  ## # ## 
//   ###### 
//    ##    
//     #    
//          
//          
0x00, 0x00, 
0x02, 0x00, 
0x07, 0x00, 
0x0f, 0x80, 
0x1a, 0xc0, 
0x3d, 0xe0, 
0x0a, 0x80, 
0x0f, 0x80, 
0x0f, 0x80, 
0x00, 0x00, 


//SSYMB_KEYBOARD_CURSOR_BACKWARD_SYMBOL w:10 h:13 id:1
//          
//          
//          
// #  #     
// # ##     
// ######## 
// ######## 
// # ##     
// #  #     
//          
//          
//          
//          
0x00, 0x00, 
0x1f, 0x80, 
0x06, 0x00, 
0x0f, 0x00, 
0x1f, 0x80, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x00, 0x00, 


//SSYMB_KEYBOARD_CURSOR_FORWARD_SYMBOL w:10 h:13 id:2
//          
//          
//          
//     #  # 
//     ## # 
// ######## 
// ######## 
//     ## # 
//     #  # 
//          
//          
//          
//          
0x00, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x1f, 0x80, 
0x0f, 0x00, 
0x06, 0x00, 
0x1f, 0x80, 
0x00, 0x00, 


//SSYMB_KEYBOARD_BACK_SYMBOL w:10 h:8 id:3
//          
//          
//          
// #  ###   
// ###   #  
// ####   # 
//        # 
//       #  
0x00, 
0x1c, 
0x0c, 
0x0c, 
0x14, 
0x10, 
0x10, 
0x09, 
0x06, 
0x00, 


//SSYMB_KEYBOARD_ACCEPT_SYMBOL w:20 h:13 id:4
//                    
//                    
//                    
//       ##     ##    
//     ####     ##    
//   #############    
//   #############    
//     ####           
//       ##           
//                    
//                    
//                    
//                    
0x00, 0x00, 
0x00, 0x00, 
0x00, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x0f, 0x00, 
0x0f, 0x00, 
0x1f, 0x80, 
0x1f, 0x80, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x1e, 0x00, 
0x1e, 0x00, 
0x00, 0x00, 
0x00, 0x00, 
0x00, 0x00, 
0x00, 0x00, 


//SSYMB_KEYBOARD_RETURN_SYMBOL w:10 h:8 id:5
//          
//          
//          
// #  ###   
// ###   #  
// ####   # 
//        # 
//       #  
0x00, 
0x1c, 
0x0c, 
0x0c, 
0x14, 
0x10, 
0x10, 
0x09, 
0x06, 
0x00, 


//SSYMB_INPUTFIELD_CURSOR_SYMBOL w:1 h:12 id:6
//#
//#
//#
//#
//#
//#
//#
//#
//#
//#
//#
//#
0xff, 0xf0, 


//SSYMB_INPUTFIELD_DESCR_SYMBOL w:10 h:12 id:7
//          
//          
//          
//  ####    
//  #   #   
//  #   # # 
//  ####  # 
//  #   #   
//  #   # # 
//  #   # # 
//  ####    
//          
0x00, 0x00, 
0x00, 0x00, 
0x1f, 0xe0, 
0x12, 0x20, 
0x12, 0x20, 
0x12, 0x20, 
0x0d, 0xc0, 
0x00, 0x00, 
0x06, 0xc0, 
0x00, 0x00, 


//SSYMB_INPUTFIELD_KEY_SYMBOL w:10 h:12 id:8
//          
//          
//          
//  #   #   
//  #  #    
//  # #   # 
//  ##    # 
//  ##      
//  # #   # 
//  #  #  # 
//  #   #   
//          
0x00, 0x00, 
0x00, 0x00, 
0x1f, 0xe0, 
0x03, 0x00, 
0x04, 0x80, 
0x08, 0x40, 
0x10, 0x20, 
0x00, 0x00, 
0x06, 0xc0, 
0x00, 0x00, 


//SSYMB_FUNCTIONIMG_VIEWKEY w:21 h:10 id:9
//                    #
//        ####        #
//    ############    #
//  ##  ####  ##  ##  #
// #     ######     # #
//  ##     ##     ##  #
//    ####    ####    #
//        ####        #
//                    #
//#####################
0x00, 0x40, 
0x08, 0x40, 
0x14, 0x40, 
0x14, 0x40, 
0x22, 0x40, 
0x22, 0x40, 
0x32, 0x40, 
0x3a, 0x40, 
0x79, 0x40, 
0x7d, 0x40, 
0x6d, 0x40, 
0x69, 0x40, 
0x3a, 0x40, 
0x32, 0x40, 
0x22, 0x40, 
0x22, 0x40, 
0x14, 0x40, 
0x14, 0x40, 
0x08, 0x40, 
0x00, 0x40, 
0xff, 0xc0, 


//SSYMB_FUNCTIONIMG_ADDKEY w:21 h:10 id:10
//#                   #
//#    ######         #
//#    #    #   #     #
//#    ######   #     #
//#       #   #####   #
//#      ##     #     #
//#       #     #     #
//#     ###           #
//#                   #
//#####################
0xff, 0xc0, 
0x00, 0x40, 
0x00, 0x40, 
0x00, 0x40, 
0x00, 0x40, 
0x70, 0x40, 
0x51, 0x40, 
0x55, 0x40, 
0x5f, 0x40, 
0x50, 0x40, 
0x70, 0x40, 
0x00, 0x40, 
0x08, 0x40, 
0x08, 0x40, 
0x3e, 0x40, 
0x08, 0x40, 
0x08, 0x40, 
0x00, 0x40, 
0x00, 0x40, 
0x00, 0x40, 
0xff, 0xc0, 


//SSYMB_FUNCTIONIMG_CONFIG w:21 h:10 id:11
//#                   #
//#   ### ####  ###   #
//#  #    #    #   #  #
//# #     ###  #      #
//# #     #    # ###  #
//#  #    #    #   #  #
//#   ### #     ###   #
//#                   #
//#                   #
//#####################
0xff, 0xc0, 
0x00, 0x40, 
0x18, 0x40, 
0x24, 0x40, 
0x42, 0x40, 
0x42, 0x40, 
0x42, 0x40, 
0x00, 0x40, 
0x7e, 0x40, 
0x50, 0x40, 
0x50, 0x40, 
0x40, 0x40, 
0x00, 0x40, 
0x3c, 0x40, 
0x42, 0x40, 
0x4a, 0x40, 
0x4a, 0x40, 
0x2c, 0x40, 
0x00, 0x40, 
0x00, 0x40, 
0xff, 0xc0, 


//SSYMB_FUNCTIONIMG_LEFT w:12 h:7 id:12
//#   ##     #
//#  ####### #
//# ######## #
//#  ####### #
//#   ##     #
//#          #
//############
0xfe, 
0x02, 
0x22, 
0x72, 
0xfa, 
0xfa, 
0x72, 
0x72, 
0x72, 
0x72, 
0x02, 
0xfe, 


//SSYMB_FUNCTIONIMG_RIGHT w:12 h:7 id:13
//#     ##   #
//# #######  #
//# ######## #
//# #######  #
//#     ##   #
//#          #
//############
0xfe, 
0x02, 
0x72, 
0x72, 
0x72, 
0x72, 
0xfa, 
0xfa, 
0x72, 
0x22, 
0x02, 
0xfe, 


//SSYMB_FUNCTIONIMG_UP w:7 h:12 id:14
//#######
//#      
//#   #  
//#  ### 
//# #####
//# #####
//#  ### 
//#  ### 
//#  ### 
//#  ### 
//#      
//#######
0xff, 0xf0, 
0x80, 0x10, 
0x8c, 0x10, 
0x9f, 0xd0, 
0xbf, 0xd0, 
0x9f, 0xd0, 
0x8c, 0x10, 


//SSYMB_FUNCTIONIMG_DOWN w:7 h:12 id:15
//#######
//#      
//#  ### 
//#  ### 
//#  ### 
//#  ### 
//# #####
//# #####
//#  ### 
//#   #  
//#      
//#######
0xff, 0xf0, 
0x80, 0x10, 
0x83, 0x10, 
0xbf, 0x90, 
0xbf, 0xd0, 
0xbf, 0x90, 
0x83, 0x10, 


//SSYMB_FUNCTIONIMG_KBHIT w:17 h:7 id:16
//#  ###  ### #   #
//# #     #   #   #
//#  ###  ### #   #
//#     # #   #   #
//#  ###  ### ### #
//#               #
//#################
0xfe, 
0x02, 
0x42, 
0xaa, 
0xaa, 
0xaa, 
0x12, 
0x02, 
0xfa, 
0xaa, 
0xaa, 
0x02, 
0xfa, 
0x0a, 
0x0a, 
0x02, 
0xfe, 


//SSYMB_FUNCTIONIMG_BACK w:16 h:7 id:17
//#     ####     #
//#         #    #
//#       # #    #
//#      ###     #
//#     #####    #
//#              #
//################
0xfe, 
0x02, 
0x02, 
0x02, 
0x02, 
0x02, 
0x8a, 
0x9a, 
0xba, 
0x9a, 
0x6a, 
0x02, 
0x02, 
0x02, 
0x02, 
0xfe, 


//SSYMB_SMALLNUMBER_0 w:4 h:7 id:18
//    
//### 
//# # 
//# # 
//# # 
//### 
//    
0x7c, 
0x44, 
0x7c, 
0x00, 


//SSYMB_SMALLNUMBER_1 w:4 h:7 id:19
//    
//##  
// #  
// #  
// #  
//### 
//    
0x44, 
0x7c, 
0x04, 
0x00, 


//SSYMB_SMALLNUMBER_2 w:4 h:7 id:20
//    
// #  
//# # 
//  # 
// #  
//### 
//    
0x24, 
0x4c, 
0x34, 
0x00, 


//SSYMB_SMALLNUMBER_3 w:4 h:7 id:21
//    
//### 
//  # 
//### 
//  # 
//### 
//    
0x54, 
0x54, 
0x7c, 
0x00, 


//SSYMB_SMALLNUMBER_4 w:4 h:7 id:22
//    
//  # 
// #  
//# # 
//### 
//  # 
//    
0x18, 
0x28, 
0x5c, 
0x00, 


//SSYMB_SMALLNUMBER_5 w:4 h:7 id:23
//    
//### 
//#   
//##  
//  # 
//##  
//    
0x74, 
0x54, 
0x48, 
0x00, 


//SSYMB_SMALLNUMBER_6 w:4 h:7 id:24
//    
//#   
//#   
//### 
//# # 
//### 
//    
0x7c, 
0x14, 
0x1c, 
0x00, 


//SSYMB_SMALLNUMBER_7 w:4 h:7 id:25
//    
//### 
//  # 
//  # 
// #  
// #  
//    
0x40, 
0x4c, 
0x70, 
0x00, 


//SSYMB_SMALLNUMBER_8 w:4 h:7 id:26
//    
//### 
//# # 
//### 
//# # 
//### 
//    
0x7c, 
0x54, 
0x7c, 
0x00, 


//SSYMB_SMALLNUMBER_9 w:4 h:7 id:27
//    
//### 
//# # 
//### 
//  # 
//### 
//    
0x74, 
0x54, 
0x7c, 
0x00, 


//SSYMB_SMALLABC_A w:4 h:7 id:28
//    
// #  
//# # 
//### 
//# # 
//# # 
//    
0x3c, 
0x50, 
0x3c, 
0x00, 


//SSYMB_SMALLABC_B w:4 h:7 id:29
//    
//##  
//# # 
//##  
//# # 
//##  
//    
0x7c, 
0x54, 
0x28, 
0x00, 


//SSYMB_SMALLABC_C w:4 h:7 id:30
//    
// ## 
//#   
//#   
//#   
// ## 
//    
0x38, 
0x44, 
0x44, 
0x00, 


//SSYMB_SMALLABC_D w:4 h:7 id:31
//    
//##  
//# # 
//# # 
//# # 
//##  
//    
0x7c, 
0x44, 
0x38, 
0x00, 


//SSYMB_SMALLABC_E w:4 h:7 id:32
//    
//### 
//#   
//### 
//#   
//### 
//    
0x7c, 
0x54, 
0x54, 
0x00, 


//SSYMB_SMALLABC_F w:4 h:7 id:33
//    
//### 
//#   
//##  
//#   
//#   
//    
0x7c, 
0x50, 
0x40, 
0x00, 


//SSYMB_SMALLNUMBER_SPLIT w:6 h:7 id:34
//      
//    # 
//   #  
//  #   
// #    
//#     
//      
0x04, 
0x08, 
0x10, 
0x20, 
0x40, 
0x00, 
};

const GFXimage systemsymb_Descriptors[] = {
{ 10, 13, 0, 0, 0},
{ 10, 13, 0, 0, 20},
{ 10, 13, 0, 0, 40},
{ 10, 8, 0, 0, 60},
{ 20, 13, 0, 0, 70},
{ 10, 8, 0, 0, 110},
{ 1, 12, 0, 0, 120},
{ 10, 12, 0, 0, 122},
{ 10, 12, 0, 0, 142},
{ 21, 10, 0, 0, 162},
{ 21, 10, 0, 0, 204},
{ 21, 10, 0, 0, 246},
{ 12, 7, 0, 0, 288},
{ 12, 7, 0, 0, 300},
{ 7, 12, 0, 0, 312},
{ 7, 12, 0, 0, 326},
{ 17, 7, 0, 0, 340},
{ 16, 7, 0, 0, 357},
{ 4, 7, 0, 0, 373},
{ 4, 7, 0, 0, 377},
{ 4, 7, 0, 0, 381},
{ 4, 7, 0, 0, 385},
{ 4, 7, 0, 0, 389},
{ 4, 7, 0, 0, 393},
{ 4, 7, 0, 0, 397},
{ 4, 7, 0, 0, 401},
{ 4, 7, 0, 0, 405},
{ 4, 7, 0, 0, 409},
{ 4, 7, 0, 0, 413},
{ 4, 7, 0, 0, 417},
{ 4, 7, 0, 0, 421},
{ 4, 7, 0, 0, 425},
{ 4, 7, 0, 0, 429},
{ 4, 7, 0, 0, 433},
{ 6, 7, 0, 0, 437},
};


//Additional bitmaps for the keyboardlayout (images for patternreferences)
const uint8_t keyboardpatternsymb_Bitmaps[] /*PROGMEM*/ = {
//Pattern a-z w:10 h:13
//          
//  ##      
//    #     
//  ###     
// #  #     
//  ###     
//          
//          
//   ####   
//     #    
//    #     
//   ####   
//          
0x00, 0x00, 
0x08, 0x00, 
0x54, 0x00, 
0x54, 0x90, 
0x3c, 0xb0, 
0x00, 0xd0, 
0x00, 0x90, 
0x00, 0x00, 
0x00, 0x00, 
0x00, 0x00, 


//Pattern A-Z w:10 h:13
//          
//  ##      
// #  #     
// ####     
// #  #     
// #  #     
//          
//   #####  
//      #   
//     #    
//    #     
//   #####  
//          
0x00, 0x00, 
0x3c, 0x00, 
0x50, 0x00, 
0x51, 0x10, 
0x3d, 0x30, 
0x01, 0x50, 
0x01, 0x90, 
0x01, 0x10, 
0x00, 0x00, 
0x00, 0x00, 


//Special 1 w:10 h:13
//          
//          
// #    #   
// #   ##   
// #    #   
// #    #   
// #    #   
// #    #   
//      #   
// #   ###  
//          
//          
//          
0x00, 0x00, 
0x3f, 0x40, 
0x00, 0x00, 
0x00, 0x00, 
0x00, 0x00, 
0x10, 0x40, 
0x3f, 0xc0, 
0x00, 0x40, 
0x00, 0x00, 
0x00, 0x00, 


//Special 2 w:10 h:13
//          
//  #       
// ##    #  
//  #   #   
//  #  #    
// ####     
//   # ##   
//  # #  #  
//    #  #  
//      #   
//     #    
//    ####  
//          
0x00, 0x00, 
0x24, 0x00, 
0x7d, 0x00, 
0x06, 0x00, 
0x05, 0x90, 
0x0a, 0x30, 
0x12, 0x50, 
0x21, 0x90, 
0x00, 0x00, 
0x00, 0x00, 
};

const GFXimage keyboardpatternsymb_Descriptors[] = {
{ 10, 13, 0, 0, 0},
{ 10, 13, 0, 0, 20},
{ 10, 13, 0, 0, 40},
{ 10, 13, 0, 0, 60},
};


/************** FONT SUMMARY ****************************/
const GFXfont gfxfonts[]  = {
  { 
    (const uint8_t  *)&systemfont_Bitmaps,
    (const GFXimage *)&systemfont_Descriptors,
    (const uint16_t)256
  }
};

const GFXimagelist gfximages[]  = {
  { 
    (const uint8_t  *)&systemsymb_Bitmaps,
    (const GFXimage *)&systemsymb_Descriptors,
    (const uint16_t)28
  },
  { 
    (const uint8_t  *)&keyboardpatternsymb_Bitmaps,
    (const GFXimage *)&keyboardpatternsymb_Descriptors,
    (const uint16_t)4
  }
};
