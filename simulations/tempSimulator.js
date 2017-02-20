var simStrategy, lowTemp, highTemp, interval, totalDuration;

const simulateLinear = (currentDuration) => {
  const tempRange = highTemp - lowTemp;
  const percentageComplete = currentDuration / totalDuration;
  
  return (tempRange * percentageComplete) + lowTemp
;
};

const simulateRandom = (currentDuration) => {
  return Math.floor(Math.random() * (highTemp - lowTemp + 1) + lowTemp);
}

const simulationStrategies = {
  'linear': simulateLinear,
  'random': simulateRandom
};

const initSimulator = (strategy, opts) => {
  simStrategy = strategy;

  lowTemp = opts.lowTemp;
  highTemp = opts.highTemp;
  interval = opts.interval;
  totalDuration = opts.totalDuration;
};

const simulateTemp = (currentDuration) => {
  return simulationStrategies[simStrategy](currentDuration);
};

module.exports = {
  init: initSimulator,
  getTemp: simulateTemp,
}