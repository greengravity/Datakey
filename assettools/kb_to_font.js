// Read a keyboardlayout (.klc file) and create the keycodes.json template as well 
// as a configfile for the bitmapfont generator
// the patterns.json have to be created manually anyway
// a keyboard layout can be generated/loaded with Microsoft Keyboard Layout Creator 1.4

const path = require('path');
const fs = require("fs")
const Jimp = require('jimp');
const { resolve } = require('path');


if ( process.argv.length < 3 ) {
	console.log( "Add fontname as argument" )
	return
}

const fontname = process.argv[2]


const locals = JSON.parse( fs.readFileSync('./locals/locals.json', 'utf8') );
const local = locals.find( l=> l.name === fontname )
if ( !local ) {
	console.log( "fontname not found" )
	return
}

const icondir =      './icons'
const layoutfile =   './locals/fonts/' + local.fontfolder + '/kblayout.klc';
const patterntemplatefile =  './locals/fonts/' + local.fontfolder + '/patterntemplates.json';
const keycodefile =  './locals/fonts/' + local.fontfolder + '/keycodes.json';
const fontfile =     './locals/fonts/' + local.fontfolder + '/font.bmfc';
const font_templatefile =  './font_template.bmfc';

let kblayout = fs.readFileSync(layoutfile, 'utf16le').split( '\r\n' )


const PM_IGNORE = 0
const PM_SHIFTSTATE = 1
const PM_LAYOUT = 2
const PM_DEADKEY = 3
const PM_END = 4



let  = []

let parsestate = { 
	mode: PM_IGNORE,
	lheader: null,
	deadkey: ''
}


//Mapping table from windows scanncode to microchip scanncode
let scancodemap = [
	{ kblcode: 0x29, mccode: 0x35 },
	{ kblcode: 0x02, mccode: 0x1e },
	{ kblcode: 0x03, mccode: 0x1f },
	{ kblcode: 0x04, mccode: 0x20 },
	{ kblcode: 0x05, mccode: 0x21 },
	{ kblcode: 0x06, mccode: 0x22 },
	{ kblcode: 0x07, mccode: 0x23 },
	{ kblcode: 0x08, mccode: 0x24 },
	{ kblcode: 0x09, mccode: 0x25 },
	{ kblcode: 0x0a, mccode: 0x26 },
	{ kblcode: 0x0b, mccode: 0x27 },
	{ kblcode: 0x0c, mccode: 0x2d },
	{ kblcode: 0x0d, mccode: 0x2e },   
	{ kblcode: 0x10, mccode: 0x14 },
	{ kblcode: 0x11, mccode: 0x1A },
	{ kblcode: 0x12, mccode: 0x08 },
	{ kblcode: 0x13, mccode: 0x15 },
	{ kblcode: 0x14, mccode: 0x17 },
	{ kblcode: 0x15, mccode: 0x1c },
	{ kblcode: 0x16, mccode: 0x18 },
	{ kblcode: 0x17, mccode: 0x0c },
	{ kblcode: 0x18, mccode: 0x12 },
	{ kblcode: 0x19, mccode: 0x13 },
	{ kblcode: 0x1a, mccode: 0x2f },
	{ kblcode: 0x1b, mccode: 0x30 },
	{ kblcode: 0x2b, mccode: 0x31 },
	{ kblcode: 0x1e, mccode: 0x04 },
	{ kblcode: 0x1f, mccode: 0x16 },
	{ kblcode: 0x20, mccode: 0x07 },
	{ kblcode: 0x21, mccode: 0x09 },
	{ kblcode: 0x22, mccode: 0x0a },
	{ kblcode: 0x23, mccode: 0x0b },
	{ kblcode: 0x24, mccode: 0x0d },
	{ kblcode: 0x25, mccode: 0x0e },
	{ kblcode: 0x26, mccode: 0x0f },
	{ kblcode: 0x27, mccode: 0x33 },
	{ kblcode: 0x28, mccode: 0x34 },
	{ kblcode: 0x2c, mccode: 0x1d },
	{ kblcode: 0x2d, mccode: 0x1b },
	{ kblcode: 0x2e, mccode: 0x06 },
	{ kblcode: 0x2f, mccode: 0x19 },
	{ kblcode: 0x30, mccode: 0x05 },
	{ kblcode: 0x31, mccode: 0x11 },
	{ kblcode: 0x32, mccode: 0x10 },
	{ kblcode: 0x33, mccode: 0x36 },
	{ kblcode: 0x34, mccode: 0x37 },
	{ kblcode: 0x35, mccode: 0x38 },
	{ kblcode: 0x39, mccode: 0x2c },
	{ kblcode: 0x53, mccode: 0x63 },
	{ kblcode: 0x56, mccode: 0x64 }	
]
	
	

