let rgb = [0x80, 0x7f, 0x7f]


let v = ( ( rgb[0] >> 3 ) << 11 ) | ( ( rgb[1] >>  2 ) << 5 ) | ( rgb[2] >> 3 );  

console.log( v.toString(16).toUpperCase() );
