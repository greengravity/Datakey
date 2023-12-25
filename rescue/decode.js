//Decoder/Encoder for the encrypted Datafiles
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');


let masterkey
if ( process.argv.length === 3 ) {
    masterkey = process.argv[2]
}

if ( !masterkey ) console.log("Masterkey must be set")


let hashgen = crypto.createHash('sha256')
for ( let i=0;i < masterkey.length;i++) {
	const cp = masterkey.codePointAt(i)
	const tp = new Uint8Array(2)
	tp[0] = ( cp & 0xff00 ) >> 8
	tp[1] = ( cp & 0xff )
	
	hashgen.update( tp )
}
hashgen.update( new Uint8Array(2) )

const cryptkey = hashgen.digest().subarray(0,16)

const checkMasterKey = ( key, verficationfile ) => {
	const vdata = fs.readFileSync( verficationfile )

	const algorithm = 'aes-128-cbc'

	// The IV is usually passed along with the ciphertext.
	const iv = Buffer.alloc(16, 0); // Initialization vector.

	const decipher = crypto.createDecipheriv(algorithm, key, iv);
	decipher.setAutoPadding(false)
	let decrypted = decipher.update(vdata);
	decrypted = Buffer.concat([decrypted, decipher.final()]);

	for ( let i=0;i<16;i++ ) {
	  let v1 = decrypted.readUInt8(i)
	  let v2 = decrypted.readUInt8(i+16)
	  let v3 = decrypted.readUInt8(i+32)

	  if ( ( v1 ^ v2 ) !== v3 ) {
		console.log( "master mismatch" )
		process.exit()
	  }
	}
}


const getTokenName = ( type ) => {	
	
	switch ( type ) {	
		case 1: //NAME
			return "NAME"
		case 2: //USER
			return "USER"
		case 3: //KEY1
			return "KEY1"		
		case 4: //KEY2
			return "KEY2"
		case 5: //URL
			return "URL"		
		case 6: //INFO
			return "INFO"		
		case 7: //CHECK
			return "CHECK"
	}

	return null
}
 

const decodeData = ( encfolder, decfolder, key ) => {

	//clean up decrypted folder
	let files  = fs.readdirSync( decfolder )
	files.forEach(f=> {
		fs.rmSync( path.resolve( decfolder, f ), { recursive: true } )	
	})
	

	let entries = []
	
	files  = fs.readdirSync( encfolder )
	files.forEach(f=> {
		//decrypt file by file
		const vdata = fs.readFileSync(path.resolve( encfolder, f ))

		const algorithm = 'aes-128-cbc'

		const iv =  vdata.subarray(0,16) //Buffer.alloc(16, 0); // Initialization vector is empty.

		const decipher = crypto.createDecipheriv(algorithm, key, iv);
		decipher.setAutoPadding(false)
		let decrypted = decipher.update(vdata.subarray(16));
		decrypted = Buffer.concat([decrypted, decipher.final()]);
		
		let d = new Uint8Array(decrypted)

		

		const TOKEN_BLOCK_SIZE = 4096
		for ( let i=0; i<16; i++) {
			//read avail tokens
			const type = getTokenName( d[i+16] )
			if ( type ) {

				let entryvalue = ""
				let readoffs = i * TOKEN_BLOCK_SIZE + 32
				for ( let j=readoffs;j<(TOKEN_BLOCK_SIZE+readoffs);) {
					let v1 = d[j]
					j++
					let v2 = d[j]
					j++
					let cp = ( v1 << 8 ) | v2
					if ( cp === 0 ) break
					
					let ch = String.fromCodePoint(cp)
					entryvalue += ch
				}
						
				if ( entryvalue.length > 0 ) {
					entries.push( { 
						type: type,
						value: entryvalue 
					})
				}
			}
		}
	})

	fs.writeFileSync( path.resolve( decfolder, 'entries.json' ), JSON.stringify( entries, null, 2) )

}

const encodeData = (decfolder, encfolder) => {



}


checkMasterKey( cryptkey, path.resolve('./encrypted/VERIFY') )
decodeData( path.resolve( './encrypted/KS' ), path.resolve( './decrypted' ), cryptkey )

console.log("done")

