const H = require ( 'highland' );
const R = require ( 'ramda' );
const request = require ( 'highland-request' );
const fs = require ( 'fs' );

const folder = process.argv[2] || 'in';
const locationName = process.argv[3] || 'Hackney Wick';
const apiUrl = 'https://itemstore-prod.inyourarea.co.uk';

const log = thing => console.log ( JSON.stringify ( thing ) );

return request ( {
    url: `${apiUrl}/items/locations`,
    qs: {
        name: locationName
    },
    json: true
} )
    .sequence ()
    .head ()
    .flatMap ( location => request ( {
        url: `${apiUrl}/items/articles`,
        qs: {
            annotation_uri: location.annotation_uri.join ( '~' )
        },
        json: true
    } )
        .sequence ()
        .take ( 50 )
        .flatMap ( item => H.wrapCallback ( fs.writeFile )( `${folder}/${item.id}`, `${item.item.title}\n${item.item.body}`.toLowerCase () ) )
    )
    .errors ( error => console.error ( error ) )
    .each ( log );
