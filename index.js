const EventEmitter = require('events').EventEmitter;

const debug = process.env.NODE_DEBUG && process.env.NODE_DEBUG.includes('node_nfc_nci');

const native_nci = require(`./build/${debug ? 'Debug' : 'Release'}/node_nfc_nci`);

class NCIListener extends EventEmitter {
    constructor() {
        super();

        this.emitter = new EventEmitter();
        this.context = null;

        this.emitter.on("arrived", tag => {
           tag.write = this.context.immediateWrite;

           this.emit("arrived", tag);
        });

        this.emitter.on("error", error => {
           this.emit("error", error);
        });

        this.emitter.on("departed", tag => {
           this.emit("departed", tag);
        });

        this.emitter.on("written", (tag, previous) => {
           this.emit("written", tag, previous);
        });
    }

    setWrite(type, content) {
        this.context.setWrite({ type, content });
    }

    clearWrite() {
        this.context.clearWrite();
    }

    hasWrite() {
        const ndef = this.context.getWrite();

        if (ndef) {
            return true;
        }

        return false;
    }

    getWrite() {
        return this.context.getWrite();
    }

    listen(cb) {
        this.context = native_nci.listen(this.emitter.emit.bind(this.emitter));

        if (cb) {
            cb(this);
        }
    }
}

module.exports = new NCIListener();
