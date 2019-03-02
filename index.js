const EventEmitter = require('events').EventEmitter;

const debug = process.env.NODE_DEBUG && process.env.NODE_DEBUG.includes('node_nfc_nci');

const native_nci = require(`./build/${debug ? 'Debug' : 'Release'}/node_nfc_nci`);

class NCIListener {
    constructor() {
        this.emitter = new EventEmitter();
        this.context = null;
    }

    on(event, handler) {
        this.emitter.on(event, handler);
    }

    write(type, content) {
        this.context.write({ type, content });
    }

    listen(cb) {
        this.context = native_nci.listen(this.emitter.emit.bind(this.emitter));

        if (cb) {
            cb(this);
        }
    }
}

module.exports = new NCIListener();
