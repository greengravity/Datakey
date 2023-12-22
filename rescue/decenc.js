//Decoder/Encoder for the encrypted Datafiles


let masterkey
if ( process.argv.length === 3 ) {
    masterkey = process.argv[2]
}

if ( !masterkey ) console.log("Masterkey must be set")







