const addon = require('./build/Release/node_nfc_nci');

console.log(addon.hello());

addon.poll(arg => {
    console.log(JSON.stringify(arg));
});

module.exports = addon;
