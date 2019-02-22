const addon = require('./build/Release/node_nfc_nci');

console.log(addon.hello());

module.exports = addon;
