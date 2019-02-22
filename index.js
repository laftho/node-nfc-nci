const addon = require('./build/Release/node_nfc_nci');

console.log(addon.hello());

addon.poll(arg => {
    console.log(arg);
});

module.exports = addon;
