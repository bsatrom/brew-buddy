var Simulator = require('./simulate-brew');
var Simulation = require('./Simulation');

Simulator.init(Simulation);
console.log(Simulator.status());

Simulator.run();
console.log(Simulator.status());