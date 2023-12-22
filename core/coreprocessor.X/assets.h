/* 
 * File:   assets_localized.h
 * Author: greengravity
 *
 * Created on 18. Dezember 2023, 12:51
 */

#ifndef ASSETS_H
#define	ASSETS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "assets_common.h"

//available assetpacks
#define ASSETPACK_english_en_US 0
#define ASSETPACK_german_de_DE 1
#define ASSETPACK_english_en_GB 2   
    


//set assetpack for compilation
#define ASSETPACK ASSETPACK_german_de_DE
//#define ASSETPACK ASSETPACK_english_ENG_uk
//#define ASSETPACK ASSETPACK_english_ENG_us
    
       

#include "assets/assets_english_en-US.h"
#include "assets/assets_german_de-DE.h"    
#include "assets/assets_english_en-GB.h"   	

#ifdef	__cplusplus
}
#endif

#endif	/* ASSETS_LOCALIZED_H */

