const path = require('path');
const cp = require('codepage');
const fs = require("fs")
const Jimp = require('jimp');
const { resolve } = require('path');


const glyphheight = 13;
const codepage = 1252;

const icondir = './icons'
const fontfile = 'fontmaps/german1_16.fnt';
const fontmap = './fontmaps/german1_16_0.png';
const outfile = 'fontmaps/systemfont.c'

let content = fs.readFileSync(fontfile, 'utf8');
let clines = content.split('\r\n')

const linefeed = '\n';
const space = ' ';
const outcp = cp[codepage];
let linefeedindex = -1;
let spaceindex = -1;

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




let outchars = [];

const getHexByteVal = (b) => {
  let xv = b.toString(16)
  return '0x' + (xv.length < 2 ? '0' : '') + xv
}


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
    })


    let linebreakchar = 0
    let spacechar = 0
    let pos = 0
    let outdata = []
    outdata.push('// Systemfont: ' + charlist[0].height + ' pixels high')
    outdata.push('const uint8_t systemfont_Bitmaps[] = ')
    outdata.push('{')

    charlist.forEach((oc, index) => {
      oc.pos = pos
      outdata.push('// @' + pos + '"' + oc.charval + '" (' + oc.width + ' pixels wide)')
      let binline = ''
      for (let y = 0; y < oc.height; y++) {
        let line = ''
        for (let x = 0; x < oc.width; x++) {
          line += oc.glyph[x][y][0] > 150 ? '#' : ' '
          if (index > 0 || y > 0 || x > 0) binline += ','
          binline += getHexByteVal(Math.floor((oc.glyph[x][y][0] + oc.glyph[x][y][1] + oc.glyph[x][y][2]) / 3))
          pos++
        }
        outdata.push('// ' + line)
      }
      outdata.push(binline)
    })

    outdata.push('};')
    outdata.push('')
    outdata.push('')
    outdata.push('const GFXimage systemfont_Descriptors[] = ')
    outdata.push('{')

    charlist.forEach((oc, index) => {
      let betweenkomma = (index < (charlist.length - 1) ? ',' : '')
      outdata.push('  ' + '{ ' + oc.width + ', ' + oc.height + ', ' + oc.xoffset + ', ' + oc.xadvance + ', ' + oc.pos + ', ' + oc.id + ' }' + betweenkomma + ' // ' + oc.charval)
    })

    charlist.forEach((oc, index) => {
      if (oc.id == 32) {
        spacechar = index
      }
      if (oc.id == 182) {
        linebreakchar = index
      }
    })

    outdata.push('};')

    outdata.push('')
    outdata.push('')
    outdata.push('const uint8_t systemfont_Linebreak = ' + linebreakchar)
    outdata.push('const uint8_t systemfont_Space = ' + spacechar)

    outdata.push('// fontsize: ' + (pos + (charlist.length * (1 + 1 + 1 + 1 + 2 + 1))) + 'Bytes')


    let imagefiles = fs.readdirSync(icondir).filter((imgfile) => {
      imgfile.toLowerCase()
      return imgfile.endsWith('png') ||
        imgfile.endsWith('jpg') ||
        imgfile.endsWith('bmp')
    })

    let imgpromises = []
    let images = imagefiles.map(imgfile => {

      const img = {
        name: imgfile,
        imgdata: []
      }

      const pr = new Promise((resolve, reject) => {
        
        Jimp.read(path.resolve(icondir, imgfile))
          .then(image => {
            // Do stuff with the image.
            for (let i = 0; i < image.bitmap.width; i++) {
              let newarr = []
              for (let j = 0; j < image.bitmap.height; j++) {
                newarr.push(0)
              }
              img.imgdata.push(newarr)
            }

            image.scan(
              0, 0, image.bitmap.width, image.bitmap.height,
              (x, y, idx) => {

                let red = image.bitmap.data[idx + 0];
                let green = image.bitmap.data[idx + 1];
                let blue = image.bitmap.data[idx + 2];

                img.imgdata[x][y] = [red, green, blue]
              }
            )

            resolve()
          }).catch((err) => {
            console.log('Error read image ' + imgfile )
            console.log(err)
            reject()
          })

      })
      imgpromises.push(pr)
    })

    Promise.all(imgpromises).then((values) => {
      images.forEach(img=>{


      })

      fs.writeFileSync(outfile, outdata.join('\r\n'), { encoding: 'utf8' });
      outdata.forEach(od => console.log(od));
    }).catch((error) => {
    })



  }).catch((err) => {
    console.log('Error read fontimage')
    console.log(err)
  })
