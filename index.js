const EventEmitter = require('events').EventEmitter;
const addon = require('./build/Debug/node_nfc_nci');

const emitter = new EventEmitter();

emitter.on("error", message => console.log(message));
emitter.on("arrived", tag => console.log(JSON.stringify(tag)));
emitter.on("departed", () => console.log("departed"));


addon.listen(emitter.emit.bind(emitter));

module.exports = addon;
