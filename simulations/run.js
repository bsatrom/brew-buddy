var Simulator = require('./controllers/simulate-brew');
var Simulation = require('./config/Simulation');

Simulator.init(Simulation);
console.log(Simulator.status());

Simulator.run();
console.log(Simulator.status());
