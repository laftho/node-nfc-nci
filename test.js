const nci = require("./index");

nci.listen(context => {
    context.on("error", msg => console.log(msg));

    context.on("arrived", tag => {
        console.log(`ARRIVED: ${JSON.stringify(tag)}`);

        if (tag.uid.id === "04:e1:5f:d2:9c:39:80") {
            context.write("Text", "hello world");
        }
    });

    context.on("written", (tag, previous) => {
        console.log(`PREVIOUS: ${JSON.stringify(previous)}`);
        console.log(`UPDATED: ${JSON.stringify(tag)}`);
    });

    context.on("departed", tag => {
        console.log(`DEPARTED: ${JSON.stringify(tag)}`);
    });
});
