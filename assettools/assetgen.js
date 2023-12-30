const path = require('path');
const fs = require("fs")
const Jimp = require('jimp');
const { resolve } = require('path');
const { getSystemErrorMap } = require('util');


if (process.argv.length < 3) {
  console.log("Add fontname as argument")
  return
}

const fontname = process.argv[2]


const locals = JSON.parse(fs.readFileSync('./locals/locals.json', 'utf8'));
const local = locals.find(l => l.name === fontname)
if (!local) {
  console.log("fontname not found")
  return
}

const icondir = './icons'
const fontfile = './locals/fonts/' + local.fontfolder + '/font.fnt';
const fontmap = './locals/fonts/' + local.fontfolder + '/font_0.png';
const patternfile = './locals/fonts/' + local.fontfolder + '/patterns.json';
const keycodefile = './locals/fonts/' + local.fontfolder + '/keycodes.json';
const textfile = './locals/text/' + local.textfile;
const defaulttext = './locals/text/default.json';


const outfileh = '../core/coreprocessor.X/assets/assets_' + fontname + '.h'
const outfilec = '../core/coreprocessor.X/assets/assets_' + fontname + '.c'

let content = fs.readFileSync(fontfile, 'utf8');
let clines = content.split('\r\n')

const linefeed = '\n';
const space = ' ';
let linefeedindex = -1;
let spaceindex = -1;

let heavyprocesserror = false

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
    character.xadvance = character.width

    character.charval = String.fromCodePoint(character.id)
    charlist.push(character);
  }
})


// Reading icondescription
const iconjson = JSON.parse(fs.readFileSync(path.resolve(icondir, 'icons.js'), { encoding: "utf-8" }))

const keycodes = JSON.parse(fs.readFileSync(keycodefile, { encoding: "utf-8" }))

// Reading patterndescription
const patterns = JSON.parse(fs.readFileSync(patternfile, { encoding: "utf-8" }))

patterns.patterns.forEach(pm => {
  while (pm.keys.length < 4) pm.keys.push([])

  pm.keys.forEach(pk => {
    let pcount = 0

    pk.forEach(pk2 => {
      pcount += pk2.size ? pk2.size : 1
    })

    if (pcount < 10) {
      pk.push({ f: "EMPTY", size: (10 - pcount) })
    }
  })

})

// Reading Textinfos
const textdatadefault = JSON.parse(fs.readFileSync(defaulttext, { encoding: "utf-8" }))
const textdatalang = JSON.parse(fs.readFileSync(textfile, { encoding: "utf-8" }))

const textdata = Object.assign(textdatadefault, textdatalang)


