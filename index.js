const addon = require('./build/Debug/node_nfc_nci');

// console.log(addon.hello());

addon.listen(context => {
  context.on("error", message => console.log(message));
  context.on("arrived", tag => console.log(JSON.stringify(tag)));
  context.on("departed", () => console.log("departed"));
});

module.exports = addon;
