const path = require('path');
const cp = require('codepage');
const fs = require("fs")
const Jimp = require('jimp');
const { resolve } = require('path');
const { getSystemErrorMap } = require('util');


const codepage = 1252;


const assetfolder = '../core/coreprocessor.X'
const fontdir = 'german1_15'

const icondir =      './icons'
const fontfile =     './fontmaps/' + fontdir + '/font.fnt';
const fontmap =      './fontmaps/' + fontdir + '/font_0.png';
const patternfile =  './fontmaps/' + fontdir + '/patterns.json';
const textfile =     './fontmaps/' + fontdir + '/texte.json';
//const outfilec =     './assets/'   + fontdir + '/assets.c'
//const outfileh =     './assets/'   + fontdir + '/assets.h'

const outfilec =     assetfolder + '/assets.c'
const outfileh =     assetfolder + '/assets.h'

let content = fs.readFileSync(fontfile, 'utf8');
let clines = content.split('\r\n')

const linefeed = '\n';
const space = ' ';
const outcp = cp[codepage];
let linefeedindex = -1;
let spaceindex = -1;

//Reading fontdescription
let charlist = [];
clines.forEach(ln => {
  if (ln.startsWith('char id=')) {
    let character = {}

    let props = ln.split(' ');
    props.forEach(p => {
      if (p.length > 0) {
        let prop = p.split('=');
        character[prop[0]] = prop[1];
      }
    })

    character.id = parseInt(character.id, 10)
    character.x = parseInt(character.x, 10)
    character.y = parseInt(character.y, 10)
    character.width = parseInt(character.width, 10)
    character.height = parseInt(character.height, 10)
    character.xoffset = parseInt(character.xoffset, 10)
    if (character.xoffset < 0) character.xoffset = 0;
    character.yoffset = parseInt(character.yoffset, 10)
    character.xadvance = parseInt(character.xadvance, 10)

    if (character.xadvance < character.width + character.xoffset) {
      character.xadvance = character.width + character.xoffset
    }

    character.charval = outcp.dec[character.id]
    charlist.push(character);
  }
})


// Reading icondescription
const iconjson = JSON.parse( fs.readFileSync(path.resolve(icondir, 'icons.js'), { encoding: "utf-8" }) )

// Reading patterndescription
const patterns = JSON.parse( fs.readFileSync( patternfile, { encoding: "utf-8" }) )

if ( patterns.patternmap.length == 9 ) {
  let syspattern =  {
    "name": "SYSTEMMENU",
    "text": "Menu",
    "keys": [
      [
        { "f": "OK" },
        { "f": "BACK" },
      ],
    ]
  }

  patterns.patterns.push( syspattern )

  patterns.patterns.forEach(pm=> {
    while ( pm.keys.length < 4 ) pm.keys.push([])
    
    pm.keys.forEach(pk=> {
      let pcount = 0

      pk.forEach(pk2=> {
        pcount+= pk2.size ? pk2.size : 1
      })            

      if ( pcount < 10 ) {
        pk.push( { f:"EMPTY", size: ( 10-pcount ) } )
      }    
    })

  }) 
}


// Reading Textinfos
const textdata = JSON.parse( fs.readFileSync( textfile, { encoding: "utf-8" }) )


// Reading imagefiles
let readpromises = []
{
  const pr = new Promise((resolve, reject) => {
    Jimp.read(fontmap)
      .then(image => {
        // Do stuff with the image.
        charlist.forEach(c => {
          c.glyph = [];

          for (let i = 0; i < c.width; i++) {
            let newarr = []
            for (let j = 0; j < c.height; j++) {
              newarr.push(0)
            }
            c.glyph.push(newarr)
          }

          image.scan(
            c.x,
            c.y,
            c.width,
            c.height,
            (x, y, idx) => {
              let xabs = (x - c.x);
              let yabs = (y - c.y) + c.yoffset;

              if (xabs >= 0 && xabs < c.width &&
                yabs >= 0 && yabs < c.height) {

                let red = image.bitmap.data[idx + 0];
                let green = image.bitmap.data[idx + 1];
                let blue = image.bitmap.data[idx + 2];

                c.glyph[xabs][yabs] = [red, green, blue]
              }
            }
          )

          resolve()
        })
      }).catch(error => {
        reject(error)
      })
  })
  readpromises.push(pr)
}