let shiftStates = [
	{ id: "0", modcode: 0x00, col:0 },
	{ id: "1", modcode: 0x02, col:1 }, //shift
	{ id: "2", modcode: 0x01, col:2 }, //cntrl
	{ id: "6", modcode: 0x40, col:3 }, //cntrl alt
	{ id: "7", modcode: 0x42, col:4 }, //shift cntrl alt
]


//parsing file
console.log( "Parsing Keyboardlayout" )
let layout = []
let deadkeys = []

const addLayout = (layout, entry ) => {
	if ( !layout.find( e=> e.uccp === entry.uccp ) )
		
	layout.push( entry )	
}



kblayout.forEach(kbl=> {
	if ( parsestate.mode === PM_END ) return
	
	if ( kbl.length > 0 ) {
		if ( kbl.startsWith("KBD") ) { 
			parsestate.mode = PM_IGNORE
		} else if ( kbl.startsWith("COPYRIGHT") ) {	
			parsestate.mode = PM_IGNORE		
		} else if ( kbl.startsWith("COMPANY") ) { 
			parsestate.mode = PM_IGNORE		
		} else if ( kbl.startsWith("LOCALNAME") ) {	
			parsestate.mode = PM_IGNORE		
		} else if ( kbl.startsWith("VERSION") ) { 
			parsestate.mode = PM_IGNORE		
		} else if ( kbl.startsWith("SHIFTSTATE") ) { 
			parsestate.mode = PM_SHIFTSTATE					
		} else if ( kbl.startsWith("LAYOUT") ) {
			parsestate.mode = PM_LAYOUT
		} else if ( kbl.startsWith("DEADKEY") ) {
			parsestate.mode = PM_DEADKEY
			parsestate.deadkey = parseInt( kbl.split("\t")[1], 16 );
		} else if ( kbl.startsWith("KEYNAME") ) { 
			parsestate.mode = PM_IGNORE
		} else if ( kbl.startsWith("KEYNAME_EXT") ) { 
			parsestate.mode = PM_IGNORE
		} else if ( kbl.startsWith("KEYNAME_DEAD") ) { 
			parsestate.mode = PM_IGNORE
		} else if ( kbl.startsWith("DESCRIPTIONS") ) { 
			parsestate.mode = PM_IGNORE
		} else if ( kbl.startsWith("LANGUAGENAMES") ) { 
			parsestate.mode = PM_IGNORE
		} else if ( kbl.startsWith("ENDKBD") ) { 
			parsestate.mode = PM_END
		} else {
			if ( parsestate.mode === PM_SHIFTSTATE ) {
				//Check if the shiftstate map in header matches our expectations, otherwise make an error
				if ( !( ( kbl === "0\t//Column 4" ) ||
						( kbl === "1\t//Column 5 : Shft" ) ||
						( kbl === "2\t//Column 6 :       Ctrl" ) ||
						( kbl === "6\t//Column 7 :       Ctrl Alt" ) ||
						( kbl === "7\t//Column 8 : Shft  Ctrl Alt" ) ) ) {
					console.log("Error, shiftstate unknown")
					parsestate.parseError = true
					parsestate.mode = PM_END
				}				 					
			} else if ( parsestate.mode === PM_LAYOUT ) {		
				if ( !parsestate.lheader ) {
					//reading the headerline of the layout section
					
					if ( kbl.startsWith( '//SC' ) ) {
						let lheader = kbl.split('\t')
						parsestate.lheader = { }
						
						lheader.forEach( (he, i) => {
							if ( he.search("SC") >= 0 ) {
								parsestate.lheader.scancode = i
							} else if (he === 'VK_' ) {
								parsestate.lheader.vk = i
							} else if (he === 'Cap' ) {
								parsestate.lheader.cap = i
							}																				
						})											
						
						if ( parsestate.lheader.scancode === undefined ) {
							parsestate.parseError = true
						}
						
						if ( parsestate.parseError ) {
							console.log("Error while parsing layoutheader")
							parsestate.mode = PM_END
						}												
					}
				} else {
					//parsing a layoutline in order to get the scanncode, the shiftstate and the corresponding character
					
					if ( kbl.startsWith( '//' ) ) return
					
					let layoutline = kbl.split('\t')
					
					let kblcode = parseInt( layoutline[ parsestate.lheader.scancode ], 16 )
					let map = scancodemap.find( scm=> scm.kblcode === kblcode )
					
										
					shiftStates.forEach( s=> {
						let entry = { 
							kblcode: map.kblcode,
							mccode: map.mccode,
							uccp: -1						
						}						
						
						let chardef = ''
					
						//This is a bit weird behaviour of the klc file
						//It seems like the VK_ columns messes up the tabulatorsplits and breaks the columncount
						//Thats why we count the columns from the right side of the list, starting at the //
						let commentcol = layoutline.findIndex( l=> l.startsWith('//') )
						if ( commentcol > 0 ) { 
							chardef = layoutline[ ( commentcol - ( 1 + shiftStates.length ) ) + s.col]							
						}
							
						let isDeadkey = false;
						
						entry.modcode = s.modcode
						entry.uccp = -1
						if ( chardef.length === 1 ) {
							entry.uccp = chardef.codePointAt(0)
						} else if ( chardef.length === 4 || ( chardef.length === 5 && chardef[4] === '@' ) ) {							
							if ( chardef.length === 5 ) {
								isDeadkey = true
								chardef  = chardef.substr(0,4)
							}									
							entry.uccp = parseInt( chardef, 16 )
						}
						
						if ( entry.uccp != -1 ) {
							if ( isDeadkey ) {
								deadkeys.push( entry )
							} else {
								addLayout(layout, entry )
							}
						}
					})
					
					
					
					
				}
				 
			} else if ( parsestate.mode === PM_DEADKEY ) {
				//deadkey transformations like a -> â
				let layoutline = kbl.split('\t')
				
				let uccpbase = parseInt( layoutline[0], 16 )
				let uccptrans = parseInt( layoutline[1], 16 ) 
								
				baseentry = layout.find( l=> l.uccp === uccpbase )
								
				let dk = deadkeys.find( dk=> dk.uccp === parsestate.deadkey )
				if ( dk ) {
					let entry = Object.assign( {}, dk, { modcode2: baseentry.modcode, mccode2: baseentry.mccode, uccp: uccptrans } )
					addLayout(layout, entry )
				}				
			}
		}			
	}	
}) 
	
	
if ( !parsestate.parseError ) {
	let keycodes = []
	
	const getHexByteVal = (b) => {
		let xv = b.toString(16)
		return '0x' + (xv.length < 2 ? '0' : '') + xv
	}

	//insert the "Enter" keycode, because its a special char and wont not be in the layoutlist
	let keycode = {
		char: "\n",
		code: [ "0x00", "0x28", "0x00", "0x00" ]
	}
	keycodes.push( keycode )
	
	const filterval = "\\\\u"
		
	//filtering characters that has no glyph to be displayed
	layout = layout.filter(l=> {		
		l.character = String.fromCodePoint( l.uccp )
			
		let str = JSON.stringify( { c: l.character } )
		if ( str.search( filterval ) < 0 ) {		
			return true
		}
		return false
	})
	
	const charlist = layout
						.filter( l => l.character != '\n' )
						.sort((a,b) => { return a.uccp - b.uccp } )
						.map( l=> ("" + l.uccp) )
						.join( ',' )
		
	let fontconfig = fs.readFileSync(font_templatefile, 'utf8')
	fontconfig = fontconfig.replace( "CHARACTERLISTHERE", charlist )
	fs.writeFileSync( fontfile, fontconfig )
	
	//convert to keycodestructure
	layout.forEach(l=> {		
		let keycode = {
			char: l.character,
			code: [ getHexByteVal( l.modcode ), getHexByteVal( l.mccode ), getHexByteVal( l.modcode2 ? l.modcode2 : 0x00 ), getHexByteVal( l.mccode2 ? l.mccode2 : 0x00 ) ]
		}
		
		keycodes.push( keycode )
	})
	
	const kdata = '{\n "keycodes": [\n\t' + keycodes.map(k=>JSON.stringify( k )).join(',\n\t') + '\n]\n}'
	
	fs.writeFileSync(keycodefile, kdata, { encoding: 'utf8' } )


	//convert to patterntemplate
	let patterntempl = []
	layout.forEach(l=> {		
		let pt = {
			f: "CHAR",
			id: l.character						
		}
		
		patterntempl.push( JSON.stringify( pt ) )
	})
	
	const tpldata = '[\n' + patterntempl.join(',\n') + '\n]'  
	fs.writeFileSync(patterntemplatefile, tpldata, { encoding: 'utf8' } )

}



