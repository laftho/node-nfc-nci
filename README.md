### node-nfc-nci

#### depedencies

cmake

`sudo apt install -y cmake automake autoconf libtool pkg-config`

linux_libnfc-nci - https://github.com/NXPNFCLinux/linux_libnfc-nci

`git clone https://github.com/NXPNFCLinux/linux_libnfc-nci.git`

`./bootstrap`


`./configure --enable-i2c`

`make`

`sudo make install`

It installs the libnfc-nci-linux library to /usr/local/lib target directory. This path must be
added to LD_LIBRARY_PATH environment variable for proper reference to the library
during linking/execution of application.

#### setup

`npm install node-nfc-nci`

https://www.npmjs.com/package/node-nfc-nci

#### documentation

include
`const nci = require("node-nfc-nci");`

module exports nci interface object with single method `listen`

`listen(callback<function>)` - will attempt to initialize the device via the `linux_libnfc-nci` library on a new thread and immediately call the callback with a `context` object.

`context` - context is an event emitter and interface to setting tag write for the next tag to arrive.

###### events

`error` - `message<string>` - emits on any error, even for errors when attempting to initialize the device.
`arrived` - `tag<object>` - emits on NFC tag arrival
`departed` -`tag<object>` - emits on NFC tag departure. Provide a copy of the original arrived tag. If NDEF data has been updated during the tag's presence it will not be reflected in departure.
`written` - `tag<object>, previous<object>` - emits on successful tag NDEF write. Provides updated tag and a copy of the original arrived tag prior to update.

###### tag object

example
```
{
  "technology": {
    "code": 9,
    "name": "Type A - Mifare Ul",
    "type": "MIFARE_UL"
  },
  "uid": {
    "id": "04:e1:5f:d2:9c:39:80",
    "type": "NFCID1",
    "length": 7
  },
  "ndef": {
    "size": 868,
    "length": 18,
    "read": 11,
    "writeable": true,
    "type": "Text",
    "content": "hello world"
  }
}
```

##### tag write

via context

`context.setNextWrite(type<string>, content<string>)` - set data to write for next tag arrival, this will attempt to indiscriminately write the next tag that arrives.
`context.clearNextWrite()` - clears the pending next write.
`context.hasNextWrite()<bool>` - flag to check if there is a next write pending.
`context.immediateWrite(type<string>, content<string>)` - attempts to immediately write to the device that is present. However, tag `arrived` event provides a `tag.write` function which is an alias of `immediateWrite` but likely more practical because `immediateWrite` depends on a device being present.

via tag

`tag.write(type<string>, content<string>)` - attempt immediate write to which ever tag is present. This write does not guarantee it will write only to the particular tag `tag` describes, as it's only a convenience alias to `context.immediateWrite` 

Acceptable types
- `Text` - writes `en` lang text to the NDEF content

#### example

```
const nci = require("node-nfc-nci");

nci.listen(context => {
    context.on("error", msg => console.log(msg));

    context.on("arrived", tag => {
        console.log(`ARRIVED: ${JSON.stringify(tag)}`);

        if (!context.hasNextWrite()) {
            if (tag.uid.id === "04:e1:5f:d2:9c:39:80") {
                tag.write("Text", "hello world");
            }
        }
    });

    context.on("written", (tag, previous) => {
        console.log(`PREVIOUS: ${JSON.stringify(previous)}`);
        console.log(`UPDATED: ${JSON.stringify(tag)}`);
    });

    context.on("departed", tag => {
        console.log(`DEPARTED: ${JSON.stringify(tag)}`);

        if (tag.ndef.content !== "foobar") {
            context.setNextWrite("Text", "foobar"); // will attempt write on any next tag
        }
    });
});
```
