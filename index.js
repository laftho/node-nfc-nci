const addon = require('./build/Debug/node_nfc_nci');

// console.log(addon.hello());

addon.poll(arg => {
    console.log(JSON.stringify(arg));
});

module.exports = addon;