// Reading imagefiles and convert to a raw rbg pixelmap
let readpromises = []
{
  const pr = new Promise((resolve, reject) => {
    //extract glyphs from the fontmap utilizing the fontdescription
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

  const icon = Object.assign({ imgdata: [], path: path.resolve(icondir, jicon.icon), color: jicon.color === false ? false : true }, jicon)

  const pr = new Promise((resolve, reject) => {
    //read all the icons from the icon directory
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

            let red = image.bitmap.data[idx + 0]
            let green = image.bitmap.data[idx + 1]
            let blue = image.bitmap.data[idx + 2]

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

//Add an empty icon at position 0 to assign to dummy chars like linefeed and zero
icons.unshift({ name: "EMPTY", width: 0, height: 0 })


//waiting for all imagefiles to be read and mapped
Promise.all(readpromises).then((values) => {


  const escapeOutCharval = (c) => {
    if (c === "\\") return "Backslash"
    return c
  }

  const getHexByteVal = (b) => {
    let xv = b.toString(16)
    return '0x' + (xv.length < 2 ? '0' : '') + xv
  }

  const rgbToByteVal = (rgb) => {
    let sval = ((rgb[0] & 0xf8) << 8) |
      ((rgb[1] & 0xfc) << 3) |
      ((rgb[2] & 0xf8) >> 3)

    let sv1 = (sval & 0xff00) >> 8
    let sv2 = (sval & 0xff)

    return [getHexByteVal(sv1), getHexByteVal(sv2)]
  }

  const compress_binary_image = (bin) => {
    //after the image gets processed into a string of bytes, this bytearray will be compressed to reduce the size of the assetfile
    //the compression is quite simple, 1 byte will mark how much time the same byte should be repeated or how much different bytes come in a row
    //the first bit of the counter will mark if its differen or same, and the 7 bits after are the counter
    //like the string 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x24, 0x25, 0x00, 0x00, 0x00 will be compressed to
    //                0x05, 0x00, 0x83, 0x23, 0x24, 0x25, 0x03, 0x00

    let binc = []
    let offs = 0

    while (offs < bin.length) {
      let foundsame = false
      let i = 0;
      //lookahead to find at least 3 bytes in a row
      for (; ; i++) {
        if (offs + i < bin.length - 3) {
          //find 3 same in a row
          foundsame = true
          for (let j = 1; j < 3; j++) {
            if (bin[offs + i] !== bin[offs + i + j]) {
              foundsame = false
              break;
            }
          }
          if (foundsame) break;
        }
        if (i == 127 || (offs + i >= bin.length)) break;
      }

      if (i > 0) {
        //write block of different bytes				
        binc.push(getHexByteVal(0x80 | i))  //length of the block
        for (let j = 0; j < i; j++)	binc.push(bin[offs + j])
        offs += i
      }


      if (foundsame) {
        //write block of same bytes

        let foundval = bin[offs]
        let i = 1;
        //count the full amount of same bytes in a row
        for (; ; i++) {
          if (offs + i < bin.length) {
            if (bin[offs + i] !== foundval) break
          } else break
          if (i == 127) break;
        }

        //write the amount of same bytes followed by the bytevalue				
        binc.push(getHexByteVal(i))  //length of the block
        binc.push(foundval)

        offs += i
      }
    }

    let b1 = ((binc.length & 0xff00) >> 8)
    let b2 = (binc.length & 0x00ff)

    binc.unshift(getHexByteVal(b2))
    binc.unshift(getHexByteVal(b1))

    return binc
  }


  let imgcount = 0
  let pos = 0
  let outdatac = []
  let outdatah = []
  let maximgbuffer = 0
  let imglist = []


  // Generating .C Assetfile

  outdatac.push('#include	"../assets.h"')
  outdatac.push('')
  outdatac.push('#if ASSETPACK == ASSETPACK_' + fontname.replace('-', '_'))
  outdatac.push('')

  
  //All imagedata goes in a single array
  //as the limitations (from the chip/compiler) are 32KB in size , some reductions and compressions has to be done here
  outdatac.push('// Systemfont: ' + charlist[0].height + ' pixels high')
  outdatac.push('const __prog__ uint8_t __attribute__((space(prog))) bitmapdata[] = ')
  outdatac.push('{')

  // first process the glyphs from the fontmap
  // the rgb map will be convertert to a grayscaled and compressed array
  charlist.forEach((oc, index) => {    
    oc.imgid = imgcount
    imgcount++

    imglist.push({
      color: false,
      width: oc.width,
      height: oc.height,
      imgoffs: pos,
    })

    outdatac.push('// @' + pos + '"' + oc.charval + '" (' + oc.width + ' pixels wide)')
    let bin_uncompressed = []

    let val = 0
    let even = true
    for (let y = 0; y < oc.height; y++) {
      let line = ''
      for (let x = 0; x < oc.width; x++) {
        line += oc.glyph[x][y][0] > 150 ? '#' : ' '

        bin_uncompressed.push(getHexByteVal(oc.glyph[x][y][0]))
      }
      outdatac.push('// ' + line)
    } 1
    let bin_compressed = compress_binary_image(bin_uncompressed);
    pos += bin_compressed.length;

    let binline = (index > 0 && bin_compressed.length > 0 ? ',' : '') + bin_compressed.join(',')
    outdatac.push(binline)
  })


  // second process the images from the icondir
  // the rgb map may be convertert to a grayscaled or a rbg565 map (for colored imaged) and compressed

  encodeerr_imgid = 0;
  space_imgid = 0;
  icons.forEach((icon, index) => {
    icon.imgid = imgcount
    if (icon.name === "ENCODEERR") { encodeerr_imgid = imgcount }
    if (icon.name === "SPACETEXT") { space_imgid = imgcount }

    imgcount++

    imglist.push({
      color: icon.color,
      width: icon.width,
      height: icon.height,
      imgoffs: pos,
    })

    outdatac.push('// @' + pos + '"' + icon.name + '" (' + icon.width + ' pixels wide / ' + icon.height + ' pixels high)')
    let bin_uncompressed = []

    for (let y = 0; y < icon.height; y++) {
      let line = ''
      for (let x = 0; x < icon.width; x++) {
        line += icon.imgdata[x][y][0] > 150 ? '#' : ' '

        if (icon.color) {
          let rgbval = rgbToByteVal(icon.imgdata[x][y])
          bin_uncompressed.push(rgbval[0])
          bin_uncompressed.push(rgbval[1])
        } else {
          bin_uncompressed.push(getHexByteVal(icon.imgdata[x][y][0]))
        }
      }
      outdatac.push('// ' + line)
    }

    let bin_compressed = compress_binary_image(bin_uncompressed);
    pos += bin_compressed.length;

    let binline = (bin_compressed.length > 0 ? ',' : '') + bin_compressed.join(',')
    outdatac.push(binline)

  })

  imglist.forEach(img => {
    let imgbuffer = img.width * img.height
    if (imgbuffer > maximgbuffer) maximgbuffer = imgbuffer
  })

  
  //writing the headers to the imagedata array including width, height and offset in the bitmap- dataarray
  outdatac.push('};')
  outdatac.push('')
  outdatac.push('')
  outdatac.push('const GFXimage bitmaps[] = ')
  outdatac.push('{')
  imglist.forEach((img, index, arr) => {
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')

    let ioffs = img.imgoffs
    if (!img.color) ioffs = ioffs | 0x8000;

    outdatac.push('  ' + '{ ' + img.width + ', ' + img.height + ', ' + ioffs + ' }' + betweenkomma)
  })
  outdatac.push('};')
  outdatac.push('')

  //write the list of available characters with there corresponding image and some headerinformation like distance from an to next glyph, unicode expression
  //because we use no wchar due memory limitations, we have only 255 possible characters and we need a unicode mapping for save and load data
  outdatac.push('const GFXChar gfxchars[] = ')
  outdatac.push('{')

  charlist = charlist.filter(c => {
    return !(c.id == 32 || c.id == 65533 || c.id == 10 || c.id == 0)
  })

  charlist.unshift({ imgid: space_imgid, xoffset: 0, xadvance: 5, id: 32, charval: " " }) //space character'
  charlist.unshift({ imgid: encodeerr_imgid, xoffset: 0, xadvance: 15, id: 65533, charval: "encodeerr" }) //Encode error character'
  charlist.unshift({ imgid: 0, xoffset: 0, xadvance: 0, id: 10, charval: "linefeed" }) //linefeed character'
  charlist.unshift({ imgid: 0, xoffset: 0, xadvance: 0, id: 0, charval: "zero" }) //dummy zero character for zero terminated strings'

  charlist.forEach((ch, index, arr) => {
    let c_keycodes = ["0x00", "0x00", "0x00", "0x00"]

    let kc = keycodes.keycodes.find(kc => (kc.char == ch.charval || kc.char == "\n" && ch.charval == "linefeed") && kc.code)
    if (kc && kc.code.length <= 4) {
      kc.code.forEach((kcc, kcindex) => {
        c_keycodes[kcindex] = kcc
      })
    } else {
      if ( !( ch.charval == 'zero' ||
              ch.charval == ' ' ||
              ch.charval == 'linefeed' ||
              ch.charval == 'encodeerr' ) ) {
        console.log( "Error - no keycode found for character " + ch.charval + ' #' + ch.id )
        process.exit()      
      }
    }

    ch.charid = index
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')
    outdatac.push('  ' + '{ ' + ch.imgid + ',' + ch.xoffset + ', ' + ch.xadvance + ', ' + ch.id + ', {' + c_keycodes.join(', ') + '} }' + betweenkomma + ' // ' + escapeOutCharval(ch.charval))
  })
  outdatac.push('};')
  outdatac.push('')


  calcCaseTranslateMap = ( ) => {
    let cm = []

    for (let i=0;i<charlist.length;i++) {
      let ch = charlist[i]
      let cmv = '0x00'

      if ( !( ch.charval == 'zero' ||
              ch.charval == ' ' ||
              ch.charval == 'linefeed' ||
              ch.charval == 'encodeerr' ) ) {
        
        let cv = ch.charval.toLowerCase()
        if ( cv != ch.charval ) {
          ch2 = charlist.find( c => c.charval === cv )
          if ( ch2 ) {
            cmv = getHexByteVal( ch2.charid )
          } 
        }

        cv = ch.charval.toUpperCase()
        if ( cv != ch.charval ) {
          ch2 = charlist.find( c => c.charval === cv )
          if ( ch2 ) {
            cmv = getHexByteVal( ch2.charid )
          } 
        }
      }
      cm.push( cmv )
    }	

    return cm  
  }
  let cm = calcCaseTranslateMap()

  outdatac.push('const uint8_t casetranslatelist[] = ')
  outdatac.push('{')
    outdatac.push( cm.join(', ') )
  outdatac.push('};')
  outdatac.push('')



  //write a sorted lookup table to find characters from there unicode expression
  charlist.sort((a, b) => {
    return a.id - b.id
  })
  outdatac.push('const Unicodelist unicodes[] = ')
  outdatac.push('{')
  charlist.forEach((ch, index, arr) => {
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')
    outdatac.push('  ' + '{ ' + ch.id + ',' + ch.charid + ' }' + betweenkomma + ' // ' + escapeOutCharval(ch.charval))
  })
  outdatac.push('};')
  outdatac.push('')



  //preprocess patterns.json and extract all textdata from it
  let mapids = {}
  let mapidx = 0

  let emptytext = { text: "" }
  textdata.texte.push(emptytext)

  patterns.patternmap.forEach((p, index) => {
    if (p && p != "") {
      if (!mapids[p]) {
        let pattern = patterns.patterns.find(kp => kp.name === p)
        if (pattern) {
          let t = { text: pattern.text }
          textdata.texte.push(t)
          let defx = pattern.defx !== undefined ? getHexByteVal(pattern.defx) : "0xff"
          let defy = pattern.defy !== undefined ? getHexByteVal(pattern.defy) : "0xff"

          mapids[p] = { idx: mapidx, text: t, defx: defx, defy: defy }
          mapidx++
        }
      }
    }
  })


  patterns.generatormaps.forEach((gm, index) => {
    gm.nametext = { text: gm.name }
    textdata.texte.push(gm.nametext)

    gm.charsettext = { text: gm.charset }
    textdata.texte.push(gm.charsettext)
  })

  //generate textdata, using information the language files as well as the patternfile
  //the outputed textdata are mapped to the intern characters
  outdatac.push('const uint8_t textdata[] = ')
  outdatac.push('{')
  {
    let bytelist = []
    let tpos = 0
    textdata.texte.forEach(td => {
      td.pos = tpos;
      for (let i = 0; i < td.text.length; i++) {
        let c = td.text.charAt(i);
        let c2 = charlist.find(cl => cl.charval == c || cl.charval == "linefeed" && c == "\n");
        if (c2) {
          bytelist.push(getHexByteVal(c2.charid))
        } else {
          bytelist.push('0x02')
        }
        tpos++
      }
      bytelist.push('0x00')
      tpos++
    })

    outdatac.push(bytelist.join(","))
  }
  outdatac.push('};')


  //Write the offsets of the textdata into a array for easier access
  outdatac.push('const uint16_t texte[] = ')
  outdatac.push('{')

  {
    let bytelist = []
    textdata.texte.forEach(td => {
      bytelist.push(td.pos)
    })

    outdatac.push(bytelist.join(","))
  }

  outdatac.push('};')
  outdatac.push('')


  //after stored the offsets from the text, the generatormaps can be outputed
  //a generatormap consists of a name and a corresponding characterset that will be used
  //by the true random generator
  outdatac.push('const Generatormaps generatormaps[] = ')
  outdatac.push('{')

  outdatac.push(
    patterns.generatormaps.map((gm, index) => {
      return '{ ' + gm.nametext.pos + ', ' + gm.charsettext.pos + ', ' + gm.len + '}'
    }).join(',')
  )
  outdatac.push('};')
  outdatac.push('')



  //Keyboardmaps and Keylayouts are the base of the virtual keyboard
  //It got so complex, because the definitionfile should be comfortable to write
  //Thats why it need logic to process loosely defined character areas
  outdatac.push('const Keyboardmaps keymaps[] = ')
  outdatac.push('{')


  patterns.patternmap.forEach((p, index, arr) => {
    let betweenkomma = (index < (arr.length - 1) ? ',' : '')

    if (p && p != "" && mapids[p]) {
      outdatac.push('  ' + '{ true,  ' + mapids[p].text.pos + ', ' + mapids[p].defx + ', ' + mapids[p].defy + ', ' + (mapids[p].idx * 40) + ' }' + betweenkomma + ' // ' + mapids[p].text)
    } else {
      outdatac.push('  ' + '{ false, ' + emptytext.pos + ', 0xff, 0xff, 0 }' + betweenkomma + ' // empty')
    }
  })
  outdatac.push('};')
  outdatac.push('')

  let defaultkmap
  let keyfunctions = ["EMPTY", "CHAR", "SPACE", "BACKSPACE", "LF", "OK", "ABORT", "GEN"]
  let keymap = []

  Object.getOwnPropertyNames(mapids).forEach(pname => {
    let pattern = patterns.patterns.find(p => p.name === pname)
    let fullkeys = []

    if (!defaultkmap) defaultkmap = { name: pattern.name }
    if (pattern.default) defaultkmap = { name: pattern.name }

    pattern.keys.forEach((k) => {
      k.forEach((k2) => { fullkeys.push(k2) })
    })

    fullkeys.forEach((k) => {
      if (!keyfunctions.find(kf => kf === k.f)) keyfunctions.push(k.f)
      let fktidx = keyfunctions.findIndex(kf => kf === k.f)

      let kid
      if (k.f.toUpperCase() === "CHAR") {
        let ch = charlist.find(c => c.charval === k.id)
        if (ch) {
          ch.inpattern = true
          kid = k.id ? getHexByteVal(ch.charid) : '0x02'          
        } else {
          console.log("Error - Character: " + k.id + ' not found')
          heavyprocesserror = true   
          kid = '0x02'      
        }
      } else if (k.f.toUpperCase() === "GEN") {
        let ind = patterns.generatormaps.findIndex((gm, index) => {
          return gm.id == k.id
        })
        kid = ind >= 0 ? getHexByteVal(ind) : '0x00'
      } else {
        kid = k.id ? getHexByteVal(k.id) : '0x00'
      }

      if (k.size && k.size > 1) {
        for (let i = 0; i < k.size; i++) {
          let spanpos = getHexByteVal((k.size - i) - 1)
          let spanneg = getHexByteVal(i)

          keymap.push('{ ' + fktidx + ', ' + kid + ', ' + spanpos + ', ' + spanneg + ' }')
        }
      } else {
        keymap.push('{ ' + fktidx + ', ' + kid + ', 0x00, 0x00 }')
      }
    })
  })

  if (defaultkmap) {
    defaultkmap.idx = patterns.patternmap.findIndex(p => p == defaultkmap.name)
    if (defaultkmap.idx < 0) defaultkmap.idx = 12
  } else {
    defaultkmap = { idx: 12 }
  }


  outdatac.push('const Keylayout keylayouts[] = ')
  outdatac.push('{')
  outdatac.push(keymap.join(', '))
  outdatac.push('};')
  outdatac.push('')

  charlist.forEach(c=> {
    if ( c.charval == 'zero' ||
         c.charval == ' ' ||
         c.charval == 'linefeed' ||
         c.charval == 'encodeerr' ) return

    if ( !c.inpattern ) {
      console.log("Warning - character " + c.charval + " #" + c.id + "  not found in Patterns")
    }
  })



  //A list of characters that can be used to display numeric information, like datetime, values ...
  outdatac.push('const uint8_t hexchars[] = ')
  outdatac.push('{')

  {
    let bytelist = []
    for (let i = 0; i < textdata.hexchars.length; i++) {
      let c = textdata.hexchars.charAt(i);
      let c2 = charlist.find(cl => cl.charval == c);
      if (c2) {
        bytelist.push(getHexByteVal(c2.charid))
      } else {
        console.log("Hexchar " + c + " not found")
        bytelist.push('0x02')
      }
    }

    outdatac.push(bytelist.join(","))
  }
  outdatac.push('};')
  outdatac.push('')
  outdatac.push('#endif')



  // Generating .H Assetfile
  outdatah.push('#if ASSETPACK == ASSETPACK_' + fontname.replace('-', '_') )

  outdatah.push('')
  outdatah.push('#define MAX_IMAGE_BUFFER ' + maximgbuffer)
  outdatah.push('#define TOTAL_IMAGE_COUNT ' + imgcount)
  outdatah.push('#define TOTAL_CHAR_COUNT ' + charlist.length)
  outdatah.push('#define CHAR_HEIGHT 15')
  outdatah.push('#define CHAR_EOL 0x00')
  outdatah.push('#define CHAR_LINEFEED 0x01')
  outdatah.push('#define CHAR_ENCODEERR 0x02')
  outdatah.push('#define CHAR_SPACE 0x03')
  outdatah.push('')
  outdatah.push('// bmpsize: ' + pos + 'Bytes')

  outdatah.push('')

  keyfunctions.forEach((kf, index) => {
    outdatah.push('#define KEYCOMMAND_' + kf + ' ' + index)
  })
  outdatah.push('')

  icons.forEach(ic => {
    outdatah.push('#define SYSICON_' + ic.name + ' ' + ic.imgid)
  })
  outdatah.push('')

  textdata.texte.forEach((td, ind) => {
    if (td.name) {
      outdatah.push('#define TEXT_' + td.name + ' ' + ind)
    }
  })
  outdatah.push('')

  outdatah.push('#define DEFAULT_KEYMAP ' + defaultkmap.idx)
  outdatah.push('')
  outdatah.push('#define PASSWORD_REPLACE_CHAR ' + getHexByteVal(charlist.find(cl => cl.charval == textdata.passwordreplacechar).charid))
  outdatah.push('')

  outdatah.push('#endif')

  if ( heavyprocesserror ) {
    process.exit(0)
  }


  fs.writeFileSync(outfilec, outdatac.join('\r\n'), { encoding: 'utf8' });
  //      outdatac.forEach(od => console.log(od));


  fs.writeFileSync(outfileh, outdatah.join('\r\n'), { encoding: 'utf8' });
  //      outdatac.forEach(od => console.log(od));


}).catch((error) => {
  console.log('Error read Imagefiles')
  console.log(error)
})