let icons = iconjson.map(jicon => {

  const icon = Object.assign({ imgdata: [], path: path.resolve(icondir, jicon.icon) }, jicon)

  const pr = new Promise((resolve, reject) => {
    Jimp.read(icon.path)
      .then(image => {
        // Do stuff with the image.
        for (let i = 0; i < image.bitmap.width; i++) {
          let newarr = []
          for (let j = 0; j < image.bitmap.height; j++) {
            newarr.push(0)
          }
          icon.imgdata.push(newarr)
        }

        icon.width = image.bitmap.width
        icon.height = image.bitmap.height

        image.scan(
          0, 0, image.bitmap.width, image.bitmap.height,
          (x, y, idx) => {

            let red = image.bitmap.data[idx + 0];
            let green = image.bitmap.data[idx + 1];
            let blue = image.bitmap.data[idx + 2];

            icon.imgdata[x][y] = [red, green, blue]
          }
        )

        resolve()
      }).catch((err) => {
        console.log('Error read image ' + icon.path)
        console.log(err)
        reject()
      })

  })
  readpromises.push(pr)
  return icon
})


// Generating C Source and Headerfiles
Promise.all(readpromises).then((values) => {

  const escapeOutCharval = (c) => {
    if ( c === "\\" ) return "Backslash"
    return c
  }

  const getHexByteVal = (b) => {
    let xv = b.toString(16)
    return '0x' + (xv.length < 2 ? '0' : '') + xv
  }

  const rgbToByteVal = ( rgb ) => {  
    let sval = ( ( rgb[0] & 0xf8 ) << 8 ) |
               ( ( rgb[1] & 0xfc ) << 3 ) |
               ( ( rgb[2] & 0xf8 ) >> 3 )



    let hexval = sval.toString(16)
    while ( hexval.length < 4 ) { hexval = '0' + hexval }      
    return '0x' + hexval
  }


  let imgcount = 0
  let pos = 0
  let outdatac = []
  let outdatah = []
  let maximgbuffer = 0
  let imglist = []

  
  outdatac.push('#include "assets.h"')
  outdatac.push('')
  outdatac.push('// Systemfont: ' + charlist[0].height + ' pixels high')
  outdatac.push('const uint16_t bitmapdata[] = ')
  outdatac.push('{')

  charlist.forEach((oc, index) => {
//    oc.pos = pos    
    oc.imgid = imgcount
    imgcount++    

    imglist.push({
      color : false,
      width: oc.width,
      height: oc.height,
      imgoffs: pos,      
    })

    outdatac.push('// @' + pos + '"' + oc.charval + '" (' + oc.width + ' pixels wide)')
    let binline = ''
    for (let y = 0; y < oc.height; y++) {
      let line = ''
      for (let x = 0; x < oc.width; x++) {
        line += oc.glyph[x][y][0] > 150 ? '#' : ' '
        if (index > 0 || y > 0 || x > 0) binline += ','
        binline += rgbToByteVal(oc.glyph[x][y]) 
        pos ++
      }
      outdatac.push('// ' + line)
    }
    outdatac.push(binline)
  })

  //Add an empty icon at position 0 to assign to dummy chars like linefeed and zero
  icons.unshift({ name:"EMPTY", width:0, height:0 })

  encodeerr_imgid = 0;
  icons.forEach((icon, index) => {
    icon.imgid = imgcount
    imgcount++

    if ( icon.name === "encodeerr" ) { encodeerr_imgid = imgcount }

    imglist.push({
      color : icon.color,
      width: icon.width,
      height: icon.height,
      imgoffs: pos,      
    })

    outdatac.push('// @' + pos + '"' + icon.name + '" (' + icon.width + ' pixels wide / ' + icon.height + ' pixels high)')
    let binline = ''
    for (let y = 0; y < icon.height; y++) {
      let line = ''
      for (let x = 0; x < icon.width; x++) {
        line += icon.imgdata[x][y][0] > 150 ? '#' : ' '
        binline += ','

        binline += rgbToByteVal(icon.imgdata[x][y])
        pos ++
      }
      outdatac.push('// ' + line)
    }    
    outdatac.push(binline)

  })

  imglist.forEach( img => {
    let imgbuffer = img.width * img.height
    if (imgbuffer > maximgbuffer) maximgbuffer = imgbuffer
  })


  outdatac.push('};')
  outdatac.push('')
  outdatac.push('')
  outdatac.push('const GFXimage bitmaps[] = ')
  outdatac.push('{')
  imglist.forEach((img, index, arr) => {
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')
    outdatac.push('  ' + '{ ' + img.width + ', ' + img.height + ', ' + img.imgoffs + ' }' + betweenkomma )
  })  
  outdatac.push('};')  
  outdatac.push('')

  outdatac.push('const GFXChar gfxchars[] = ')
  outdatac.push('{')

  charlist.unshift( { imgid: encodeerr_imgid, xoffset:0, xadvance:15, id:65533, charval: "encodeerr" } ) //Encode error character'
  charlist.unshift( { imgid: 0, xoffset:0, xadvance:0, id:10, charval: "linefeed" } ) //linefeed character'
  charlist.unshift( { imgid: 0, xoffset:0, xadvance:0, id:0, charval: "zero" } ) //dummy zero character for zero terminated strings'
  
  charlist.forEach((ch, index, arr) => {
    ch.charid = index
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')
    outdatac.push('  ' + '{ ' + ch.imgid + ',' + ch.xoffset + ', ' + ch.xadvance + ', ' + ch.id + ' }' + betweenkomma + ' // ' + escapeOutCharval( ch.charval ) )
  })  
  outdatac.push('};')  
  outdatac.push('')



  charlist.sort((a,b)=> {
    return a.id - b.id
  })
  outdatac.push('const Unicodelist unicodes[] = ')
  outdatac.push('{')
  charlist.forEach((ch, index, arr ) => {
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')
    outdatac.push('  ' + '{ ' + ch.id + ',' + ch.charid + ' }' + betweenkomma + ' // ' + escapeOutCharval( ch.charval ) )
  })
  outdatac.push('};')  
  outdatac.push('')

  
  let mapids = {}
  let mapidx = 0

  patterns.patternmap.forEach( (p,index)=> {
    if ( p && p != "" ) {      
      if ( !mapids[p] ) {
        let pattern = patterns.patterns.find( kp=> kp.name === p ) 
        if ( pattern ) {
          mapids[p] = { idx: mapidx, text: pattern.text }
          mapidx++
        }        
      }
    }
  })


  outdatac.push('const Keyboardmaps keymaps[] = ')
  outdatac.push('{')


  patterns.patternmap.forEach( (p,index, arr)=> {
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')
    
    if ( p && p!= "" && mapids[p] ) {

      let name = []
      for ( let i=0;i<5;i++) {
        if ( mapids[p].text.length > i ) {
          let ch = mapids[p].text.charAt(i)
          let c = charlist.find( c=> c.charval === ch )
          if ( c ) {
            name.push( getHexByteVal(c.charid) )
          } else {
            name.push( '0x00' )
          }
        } else {
          name.push( '0x00' )
        }
      }

      outdatac.push('  ' + '{ true,  ' + '{ ' + name.join(', ') + ' }, ' + ( mapids[p].idx * 40 ) + ' }' + betweenkomma + ' // ' + mapids[p].text )

    } else {

      outdatac.push('  ' + '{ false, ' + '{ 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }' + betweenkomma + ' // empty' )

    }
  })
  outdatac.push('};')
  outdatac.push('')

  let defaultkmap
  let keyfunctions = ["CHAR", "SPACE", "DEL", "OK", "BACK", "EMPTY", "LF"]
  let keymap = []

  Object.getOwnPropertyNames( mapids ).forEach( pname=>{
    let pattern = patterns.patterns.find( p=> p.name === pname )
    let fullkeys = []
    
    if ( !defaultkmap ) defaultkmap = { name: pattern.name }
    if ( pattern.default ) defaultkmap = { name: pattern.name }

    pattern.keys.forEach( (k) => {                  
      k.forEach( (k2) => { fullkeys.push(k2) } )
    })

    fullkeys.forEach( (k)=> {
      if ( !keyfunctions.find( kf => kf === k.f ) ) keyfunctions.push( k.f )
      let fktidx = keyfunctions.findIndex( kf => kf === k.f )

      let kid
      if ( k.f.toUpperCase() === "CHAR" ) {        
        let ch = charlist.find( c=>c.charval === k.id )
        if ( ch ) {
          kid = k.id ? getHexByteVal( ch.charid ) : '0x02'
        } else {
          console.log( "Character: " + k.id + ' not found')
          kid = '0x02'
        }
      } else {
        kid = k.id ? getHexByteVal( k.id ) : '0x02'
      }
                    
      if ( k.size && k.size > 1 ) {
        for ( let i= 0; i< k.size; i++ ) {
          let spanpos = getHexByteVal( ( k.size - i ) - 1 )
          let spanneg = getHexByteVal( i )

          keymap.push('{ ' + fktidx + ', ' + kid + ', ' + spanpos + ', ' + spanneg + ' }' )
        }        
      } else {
        keymap.push('{ ' + fktidx + ', ' + kid + ', 0x00, 0x00 }' )
      }
    })
  })

  if ( defaultkmap ) {
    defaultkmap.idx = patterns.patternmap.findIndex( p=> p == defaultkmap.name )
    if ( defaultkmap.idx < 0 ) defaultkmap.idx = 12
  } else {
    defaultkmap = { idx: 12 }
  }


  outdatac.push('const Keylayout keylayouts[] = ')
  outdatac.push('{')
  outdatac.push( keymap.join(', ') )
  outdatac.push('};')
  outdatac.push('')

  outdatac.push('const uint8_t textdata[] = ')
  outdatac.push('{')
  {
    let bytelist = []
    let tpos = 0
    textdata.texte.forEach(td=>{        
      td.pos = tpos;
      for (let i=0; i<td.text.length;i++) {
        let c = td.text.charAt(i);
        let c2 = charlist.find( cl=>cl.charval == c );      
        if ( c2 ) {
          bytelist.push( getHexByteVal( c2.charid ) )                
        } else {
          bytelist.push( '0x02' )
        }  
        tpos++
      }
      bytelist.push( '0x00' )
      tpos++
    })

    outdatac.push(bytelist.join(","))  
  }  
  outdatac.push('};')

  outdatac.push('const uint16_t texte[] = ')
  outdatac.push('{')

  {
    let bytelist = []
    textdata.texte.forEach(td=>{      
      bytelist.push( td.pos )
    })

    outdatac.push(bytelist.join(","))  
  }

  outdatac.push('};')
  outdatac.push('')
  outdatac.push('const uint8_t hexchars[] = ')
  outdatac.push('{')

  {
    let bytelist = []
    for (let i=0;i<textdata.hexchars.length; i++) {
      let c = textdata.hexchars.charAt(i);
      let c2 = charlist.find( cl=>cl.charval == c );      
      if ( c2 ) {
        bytelist.push( getHexByteVal( c2.charid ) )                
      } else {
        bytelist.push( '0x02' )
      }        
    }

    outdatac.push(bytelist.join(","))  
  }
  outdatac.push('};')
  outdatac.push('')

  outdatah.push('#ifndef SYSIMAGES_H')
  outdatah.push('#define	SYSIMAGES_H')
  outdatah.push('#include	<xc.h>')
  outdatah.push('#include	<stdbool.h>')
  outdatah.push('')  
  outdatah.push('#define MAX_IMAGE_BUFFER ' + maximgbuffer)
  outdatah.push('#define TOTAL_IMAGE_COUNT ' + imgcount)
  outdatah.push('#define TOTAL_CHAR_COUNT ' + charlist.length )
  outdatah.push('#define CHAR_HEIGHT 15' )
  outdatah.push('#define CHAR_LINEFEED 0x01' )
  outdatah.push('#define CHAR_ENCODEERR 0x02' )
  outdatah.push('')  
  outdatah.push('// bmpsize: ' + pos + 'Bytes')
  outdatah.push('typedef struct {')
  outdatah.push('  const uint8_t width; // Bitmap width in pixels')
  outdatah.push('  const uint8_t height; // Bitmap width in pixels')
  outdatah.push('  const uint16_t bitmapOffset; // offset in bitmaparray')
  outdatah.push('} GFXimage;')
  outdatah.push('')
  outdatah.push('typedef struct {')
  outdatah.push('  const uint16_t id; // Image id')
  outdatah.push('  const uint8_t xoff;' )
  outdatah.push('  const uint8_t xadv;' )
  outdatah.push('  const uint16_t uccp; // Unicode codepoint' )  
  outdatah.push('} GFXChar;')
  outdatah.push('')  
  outdatah.push('typedef struct {')
  outdatah.push('  const uint16_t uccp;' )
  outdatah.push('  const uint8_t cid;' )    
  outdatah.push('} Unicodelist;')
  outdatah.push('')   
  outdatah.push('typedef struct {')
  outdatah.push('  const uint8_t active;' ) 
  outdatah.push('  const uint8_t name[5];' )
  outdatah.push('  const uint16_t layoutoff;' )
  outdatah.push('} Keyboardmaps;')
  outdatah.push('')
  outdatah.push('typedef struct {')
  outdatah.push('  const uint8_t fkt;' )
  outdatah.push('  const uint8_t id;' )
  outdatah.push('  const uint8_t span_pos;' )
  outdatah.push('  const uint8_t span_neg;' )
  outdatah.push('} Keylayout;')
  outdatah.push('')    
  outdatah.push('extern const uint16_t bitmapdata[];')
  outdatah.push('extern const GFXimage bitmaps[];')
  outdatah.push('extern const GFXChar gfxchars[];')
  outdatah.push('extern const Unicodelist unicodes[];')
  outdatah.push('extern const Keyboardmaps keymaps[];')
  outdatah.push('extern const Keylayout keylayouts[];')
  outdatah.push('extern const uint8_t textdata[];')
  outdatah.push('extern const uint16_t texte[];')
  outdatah.push('extern const uint8_t hexchars[];')
  outdatah.push('')  

  keyfunctions.forEach( ( kf, index )=>{
    outdatah.push('#define KEYCOMMAND_' + kf + ' ' + index )
  })
  outdatah.push('') 

  icons.forEach(ic=>{
    outdatah.push('#define SYSICON_' + ic.name + ' ' + ic.imgid )  
  })
  outdatah.push('')  

  textdata.texte.forEach(( td, ind )=>{    
    outdatah.push('#define TEXT_' + td.name + ' ' + ind )  
  })
  outdatah.push('')  

  outdatah.push('#define DEFAULT_KEYMAP ' + defaultkmap.idx ) 
  outdatah.push('')

  outdatah.push('#endif')


  fs.writeFileSync(outfilec, outdatac.join('\r\n'), { encoding: 'utf8' });
  //      outdatac.forEach(od => console.log(od));


  fs.writeFileSync(outfileh, outdatah.join('\r\n'), { encoding: 'utf8' });
  //      outdatac.forEach(od => console.log(od));


}).catch((error) => {
  console.log('Error read Imagefiles')
  console.log(error)
})

